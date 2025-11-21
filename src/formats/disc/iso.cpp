/**
 * ISO 9660 Disc Image Format Implementation
 */

#include "iso.h"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <functional>

#ifdef _MSC_VER
#include <stdlib.h>
#define bswap32(x) _byteswap_ulong(x)
#define bswap16(x) _byteswap_ushort(x)
#else
#define bswap32(x) bswap32(x)
#define bswap16(x) bswap16(x)
#endif

namespace fconvert {
namespace formats {

namespace fs = std::filesystem;

bool ISOCodec::is_iso(const uint8_t* data, size_t size) {
    // Check for ISO 9660 signature at sector 16
    if (size < ISO_SYSTEM_AREA_SIZE + ISO_SECTOR_SIZE) {
        return false;
    }

    const uint8_t* vd = data + ISO_SYSTEM_AREA_SIZE;

    // Check "CD001" identifier
    return (vd[0] == ISO_VD_PRIMARY &&
            std::memcmp(vd + 1, "CD001", 5) == 0);
}

std::string ISOCodec::decode_filename(const char* name, uint8_t length) {
    std::string result(name, length);

    // Remove version number (;1)
    size_t semi = result.find(';');
    if (semi != std::string::npos) {
        result = result.substr(0, semi);
    }

    // Remove trailing dot for directories
    if (!result.empty() && result.back() == '.') {
        result.pop_back();
    }

    return result;
}

std::string ISOCodec::encode_filename(const std::string& name, bool is_dir) {
    std::string result = name;

    // Convert to uppercase
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);

    // Replace invalid characters
    for (char& c : result) {
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.')) {
            c = '_';
        }
    }

    // Truncate to 8.3 format if no extension
    if (result.find('.') == std::string::npos && result.length() > 8) {
        result = result.substr(0, 8);
    }

    // Add version number for files
    if (!is_dir) {
        result += ";1";
    }

    return result;
}

fconvert_error_t ISOCodec::parse_directory(
    const uint8_t* data,
    size_t data_size,
    uint32_t location,
    uint32_t length,
    IsoFileEntry& dir) {

    if (location * ISO_SECTOR_SIZE + length > data_size) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    const uint8_t* dir_data = data + location * ISO_SECTOR_SIZE;
    size_t offset = 0;

    while (offset < length) {
        const IsoDirRecord* record = reinterpret_cast<const IsoDirRecord*>(dir_data + offset);

        if (record->length == 0) {
            // Move to next sector
            offset = ((offset / ISO_SECTOR_SIZE) + 1) * ISO_SECTOR_SIZE;
            if (offset >= length) break;
            continue;
        }

        // Get filename
        const char* name_ptr = reinterpret_cast<const char*>(dir_data + offset + 33);
        std::string name = decode_filename(name_ptr, record->name_length);

        // Skip . and .. entries
        if (record->name_length == 1 && (name_ptr[0] == 0 || name_ptr[0] == 1)) {
            offset += record->length;
            continue;
        }

        IsoFileEntry entry;
        entry.name = name;
        entry.path = dir.path.empty() ? name : dir.path + "/" + name;
        entry.location = record->extent_location_le;
        entry.size = record->data_length_le;
        entry.is_directory = (record->flags & ISO_FLAG_DIRECTORY) != 0;
        entry.date = record->recording_date;

        // Recursively parse subdirectories
        if (entry.is_directory && entry.location != location) {
            fconvert_error_t result = parse_directory(
                data, data_size, entry.location, entry.size, entry);
            if (result != FCONVERT_OK) {
                return result;
            }
        }

        dir.children.push_back(entry);
        offset += record->length;
    }

    return FCONVERT_OK;
}

