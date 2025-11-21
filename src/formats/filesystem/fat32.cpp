/**
 * FAT32 File System Implementation
 */

#include "fat32.h"
#include <cstring>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <functional>

namespace fconvert {
namespace formats {

namespace fs = std::filesystem;

bool FAT32Codec::is_fat32(const uint8_t* data, size_t size) {
    if (size < sizeof(Fat32BootSector)) {
        return false;
    }

    const Fat32BootSector* boot = reinterpret_cast<const Fat32BootSector*>(data);

    // Check signature
    if (boot->signature != 0xAA55) {
        return false;
    }

    // Check for FAT32 markers
    if (boot->fat_size_16 != 0) {
        return false;  // FAT16 or FAT12
    }

    // Check filesystem type string
    std::string fs_type(boot->fs_type, 8);
    return fs_type.find("FAT32") != std::string::npos;
}

std::string FAT32Codec::decode_83_name(const char* name) {
    std::string result;

    // First 8 characters are the name
    for (int i = 0; i < 8 && name[i] != ' '; i++) {
        result += name[i];
    }

    // Last 3 characters are the extension
    if (name[8] != ' ') {
        result += '.';
        for (int i = 8; i < 11 && name[i] != ' '; i++) {
            result += name[i];
        }
    }

    return result;
}

void FAT32Codec::encode_83_name(const std::string& name, char* out) {
    std::memset(out, ' ', 11);

    // Handle . and ..
    if (name == ".") {
        out[0] = '.';
        return;
    }
    if (name == "..") {
        out[0] = '.';
        out[1] = '.';
        return;
    }

    // Find extension
    size_t dot = name.rfind('.');
    std::string base = (dot != std::string::npos) ? name.substr(0, dot) : name;
    std::string ext = (dot != std::string::npos) ? name.substr(dot + 1) : "";

    // Convert to uppercase
    std::transform(base.begin(), base.end(), base.begin(), ::toupper);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

    // Copy base (max 8 chars)
    for (size_t i = 0; i < std::min(base.size(), size_t(8)); i++) {
        out[i] = base[i];
    }

    // Copy extension (max 3 chars)
    for (size_t i = 0; i < std::min(ext.size(), size_t(3)); i++) {
        out[8 + i] = ext[i];
    }
}

uint8_t FAT32Codec::lfn_checksum(const char* name) {
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + name[i];
    }
    return sum;
}

uint32_t FAT32Codec::cluster_to_sector(const Fat32Image& image, uint32_t cluster) {
    return image.data_start_sector + (cluster - 2) * image.sectors_per_cluster;
}

void FAT32Codec::read_cluster(
    const Fat32Image& image,
    uint32_t cluster,
    uint8_t* buffer) {

    uint32_t sector = cluster_to_sector(image, cluster);
    uint32_t offset = sector * image.bytes_per_sector;
    uint32_t size = image.sectors_per_cluster * image.bytes_per_sector;

    if (offset + size <= image.data.size()) {
        std::memcpy(buffer, image.data.data() + offset, size);
    } else {
        std::memset(buffer, 0, size);
    }
}

void FAT32Codec::write_cluster(
    Fat32Image& image,
    uint32_t cluster,
    const uint8_t* data) {

    uint32_t sector = cluster_to_sector(image, cluster);
    uint32_t offset = sector * image.bytes_per_sector;
    uint32_t size = image.sectors_per_cluster * image.bytes_per_sector;

    if (offset + size <= image.data.size()) {
        std::memcpy(image.data.data() + offset, data, size);
    }
}

std::vector<uint32_t> FAT32Codec::get_cluster_chain(
    const Fat32Image& image,
    uint32_t start_cluster) {

    std::vector<uint32_t> chain;
    uint32_t cluster = start_cluster;

    while (cluster >= 2 && cluster < image.fat.size() &&
           cluster < FAT32_BAD_CLUSTER) {
        chain.push_back(cluster);
        uint32_t next = image.fat[cluster] & 0x0FFFFFFF;
        if (next >= FAT32_EOC) break;
        cluster = next;
    }

    return chain;
}

uint32_t FAT32Codec::allocate_cluster(Fat32Image& image) {
    for (uint32_t i = 2; i < image.fat.size(); i++) {
        if (image.fat[i] == FAT32_FREE_CLUSTER) {
            image.fat[i] = FAT32_EOC;
            return i;
        }
    }
    return 0;  // No free clusters
}

void FAT32Codec::free_cluster_chain(Fat32Image& image, uint32_t start_cluster) {
    uint32_t cluster = start_cluster;

    while (cluster >= 2 && cluster < image.fat.size() &&
           cluster < FAT32_BAD_CLUSTER) {
        uint32_t next = image.fat[cluster] & 0x0FFFFFFF;
        image.fat[cluster] = FAT32_FREE_CLUSTER;
        if (next >= FAT32_EOC) break;
        cluster = next;
    }
}

fconvert_error_t FAT32Codec::parse_directory(
    const Fat32Image& image,
    uint32_t cluster,
    FatFileEntry& dir) {

    std::vector<uint32_t> chain = get_cluster_chain(image, cluster);
    uint32_t cluster_size = image.sectors_per_cluster * image.bytes_per_sector;
    std::vector<uint8_t> buffer(cluster_size);

    std::string lfn_name;
    uint8_t lfn_checksum_val = 0;

    for (uint32_t clust : chain) {
        read_cluster(image, clust, buffer.data());

        for (uint32_t i = 0; i < cluster_size; i += 32) {
            const Fat32DirEntry* entry = reinterpret_cast<const Fat32DirEntry*>(buffer.data() + i);

            // End of directory
            if (entry->name[0] == 0x00) {
                return FCONVERT_OK;
            }

            // Deleted entry
            if (static_cast<uint8_t>(entry->name[0]) == 0xE5) {
                continue;
            }

            // Long filename entry
            if (entry->attr == FAT32_ATTR_LONG_NAME) {
                const Fat32LfnEntry* lfn = reinterpret_cast<const Fat32LfnEntry*>(entry);

                // Build LFN name
                char chars[13];
                for (int j = 0; j < 5; j++) {
                    chars[j] = (lfn->name1[j] <= 0xFF) ? static_cast<char>(lfn->name1[j]) : '?';
                }
                for (int j = 0; j < 6; j++) {
                    chars[5 + j] = (lfn->name2[j] <= 0xFF) ? static_cast<char>(lfn->name2[j]) : '?';
                }
                for (int j = 0; j < 2; j++) {
                    chars[11 + j] = (lfn->name3[j] <= 0xFF) ? static_cast<char>(lfn->name3[j]) : '?';
                }

                std::string part;
                for (int j = 0; j < 13 && chars[j] != 0 && chars[j] != -1; j++) {
                    part += chars[j];
                }

                if (lfn->order & 0x40) {
                    // First LFN entry (last part of name)
                    lfn_name = part;
                    lfn_checksum_val = lfn->checksum;
                } else {
                    lfn_name = part + lfn_name;
                }
                continue;
            }

            // Skip volume label
            if (entry->attr & FAT32_ATTR_VOLUME_ID) {
                lfn_name.clear();
                continue;
            }

            // Regular entry
            FatFileEntry file;

            // Use LFN if available and checksum matches
            if (!lfn_name.empty() && lfn_checksum(entry->name) == lfn_checksum_val) {
                file.name = lfn_name;
            } else {
                file.name = decode_83_name(entry->name);
            }

            // Skip . and ..
            if (file.name == "." || file.name == "..") {
                lfn_name.clear();
                continue;
            }

            file.path = dir.path.empty() ? file.name : dir.path + "/" + file.name;
            file.first_cluster = (static_cast<uint32_t>(entry->first_cluster_high) << 16) |
                                 entry->first_cluster_low;
            file.size = entry->file_size;
            file.is_directory = (entry->attr & FAT32_ATTR_DIRECTORY) != 0;
            file.attributes = entry->attr;
            file.create_date = entry->create_date;
            file.create_time = entry->create_time;
            file.modify_date = entry->write_date;
            file.modify_time = entry->write_time;

            // Parse subdirectories recursively
            if (file.is_directory && file.first_cluster >= 2) {
                parse_directory(image, file.first_cluster, file);
            }

            dir.children.push_back(file);
            lfn_name.clear();
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t FAT32Codec::decode(
    const std::vector<uint8_t>& data,
    Fat32Image& image) {

    if (!is_fat32(data.data(), data.size())) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    const Fat32BootSector* boot = reinterpret_cast<const Fat32BootSector*>(data.data());

    // Copy basic info
    image.bytes_per_sector = boot->bytes_per_sector;
    image.sectors_per_cluster = boot->sectors_per_cluster;
    image.fat_size = boot->fat_size_32;
    image.total_sectors = boot->total_sectors_32;
    image.volume_serial = boot->volume_serial;
    image.volume_label = std::string(boot->volume_label, 11);
    image.volume_label.erase(image.volume_label.find_last_not_of(' ') + 1);

    // Calculate derived values
    uint32_t fat_start = boot->reserved_sectors;
    image.data_start_sector = fat_start + boot->num_fats * boot->fat_size_32;
    image.total_clusters = (image.total_sectors - image.data_start_sector) /
                           image.sectors_per_cluster;

    // Read FAT
    image.fat.resize(image.total_clusters + 2);
    uint32_t fat_offset = fat_start * image.bytes_per_sector;
    const uint32_t* fat_data = reinterpret_cast<const uint32_t*>(data.data() + fat_offset);

    for (uint32_t i = 0; i < image.fat.size() && i * 4 < boot->fat_size_32 * image.bytes_per_sector; i++) {
        image.fat[i] = fat_data[i];
    }

    // Store raw data
    image.data = data;

    // Parse root directory
    image.root.name = "";
    image.root.path = "";
    image.root.first_cluster = boot->root_cluster;
    image.root.is_directory = true;

    return parse_directory(image, boot->root_cluster, image.root);
}

std::vector<std::string> FAT32Codec::list_files(const Fat32Image& image) {
    std::vector<std::string> files;

    std::function<void(const FatFileEntry&)> collect = [&](const FatFileEntry& entry) {
        if (!entry.path.empty()) {
            files.push_back(entry.path + (entry.is_directory ? "/" : ""));
        }
        for (const auto& child : entry.children) {
            collect(child);
        }
    };

    collect(image.root);
    return files;
}

fconvert_error_t FAT32Codec::read_file(
    const Fat32Image& image,
    const std::string& path,
    std::vector<uint8_t>& file_data) {

    // Find file
    std::function<const FatFileEntry*(const FatFileEntry&, const std::string&)> find_file;
    find_file = [&](const FatFileEntry& entry, const std::string& target) -> const FatFileEntry* {
        if (entry.path == target) {
            return &entry;
        }
        for (const auto& child : entry.children) {
            const FatFileEntry* result = find_file(child, target);
            if (result) return result;
        }
        return nullptr;
    };

    const FatFileEntry* file = find_file(image.root, path);
    if (!file || file->is_directory) {
        return FCONVERT_ERROR_FILE_NOT_FOUND;
    }

    // Read file data
    std::vector<uint32_t> chain = get_cluster_chain(image, file->first_cluster);
    uint32_t cluster_size = image.sectors_per_cluster * image.bytes_per_sector;

    file_data.resize(file->size);
    uint32_t remaining = file->size;
    uint32_t offset = 0;

    std::vector<uint8_t> buffer(cluster_size);
    for (uint32_t cluster : chain) {
        read_cluster(image, cluster, buffer.data());
        uint32_t copy_size = std::min(remaining, cluster_size);
        std::memcpy(file_data.data() + offset, buffer.data(), copy_size);
        offset += copy_size;
        remaining -= copy_size;
    }

    return FCONVERT_OK;
}

fconvert_error_t FAT32Codec::extract_to_directory(
    const Fat32Image& image,
    const std::string& dest_path) {

    std::function<fconvert_error_t(const FatFileEntry&, const std::string&)> extract;
    extract = [&](const FatFileEntry& entry, const std::string& base_path) -> fconvert_error_t {
        std::string full_path = base_path;
        if (!entry.name.empty()) {
            full_path += "/" + entry.name;
        }

        if (entry.is_directory) {
            if (!entry.name.empty()) {
                std::error_code ec;
                fs::create_directories(full_path, ec);
                if (ec) return FCONVERT_ERROR_IO;
            }

            for (const auto& child : entry.children) {
                fconvert_error_t result = extract(child, full_path);
                if (result != FCONVERT_OK) return result;
            }
        } else {
            std::vector<uint8_t> file_data;
            fconvert_error_t result = read_file(image, entry.path, file_data);
            if (result != FCONVERT_OK) return result;

            std::ofstream out(full_path, std::ios::binary);
            if (!out) return FCONVERT_ERROR_IO;
            out.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        }

        return FCONVERT_OK;
    };

    std::error_code ec;
    fs::create_directories(dest_path, ec);
    if (ec) return FCONVERT_ERROR_IO;

    return extract(image.root, dest_path);
}

fconvert_error_t FAT32Codec::create_from_directory(
    const std::string& source_path,
    Fat32Image& image,
    uint64_t size_bytes,
    const std::string& volume_label) {

    if (!fs::exists(source_path) || !fs::is_directory(source_path)) {
        return FCONVERT_ERROR_FILE_NOT_FOUND;
    }

    // Calculate required size
    uint64_t content_size = 0;
    for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
        if (entry.is_regular_file()) {
            content_size += entry.file_size();
        }
    }

    // Add overhead (FAT, directories, etc.)
    if (size_bytes == 0) {
        size_bytes = content_size * 2 + 16 * 1024 * 1024;  // 2x content + 16MB overhead
        // Round up to 16MB boundary
        size_bytes = ((size_bytes + 16 * 1024 * 1024 - 1) / (16 * 1024 * 1024)) * 16 * 1024 * 1024;
        // Minimum 32MB
        if (size_bytes < 32 * 1024 * 1024) {
            size_bytes = 32 * 1024 * 1024;
        }
    }

    // Setup image parameters
    image.bytes_per_sector = FAT32_BYTES_PER_SECTOR;
    image.sectors_per_cluster = FAT32_SECTORS_PER_CLUSTER_DEFAULT;
    image.total_sectors = static_cast<uint32_t>(size_bytes / image.bytes_per_sector);
    image.volume_label = volume_label;
    image.volume_serial = static_cast<uint32_t>(time(nullptr));

    // Calculate FAT size
    uint32_t cluster_size = image.sectors_per_cluster * image.bytes_per_sector;
    uint32_t data_sectors = image.total_sectors - FAT32_RESERVED_SECTORS;
    image.total_clusters = data_sectors / image.sectors_per_cluster;
    image.fat_size = (image.total_clusters * 4 + image.bytes_per_sector - 1) / image.bytes_per_sector;
    image.data_start_sector = FAT32_RESERVED_SECTORS + FAT32_NUM_FATS * image.fat_size;

    // Recalculate clusters
    data_sectors = image.total_sectors - image.data_start_sector;
    image.total_clusters = data_sectors / image.sectors_per_cluster;

    // Allocate image data
    image.data.resize(image.total_sectors * image.bytes_per_sector, 0);

    // Initialize FAT
    image.fat.resize(image.total_clusters + 2, FAT32_FREE_CLUSTER);
    image.fat[0] = 0x0FFFFFF8;  // Media type
    image.fat[1] = 0x0FFFFFFF;  // End marker

    // Setup boot sector
    Fat32BootSector* boot = reinterpret_cast<Fat32BootSector*>(image.data.data());
    boot->jump[0] = 0xEB;
    boot->jump[1] = 0x58;
    boot->jump[2] = 0x90;
    std::memcpy(boot->oem_name, "FCONVRT ", 8);
    boot->bytes_per_sector = image.bytes_per_sector;
    boot->sectors_per_cluster = static_cast<uint8_t>(image.sectors_per_cluster);
    boot->reserved_sectors = FAT32_RESERVED_SECTORS;
    boot->num_fats = FAT32_NUM_FATS;
    boot->root_entries = 0;
    boot->total_sectors_16 = 0;
    boot->media_type = 0xF8;
    boot->fat_size_16 = 0;
    boot->sectors_per_track = 63;
    boot->num_heads = 255;
    boot->hidden_sectors = 0;
    boot->total_sectors_32 = image.total_sectors;
    boot->fat_size_32 = image.fat_size;
    boot->ext_flags = 0;
    boot->fs_version = 0;
    boot->root_cluster = FAT32_ROOT_CLUSTER;
    boot->fs_info_sector = 1;
    boot->backup_boot_sector = 6;
    boot->drive_number = 0x80;
    boot->boot_signature = 0x29;
    boot->volume_serial = image.volume_serial;
    std::memset(boot->volume_label, ' ', 11);
    std::memcpy(boot->volume_label, volume_label.c_str(),
               std::min(volume_label.size(), size_t(11)));
    std::memcpy(boot->fs_type, "FAT32   ", 8);
    boot->signature = 0xAA55;

    // Allocate root directory cluster
    image.fat[FAT32_ROOT_CLUSTER] = FAT32_EOC;

    // Initialize root directory
    image.root.name = "";
    image.root.path = "";
    image.root.first_cluster = FAT32_ROOT_CLUSTER;
    image.root.is_directory = true;

    // Build file tree and copy files
    std::function<fconvert_error_t(const fs::path&, FatFileEntry&, uint32_t)> add_files;
    add_files = [&](const fs::path& path, FatFileEntry& parent_entry, uint32_t parent_cluster) -> fconvert_error_t {
        std::vector<Fat32DirEntry> dir_entries;

        // Add . and .. entries
        Fat32DirEntry dot{}, dotdot{};
        encode_83_name(".", dot.name);
        dot.attr = FAT32_ATTR_DIRECTORY;
        dot.first_cluster_low = parent_cluster & 0xFFFF;
        dot.first_cluster_high = parent_cluster >> 16;

        encode_83_name("..", dotdot.name);
        dotdot.attr = FAT32_ATTR_DIRECTORY;
        // Parent cluster is 0 for root

        if (parent_cluster != FAT32_ROOT_CLUSTER) {
            dir_entries.push_back(dot);
            dir_entries.push_back(dotdot);
        }

        for (const auto& item : fs::directory_iterator(path)) {
            FatFileEntry entry;
            entry.name = item.path().filename().string();
            entry.path = parent_entry.path.empty() ? entry.name : parent_entry.path + "/" + entry.name;
            entry.is_directory = item.is_directory();

            Fat32DirEntry dir_entry{};
            encode_83_name(entry.name, dir_entry.name);

            if (entry.is_directory) {
                // Allocate cluster for directory
                entry.first_cluster = allocate_cluster(image);
                if (entry.first_cluster == 0) return FCONVERT_ERROR_MEMORY;

                dir_entry.attr = FAT32_ATTR_DIRECTORY;
                dir_entry.first_cluster_low = entry.first_cluster & 0xFFFF;
                dir_entry.first_cluster_high = entry.first_cluster >> 16;

                fconvert_error_t result = add_files(item.path(), entry, entry.first_cluster);
                if (result != FCONVERT_OK) return result;
            } else {
                // Read file and allocate clusters
                std::ifstream in(item.path(), std::ios::binary);
                if (!in) continue;

                std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(in)),
                                                std::istreambuf_iterator<char>());

                entry.size = static_cast<uint32_t>(file_data.size());

                if (entry.size > 0) {
                    // Allocate cluster chain
                    uint32_t clusters_needed = (entry.size + cluster_size - 1) / cluster_size;
                    uint32_t prev_cluster = 0;

                    for (uint32_t i = 0; i < clusters_needed; i++) {
                        uint32_t new_cluster = allocate_cluster(image);
                        if (new_cluster == 0) return FCONVERT_ERROR_MEMORY;

                        if (i == 0) {
                            entry.first_cluster = new_cluster;
                        } else {
                            image.fat[prev_cluster] = new_cluster;
                        }
                        prev_cluster = new_cluster;

                        // Write data to cluster
                        uint32_t offset = i * cluster_size;
                        uint32_t copy_size = std::min(cluster_size, entry.size - offset);
                        std::vector<uint8_t> cluster_data(cluster_size, 0);
                        std::memcpy(cluster_data.data(), file_data.data() + offset, copy_size);
                        write_cluster(image, new_cluster, cluster_data.data());
                    }
                }

                dir_entry.attr = FAT32_ATTR_ARCHIVE;
                dir_entry.first_cluster_low = entry.first_cluster & 0xFFFF;
                dir_entry.first_cluster_high = entry.first_cluster >> 16;
                dir_entry.file_size = entry.size;
            }

            dir_entries.push_back(dir_entry);
            parent_entry.children.push_back(entry);
        }

        // Write directory entries
        std::vector<uint8_t> dir_data(cluster_size, 0);
        for (size_t i = 0; i < dir_entries.size() && i * 32 < cluster_size; i++) {
            std::memcpy(dir_data.data() + i * 32, &dir_entries[i], 32);
        }
        write_cluster(image, parent_cluster, dir_data.data());

        return FCONVERT_OK;
    };

    fconvert_error_t result = add_files(source_path, image.root, FAT32_ROOT_CLUSTER);
    if (result != FCONVERT_OK) return result;

    // Write FAT to image
    uint32_t fat_offset = FAT32_RESERVED_SECTORS * image.bytes_per_sector;
    for (uint32_t i = 0; i < FAT32_NUM_FATS; i++) {
        uint32_t* fat_ptr = reinterpret_cast<uint32_t*>(image.data.data() + fat_offset +
                                                         i * image.fat_size * image.bytes_per_sector);
        for (size_t j = 0; j < image.fat.size(); j++) {
            fat_ptr[j] = image.fat[j];
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t FAT32Codec::encode(
    const Fat32Image& image,
    std::vector<uint8_t>& data) {

    data = image.data;
    return FCONVERT_OK;
}

fconvert_error_t FAT32Codec::write_file(
    Fat32Image& image,
    const std::string& path,
    const std::vector<uint8_t>& file_data) {

    (void)image;
    (void)path;
    (void)file_data;
    // TODO: Implement file writing
    return FCONVERT_ERROR_INVALID_FORMAT;
}

fconvert_error_t FAT32Codec::delete_file(
    Fat32Image& image,
    const std::string& path) {

    (void)image;
    (void)path;
    // TODO: Implement file deletion
    return FCONVERT_ERROR_INVALID_FORMAT;
}

fconvert_error_t FAT32Codec::create_directory(
    Fat32Image& image,
    const std::string& path) {

    (void)image;
    (void)path;
    // TODO: Implement directory creation
    return FCONVERT_ERROR_INVALID_FORMAT;
}

} // namespace formats
} // namespace fconvert
