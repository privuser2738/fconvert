/**
 * ext2 File System Reader Implementation
 */

#include "ext2.h"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <functional>

namespace fconvert {
namespace formats {

namespace fs = std::filesystem;

bool Ext2Codec::is_ext2(const uint8_t* data, size_t size) {
    if (size < EXT2_SUPERBLOCK_OFFSET + sizeof(Ext2Superblock)) {
        return false;
    }

    const Ext2Superblock* sb = reinterpret_cast<const Ext2Superblock*>(
        data + EXT2_SUPERBLOCK_OFFSET);

    return sb->magic == EXT2_SUPER_MAGIC;
}

const uint8_t* Ext2Codec::get_block(const Ext2Image& image, uint32_t block) {
    if (block == 0) return nullptr;
    uint64_t offset = static_cast<uint64_t>(block) * image.block_size;
    if (offset + image.block_size > image.data.size()) {
        return nullptr;
    }
    return image.data.data() + offset;
}

fconvert_error_t Ext2Codec::read_inode(
    const Ext2Image& image,
    uint32_t inode_num,
    Ext2Inode& inode) {

    if (inode_num == 0 || inode_num > image.inodes_count) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    // Calculate block group and index
    uint32_t group = (inode_num - 1) / image.inodes_per_group;
    uint32_t index = (inode_num - 1) % image.inodes_per_group;

    if (group >= image.group_descriptors.size()) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Get inode table block
    uint32_t inode_table = image.group_descriptors[group].inode_table;
    uint32_t inodes_per_block = image.block_size / image.inode_size;
    uint32_t block_offset = index / inodes_per_block;
    uint32_t inode_offset = (index % inodes_per_block) * image.inode_size;

    const uint8_t* block_data = get_block(image, inode_table + block_offset);
    if (!block_data) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    std::memcpy(&inode, block_data + inode_offset, sizeof(Ext2Inode));
    return FCONVERT_OK;
}

void Ext2Codec::read_indirect_blocks(
    const Ext2Image& image,
    uint32_t block,
    int level,
    std::vector<uint32_t>& blocks,
    uint32_t& remaining) {

    if (block == 0 || remaining == 0) return;

    const uint8_t* block_data = get_block(image, block);
    if (!block_data) return;

    uint32_t ptrs_per_block = image.block_size / 4;
    const uint32_t* ptrs = reinterpret_cast<const uint32_t*>(block_data);

    for (uint32_t i = 0; i < ptrs_per_block && remaining > 0; i++) {
        if (ptrs[i] == 0) continue;

        if (level == 1) {
            blocks.push_back(ptrs[i]);
            remaining--;
        } else {
            read_indirect_blocks(image, ptrs[i], level - 1, blocks, remaining);
        }
    }
}

std::vector<uint32_t> Ext2Codec::get_inode_blocks(
    const Ext2Image& image,
    const Ext2Inode& inode) {

    std::vector<uint32_t> blocks;
    uint32_t total_blocks = (inode.size + image.block_size - 1) / image.block_size;
    uint32_t remaining = total_blocks;

    // Direct blocks (0-11)
    for (int i = 0; i < 12 && remaining > 0; i++) {
        if (inode.block[i] != 0) {
            blocks.push_back(inode.block[i]);
            remaining--;
        }
    }

    // Single indirect (12)
    if (remaining > 0 && inode.block[12] != 0) {
        read_indirect_blocks(image, inode.block[12], 1, blocks, remaining);
    }

    // Double indirect (13)
    if (remaining > 0 && inode.block[13] != 0) {
        read_indirect_blocks(image, inode.block[13], 2, blocks, remaining);
    }

    // Triple indirect (14)
    if (remaining > 0 && inode.block[14] != 0) {
        read_indirect_blocks(image, inode.block[14], 3, blocks, remaining);
    }

    return blocks;
}

fconvert_error_t Ext2Codec::parse_directory(
    const Ext2Image& image,
    uint32_t inode_num,
    Ext2FileEntry& dir) {

    Ext2Inode inode;
    fconvert_error_t result = read_inode(image, inode_num, inode);
    if (result != FCONVERT_OK) return result;

    std::vector<uint32_t> blocks = get_inode_blocks(image, inode);

    for (uint32_t block : blocks) {
        const uint8_t* block_data = get_block(image, block);
        if (!block_data) continue;

        uint32_t offset = 0;
        while (offset < image.block_size) {
            const Ext2DirEntry* entry = reinterpret_cast<const Ext2DirEntry*>(
                block_data + offset);

            if (entry->rec_len == 0) break;
            if (entry->inode == 0) {
                offset += entry->rec_len;
                continue;
            }

            std::string name(reinterpret_cast<const char*>(entry) + 8, entry->name_len);

            // Skip . and ..
            if (name == "." || name == "..") {
                offset += entry->rec_len;
                continue;
            }

            Ext2FileEntry file;
            file.name = name;
            file.path = dir.path.empty() ? name : dir.path + "/" + name;
            file.inode = entry->inode;
            file.is_directory = (entry->file_type == EXT2_FT_DIR);
            file.is_symlink = (entry->file_type == EXT2_FT_SYMLINK);

            // Read inode for size and other metadata
            Ext2Inode file_inode;
            if (read_inode(image, entry->inode, file_inode) == FCONVERT_OK) {
                file.size = file_inode.size;
                file.mode = file_inode.mode;
                file.atime = file_inode.atime;
                file.mtime = file_inode.mtime;
                file.ctime = file_inode.ctime;

                // Check mode for directory type
                if ((file_inode.mode & EXT2_S_IFMT) == EXT2_S_IFDIR) {
                    file.is_directory = true;
                }
                if ((file_inode.mode & EXT2_S_IFMT) == EXT2_S_IFLNK) {
                    file.is_symlink = true;
                }
            }

            // Recursively parse subdirectories
            if (file.is_directory) {
                parse_directory(image, file.inode, file);
            }

            dir.children.push_back(file);
            offset += entry->rec_len;
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t Ext2Codec::decode(
    const std::vector<uint8_t>& data,
    Ext2Image& image) {

    if (!is_ext2(data.data(), data.size())) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    const Ext2Superblock* sb = reinterpret_cast<const Ext2Superblock*>(
        data.data() + EXT2_SUPERBLOCK_OFFSET);

    // Basic superblock info
    image.block_size = 1024 << sb->log_block_size;
    image.blocks_count = sb->blocks_count;
    image.inodes_count = sb->inodes_count;
    image.inodes_per_group = sb->inodes_per_group;
    image.blocks_per_group = sb->blocks_per_group;
    image.inode_size = (sb->rev_level >= 1) ? sb->inode_size : 128;
    image.volume_name = std::string(sb->volume_name, 16);
    image.volume_name.erase(image.volume_name.find_last_not_of('\0') + 1);

    // Store raw data
    image.data = data;

    // Calculate number of block groups
    uint32_t num_groups = (sb->blocks_count + sb->blocks_per_group - 1) / sb->blocks_per_group;

    // Read group descriptors
    uint32_t gd_block = (image.block_size == 1024) ? 2 : 1;
    const uint8_t* gd_data = get_block(image, gd_block);
    if (!gd_data) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    image.group_descriptors.resize(num_groups);
    const Ext2GroupDesc* gd = reinterpret_cast<const Ext2GroupDesc*>(gd_data);
    for (uint32_t i = 0; i < num_groups; i++) {
        image.group_descriptors[i] = gd[i];
    }

    // Parse root directory
    image.root.name = "";
    image.root.path = "";
    image.root.inode = EXT2_ROOT_INODE;
    image.root.is_directory = true;

    return parse_directory(image, EXT2_ROOT_INODE, image.root);
}

std::vector<std::string> Ext2Codec::list_files(const Ext2Image& image) {
    std::vector<std::string> files;

    std::function<void(const Ext2FileEntry&)> collect = [&](const Ext2FileEntry& entry) {
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

fconvert_error_t Ext2Codec::read_file(
    const Ext2Image& image,
    const std::string& path,
    std::vector<uint8_t>& file_data) {

    // Find file
    std::function<const Ext2FileEntry*(const Ext2FileEntry&, const std::string&)> find_file;
    find_file = [&](const Ext2FileEntry& entry, const std::string& target) -> const Ext2FileEntry* {
        if (entry.path == target) {
            return &entry;
        }
        for (const auto& child : entry.children) {
            const Ext2FileEntry* result = find_file(child, target);
            if (result) return result;
        }
        return nullptr;
    };

    const Ext2FileEntry* file = find_file(image.root, path);
    if (!file || file->is_directory) {
        return FCONVERT_ERROR_FILE_NOT_FOUND;
    }

    // Read inode
    Ext2Inode inode;
    fconvert_error_t result = read_inode(image, file->inode, inode);
    if (result != FCONVERT_OK) return result;

    // Read file data
    std::vector<uint32_t> blocks = get_inode_blocks(image, inode);
    file_data.resize(inode.size);

    uint64_t remaining = inode.size;
    uint64_t offset = 0;

    for (uint32_t block : blocks) {
        const uint8_t* block_data = get_block(image, block);
        if (!block_data) break;

        uint32_t copy_size = static_cast<uint32_t>(std::min(
            static_cast<uint64_t>(image.block_size), remaining));
        std::memcpy(file_data.data() + offset, block_data, copy_size);
        offset += copy_size;
        remaining -= copy_size;
    }

    return FCONVERT_OK;
}

fconvert_error_t Ext2Codec::extract_to_directory(
    const Ext2Image& image,
    const std::string& dest_path) {

    std::function<fconvert_error_t(const Ext2FileEntry&, const std::string&)> extract;
    extract = [&](const Ext2FileEntry& entry, const std::string& base_path) -> fconvert_error_t {
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
        } else if (!entry.is_symlink) {
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

} // namespace formats
} // namespace fconvert