fconvert_error_t ISOCodec::decode(
    const std::vector<uint8_t>& data,
    IsoImage& image) {

    if (!is_iso(data.data(), data.size())) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Parse primary volume descriptor
    const IsoVolumeDescriptor* pvd = reinterpret_cast<const IsoVolumeDescriptor*>(
        data.data() + ISO_SYSTEM_AREA_SIZE);

    // Extract volume info
    image.volume_id = std::string(pvd->volume_id, 32);
    image.volume_id.erase(image.volume_id.find_last_not_of(' ') + 1);

    image.system_id = std::string(pvd->system_id, 32);
    image.system_id.erase(image.system_id.find_last_not_of(' ') + 1);

    image.publisher_id = std::string(pvd->publisher_id, 128);
    image.publisher_id.erase(image.publisher_id.find_last_not_of(' ') + 1);

    image.application_id = std::string(pvd->application_id, 128);
    image.application_id.erase(image.application_id.find_last_not_of(' ') + 1);

    image.sector_count = pvd->volume_space_size_le;

    // Parse root directory
    const IsoDirRecord* root_record = reinterpret_cast<const IsoDirRecord*>(pvd->root_dir_record);
    image.root.name = "";
    image.root.path = "";
    image.root.location = root_record->extent_location_le;
    image.root.size = root_record->data_length_le;
    image.root.is_directory = true;

    fconvert_error_t result = parse_directory(
        data.data(), data.size(),
        image.root.location, image.root.size,
        image.root);

    if (result != FCONVERT_OK) {
        return result;
    }

    // Store raw data for file extraction
    image.data = data;

    return FCONVERT_OK;
}

