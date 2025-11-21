/**
 * ext2 File System Reader
 * Read-only support for ext2/ext3/ext4 disk images
 */

#ifndef EXT2_H
#define EXT2_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>
#include <map>

namespace fconvert {
namespace formats {

// ext2 constants
constexpr uint16_t EXT2_SUPER_MAGIC = 0xEF53;
constexpr uint32_t EXT2_SUPERBLOCK_OFFSET = 1024;
constexpr uint32_t EXT2_ROOT_INODE = 2;

// File types in directory entries
constexpr uint8_t EXT2_FT_UNKNOWN = 0;
constexpr uint8_t EXT2_FT_REG_FILE = 1;
constexpr uint8_t EXT2_FT_DIR = 2;
constexpr uint8_t EXT2_FT_CHRDEV = 3;
constexpr uint8_t EXT2_FT_BLKDEV = 4;
constexpr uint8_t EXT2_FT_FIFO = 5;
constexpr uint8_t EXT2_FT_SOCK = 6;
constexpr uint8_t EXT2_FT_SYMLINK = 7;

// Inode flags
constexpr uint16_t EXT2_S_IFMT = 0xF000;
constexpr uint16_t EXT2_S_IFSOCK = 0xC000;
constexpr uint16_t EXT2_S_IFLNK = 0xA000;
constexpr uint16_t EXT2_S_IFREG = 0x8000;
constexpr uint16_t EXT2_S_IFBLK = 0x6000;
constexpr uint16_t EXT2_S_IFDIR = 0x4000;
constexpr uint16_t EXT2_S_IFCHR = 0x2000;
constexpr uint16_t EXT2_S_IFIFO = 0x1000;

#pragma pack(push, 1)
// Superblock
struct Ext2Superblock {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t reserved_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;

    // EXT2_DYNAMIC_REV specific
    uint32_t first_ino;
    uint16_t inode_size;
    uint16_t block_group_nr;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint8_t uuid[16];
    char volume_name[16];
    char last_mounted[64];
    uint32_t algorithm_usage_bitmap;

    // Performance hints
    uint8_t prealloc_blocks;
    uint8_t prealloc_dir_blocks;
    uint16_t padding1;

    // Journaling (ext3)
    uint8_t journal_uuid[16];
    uint32_t journal_inum;
    uint32_t journal_dev;
    uint32_t last_orphan;

    // Directory indexing (ext3)
    uint32_t hash_seed[4];
    uint8_t def_hash_version;
    uint8_t padding2[3];

    // Other options
    uint32_t default_mount_options;
    uint32_t first_meta_bg;
    uint8_t reserved[760];
};

// Block Group Descriptor
struct Ext2GroupDesc {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint8_t reserved[12];
};

// Inode structure
struct Ext2Inode {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[15];     // 12 direct + 1 indirect + 1 double + 1 triple
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;       // size_high for regular files in ext4
    uint32_t faddr;
    uint8_t osd2[12];
};

// Directory entry
struct Ext2DirEntry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    // Variable length name follows
};
#pragma pack(pop)

// File entry representation
struct Ext2FileEntry {
    std::string name;
    std::string path;
    uint32_t inode;
    uint64_t size;
    bool is_directory;
    bool is_symlink;
    uint16_t mode;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
    std::vector<Ext2FileEntry> children;
};

// ext2 image representation
struct Ext2Image {
    std::string volume_name;
    uint32_t block_size;
    uint32_t blocks_count;
    uint32_t inodes_count;
    uint32_t inodes_per_group;
    uint32_t blocks_per_group;
    uint16_t inode_size;

    Ext2FileEntry root;
    std::vector<Ext2GroupDesc> group_descriptors;
    std::vector<uint8_t> data;
};

/**
 * ext2 Codec (read-only)
 */
class Ext2Codec {
public:
    /**
     * Check if data is ext2/ext3/ext4 format
     */
    static bool is_ext2(const uint8_t* data, size_t size);

    /**
     * Decode ext2 image
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        Ext2Image& image);

    /**
     * Extract ext2 image to directory
     */
    static fconvert_error_t extract_to_directory(
        const Ext2Image& image,
        const std::string& dest_path);

    /**
     * List files in ext2 image
     */
    static std::vector<std::string> list_files(const Ext2Image& image);

    /**
     * Read file from ext2 image
     */
    static fconvert_error_t read_file(
        const Ext2Image& image,
        const std::string& path,
        std::vector<uint8_t>& file_data);

    /**
     * Read inode data
     */
    static fconvert_error_t read_inode(
        const Ext2Image& image,
        uint32_t inode_num,
        Ext2Inode& inode);

private:
    // Get block data
    static const uint8_t* get_block(
        const Ext2Image& image,
        uint32_t block);

    // Get inode data blocks
    static std::vector<uint32_t> get_inode_blocks(
        const Ext2Image& image,
        const Ext2Inode& inode);

    // Parse directory entries
    static fconvert_error_t parse_directory(
        const Ext2Image& image,
        uint32_t inode_num,
        Ext2FileEntry& dir);

    // Read indirect block pointers
    static void read_indirect_blocks(
        const Ext2Image& image,
        uint32_t block,
        int level,
        std::vector<uint32_t>& blocks,
        uint32_t& remaining);
};

} // namespace formats
} // namespace fconvert

#endif // EXT2_H