std::vector<std::string> ISOCodec::list_files(const IsoImage& image) {
    std::vector<std::string> files;

    std::function<void(const IsoFileEntry&)> collect = [&](const IsoFileEntry& entry) {
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

fconvert_error_t ISOCodec::read_file(
    const IsoImage& image,
    const std::string& path,
    std::vector<uint8_t>& file_data) {

    // Find file in image
    std::function<const IsoFileEntry*(const IsoFileEntry&, const std::string&)> find_file;
    find_file = [&](const IsoFileEntry& entry, const std::string& target) -> const IsoFileEntry* {
        if (entry.path == target) {
            return &entry;
        }
        for (const auto& child : entry.children) {
            const IsoFileEntry* result = find_file(child, target);
            if (result) return result;
        }
        return nullptr;
    };

    const IsoFileEntry* file = find_file(image.root, path);
    if (!file || file->is_directory) {
        return FCONVERT_ERROR_FILE_NOT_FOUND;
    }

    // Extract file data
    size_t offset = file->location * ISO_SECTOR_SIZE;
    if (offset + file->size > image.data.size()) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    file_data.resize(file->size);
    std::memcpy(file_data.data(), image.data.data() + offset, file->size);

    return FCONVERT_OK;
}

fconvert_error_t ISOCodec::extract_to_directory(
    const IsoImage& image,
    const std::string& dest_path) {

    std::function<fconvert_error_t(const IsoFileEntry&, const std::string&)> extract;
    extract = [&](const IsoFileEntry& entry, const std::string& base_path) -> fconvert_error_t {
        std::string full_path = base_path;
        if (!entry.name.empty()) {
            full_path += "/" + entry.name;
        }

        if (entry.is_directory) {
            // Create directory
            if (!entry.name.empty()) {
                std::error_code ec;
                fs::create_directories(full_path, ec);
                if (ec) {
                    return FCONVERT_ERROR_IO;
                }
            }

            // Extract children
            for (const auto& child : entry.children) {
                fconvert_error_t result = extract(child, full_path);
                if (result != FCONVERT_OK) {
                    return result;
                }
            }
        } else {
            // Extract file
            std::vector<uint8_t> file_data;
            fconvert_error_t result = read_file(image, entry.path, file_data);
            if (result != FCONVERT_OK) {
                return result;
            }

            std::ofstream out(full_path, std::ios::binary);
            if (!out) {
                return FCONVERT_ERROR_IO;
            }
            out.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        }

        return FCONVERT_OK;
    };

    // Create destination directory
    std::error_code ec;
    fs::create_directories(dest_path, ec);
    if (ec) {
        return FCONVERT_ERROR_IO;
    }

    return extract(image.root, dest_path);
}

uint32_t ISOCodec::calculate_dir_size(const IsoFileEntry& dir) {
    uint32_t size = 68;  // . and .. entries (34 bytes each)

    for (const auto& child : dir.children) {
        std::string iso_name = encode_filename(child.name, child.is_directory);
        size += 33 + iso_name.length();
        // Pad to even
        if (size % 2) size++;
    }

    // Round up to sector
    return ((size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
}

fconvert_error_t ISOCodec::create_from_directory(
    const std::string& source_path,
    IsoImage& image,
    const std::string& volume_id) {

    if (!fs::exists(source_path) || !fs::is_directory(source_path)) {
        return FCONVERT_ERROR_FILE_NOT_FOUND;
    }

    image.volume_id = volume_id;
    image.system_id = "FCONVERT";
    image.publisher_id = "";
    image.application_id = "FCONVERT ISO CREATOR";

    // Build file tree and calculate sizes
    std::function<void(const fs::path&, IsoFileEntry&)> build_tree;
    build_tree = [&](const fs::path& path, IsoFileEntry& entry) {
        for (const auto& item : fs::directory_iterator(path)) {
            IsoFileEntry child;
            child.name = item.path().filename().string();
            child.path = entry.path.empty() ? child.name : entry.path + "/" + child.name;
            child.is_directory = item.is_directory();

            if (child.is_directory) {
                build_tree(item.path(), child);
                child.size = calculate_dir_size(child);
            } else {
                child.size = static_cast<uint32_t>(fs::file_size(item.path()));
            }

            entry.children.push_back(child);
        }
    };

    image.root.name = "";
    image.root.path = "";
    image.root.is_directory = true;
    build_tree(source_path, image.root);
    image.root.size = calculate_dir_size(image.root);

    // Calculate sector layout
    uint32_t current_sector = 18;  // After system area and volume descriptors

    // Assign sectors to directories first
    std::function<void(IsoFileEntry&)> assign_dir_sectors;
    assign_dir_sectors = [&](IsoFileEntry& entry) {
        if (entry.is_directory) {
            entry.location = current_sector;
            current_sector += (entry.size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE;

            for (auto& child : entry.children) {
                assign_dir_sectors(child);
            }
        }
    };
    assign_dir_sectors(image.root);

    // Assign sectors to files
    std::function<void(IsoFileEntry&)> assign_file_sectors;
    assign_file_sectors = [&](IsoFileEntry& entry) {
        if (!entry.is_directory) {
            entry.location = current_sector;
            current_sector += (entry.size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE;
        }
        for (auto& child : entry.children) {
            assign_file_sectors(child);
        }
    };
    assign_file_sectors(image.root);

    image.sector_count = current_sector;

    // Build ISO image
    image.data.resize(current_sector * ISO_SECTOR_SIZE, 0);

    // Write volume descriptors
    IsoVolumeDescriptor* pvd = reinterpret_cast<IsoVolumeDescriptor*>(
        image.data.data() + ISO_SYSTEM_AREA_SIZE);

    pvd->type = ISO_VD_PRIMARY;
    std::memcpy(pvd->identifier, "CD001", 5);
    pvd->version = 1;

    std::memset(pvd->system_id, ' ', 32);
    std::memcpy(pvd->system_id, image.system_id.c_str(),
                std::min(image.system_id.size(), size_t(32)));

    std::memset(pvd->volume_id, ' ', 32);
    std::memcpy(pvd->volume_id, image.volume_id.c_str(),
                std::min(image.volume_id.size(), size_t(32)));

    pvd->volume_space_size_le = image.sector_count;
    pvd->volume_space_size_be = bswap32(image.sector_count);
    pvd->volume_set_size_le = 1;
    pvd->volume_set_size_be = 0x0100;
    pvd->volume_seq_number_le = 1;
    pvd->volume_seq_number_be = 0x0100;
    pvd->logical_block_size_le = ISO_SECTOR_SIZE;
    pvd->logical_block_size_be = bswap16(ISO_SECTOR_SIZE);
    pvd->file_structure_version = 1;

    // Write root directory record in PVD
    IsoDirRecord* root_record = reinterpret_cast<IsoDirRecord*>(pvd->root_dir_record);
    root_record->length = 34;
    root_record->extent_location_le = image.root.location;
    root_record->extent_location_be = bswap32(image.root.location);
    root_record->data_length_le = image.root.size;
    root_record->data_length_be = bswap32(image.root.size);
    root_record->flags = ISO_FLAG_DIRECTORY;
    root_record->volume_seq_le = 1;
    root_record->volume_seq_be = 0x0100;
    root_record->name_length = 1;

    // Write volume descriptor terminator
    uint8_t* term = image.data.data() + ISO_SYSTEM_AREA_SIZE + ISO_SECTOR_SIZE;
    term[0] = ISO_VD_TERMINATOR;
    std::memcpy(term + 1, "CD001", 5);
    term[6] = 1;

    // Write directories
    std::function<void(const IsoFileEntry&, uint32_t)> write_dirs;
    write_dirs = [&](const IsoFileEntry& entry, uint32_t parent_loc) {
        if (!entry.is_directory) return;

        uint8_t* dir_ptr = image.data.data() + entry.location * ISO_SECTOR_SIZE;
        size_t offset = 0;

        // Write . entry
        IsoDirRecord* dot = reinterpret_cast<IsoDirRecord*>(dir_ptr + offset);
        dot->length = 34;
        dot->extent_location_le = entry.location;
        dot->extent_location_be = bswap32(entry.location);
        dot->data_length_le = entry.size;
        dot->data_length_be = bswap32(entry.size);
        dot->flags = ISO_FLAG_DIRECTORY;
        dot->volume_seq_le = 1;
        dot->volume_seq_be = 0x0100;
        dot->name_length = 1;
        dir_ptr[offset + 33] = 0;
        offset += 34;

        // Write .. entry
        IsoDirRecord* dotdot = reinterpret_cast<IsoDirRecord*>(dir_ptr + offset);
        dotdot->length = 34;
        dotdot->extent_location_le = parent_loc;
        dotdot->extent_location_be = bswap32(parent_loc);
        dotdot->data_length_le = entry.size;  // Approximate
        dotdot->data_length_be = bswap32(entry.size);
        dotdot->flags = ISO_FLAG_DIRECTORY;
        dotdot->volume_seq_le = 1;
        dotdot->volume_seq_be = 0x0100;
        dotdot->name_length = 1;
        dir_ptr[offset + 33] = 1;
        offset += 34;

        // Write children
        for (const auto& child : entry.children) {
            std::string iso_name = encode_filename(child.name, child.is_directory);
            uint8_t rec_len = 33 + static_cast<uint8_t>(iso_name.length());
            if (rec_len % 2) rec_len++;

            // Check if we need to move to next sector
            if (offset + rec_len > ISO_SECTOR_SIZE) {
                offset = ((offset / ISO_SECTOR_SIZE) + 1) * ISO_SECTOR_SIZE;
            }

            IsoDirRecord* rec = reinterpret_cast<IsoDirRecord*>(dir_ptr + offset);
            rec->length = rec_len;
            rec->extent_location_le = child.location;
            rec->extent_location_be = bswap32(child.location);
            rec->data_length_le = child.size;
            rec->data_length_be = bswap32(child.size);
            rec->flags = child.is_directory ? ISO_FLAG_DIRECTORY : 0;
            rec->volume_seq_le = 1;
            rec->volume_seq_be = 0x0100;
            rec->name_length = static_cast<uint8_t>(iso_name.length());
            std::memcpy(dir_ptr + offset + 33, iso_name.c_str(), iso_name.length());

            offset += rec_len;
        }

        // Recursively write subdirectories
        for (const auto& child : entry.children) {
            if (child.is_directory) {
                write_dirs(child, entry.location);
            }
        }
    };

    write_dirs(image.root, image.root.location);

    // Write file contents
    std::function<void(const IsoFileEntry&, const fs::path&)> write_files;
    write_files = [&](const IsoFileEntry& entry, const fs::path& base) {
        fs::path current = entry.name.empty() ? base : base / entry.name;

        if (!entry.is_directory) {
            std::ifstream in(current, std::ios::binary);
            if (in) {
                in.read(reinterpret_cast<char*>(
                    image.data.data() + entry.location * ISO_SECTOR_SIZE), entry.size);
            }
        }

        for (const auto& child : entry.children) {
            write_files(child, current);
        }
    };

    write_files(image.root, source_path);

    return FCONVERT_OK;
}

fconvert_error_t ISOCodec::encode(
    const IsoImage& image,
    std::vector<uint8_t>& data) {

    data = image.data;
    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
