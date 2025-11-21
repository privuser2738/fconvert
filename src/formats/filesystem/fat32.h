/**
 * FAT32 File System
 * Support for reading and writing FAT32 disk images
 */

#ifndef FAT32_H
#define FAT32_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>
#include <map>

namespace fconvert {
namespace formats {

// FAT32 constants
constexpr uint32_t FAT32_BYTES_PER_SECTOR = 512;
constexpr uint32_t FAT32_SECTORS_PER_CLUSTER_DEFAULT = 8;
constexpr uint32_t FAT32_RESERVED_SECTORS = 32;
constexpr uint32_t FAT32_NUM_FATS = 2;
constexpr uint32_t FAT32_ROOT_CLUSTER = 2;

// FAT entry values
constexpr uint32_t FAT32_FREE_CLUSTER = 0x00000000;
constexpr uint32_t FAT32_EOC = 0x0FFFFFF8;      // End of cluster chain
constexpr uint32_t FAT32_BAD_CLUSTER = 0x0FFFFFF7;

// Directory entry attributes
constexpr uint8_t FAT32_ATTR_READ_ONLY = 0x01;
constexpr uint8_t FAT32_ATTR_HIDDEN = 0x02;
constexpr uint8_t FAT32_ATTR_SYSTEM = 0x04;
constexpr uint8_t FAT32_ATTR_VOLUME_ID = 0x08;
constexpr uint8_t FAT32_ATTR_DIRECTORY = 0x10;
constexpr uint8_t FAT32_ATTR_ARCHIVE = 0x20;
constexpr uint8_t FAT32_ATTR_LONG_NAME = 0x0F;

#pragma pack(push, 1)
// BIOS Parameter Block (BPB) and boot sector
struct Fat32BootSector {
    uint8_t jump[3];               // Jump instruction
    char oem_name[8];              // OEM name
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;         // 0 for FAT32
    uint16_t total_sectors_16;     // 0 for FAT32
    uint8_t media_type;
    uint16_t fat_size_16;          // 0 for FAT32
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // FAT32 specific
    uint32_t fat_size_32;          // Sectors per FAT
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;         // Usually 2
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;        // 0x29
    uint32_t volume_serial;
    char volume_label[11];
    char fs_type[8];               // "FAT32   "
    uint8_t boot_code[420];
    uint16_t signature;            // 0xAA55
};

// Directory entry (32 bytes)
struct Fat32DirEntry {
    char name[11];                 // 8.3 filename
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
};

// Long filename entry (32 bytes)
struct Fat32LfnEntry {
    uint8_t order;                 // Sequence number
    uint16_t name1[5];             // First 5 chars
    uint8_t attr;                  // Always 0x0F
    uint8_t type;                  // Always 0
    uint8_t checksum;
    uint16_t name2[6];             // Next 6 chars
    uint16_t first_cluster;        // Always 0
    uint16_t name3[2];             // Last 2 chars
};

// FSInfo sector
struct Fat32FSInfo {
    uint32_t signature1;           // 0x41615252
    uint8_t reserved1[480];
    uint32_t signature2;           // 0x61417272
    uint32_t free_clusters;
    uint32_t next_free;
    uint8_t reserved2[12];
    uint32_t signature3;           // 0xAA550000
};
#pragma pack(pop)

// File entry representation
struct FatFileEntry {
    std::string name;
    std::string path;
    uint32_t first_cluster;
    uint32_t size;
    bool is_directory;
    uint8_t attributes;
    uint16_t create_date;
    uint16_t create_time;
    uint16_t modify_date;
    uint16_t modify_time;
    std::vector<FatFileEntry> children;  // For directories
};

// FAT32 image representation
struct Fat32Image {
    std::string volume_label;
    uint32_t volume_serial;
    uint32_t total_sectors;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_sector;
    uint32_t fat_size;
    uint32_t data_start_sector;
    uint32_t total_clusters;

    FatFileEntry root;
    std::vector<uint32_t> fat;        // File allocation table
    std::vector<uint8_t> data;        // Raw image data
};

/**
 * FAT32 Codec
 */
class FAT32Codec {
public:
    /**
     * Check if data is FAT32 format
     */
    static bool is_fat32(const uint8_t* data, size_t size);

    /**
     * Decode FAT32 image
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        Fat32Image& image);

    /**
     * Encode FAT32 image
     */
    static fconvert_error_t encode(
        const Fat32Image& image,
        std::vector<uint8_t>& data);

    /**
     * Create FAT32 image from directory
     */
    static fconvert_error_t create_from_directory(
        const std::string& source_path,
        Fat32Image& image,
        uint64_t size_bytes = 0,  // Auto-calculate if 0
        const std::string& volume_label = "FCONVERT");

    /**
     * Extract FAT32 image to directory
     */
    static fconvert_error_t extract_to_directory(
        const Fat32Image& image,
        const std::string& dest_path);

    /**
     * List files in FAT32 image
     */
    static std::vector<std::string> list_files(const Fat32Image& image);

    /**
     * Read file from FAT32 image
     */
    static fconvert_error_t read_file(
        const Fat32Image& image,
        const std::string& path,
        std::vector<uint8_t>& file_data);

    /**
     * Write file to FAT32 image
     */
    static fconvert_error_t write_file(
        Fat32Image& image,
        const std::string& path,
        const std::vector<uint8_t>& file_data);

    /**
     * Delete file from FAT32 image
     */
    static fconvert_error_t delete_file(
        Fat32Image& image,
        const std::string& path);

    /**
     * Create directory in FAT32 image
     */
    static fconvert_error_t create_directory(
        Fat32Image& image,
        const std::string& path);

private:
    // Parse directory entries
    static fconvert_error_t parse_directory(
        const Fat32Image& image,
        uint32_t cluster,
        FatFileEntry& dir);

    // Get cluster chain
    static std::vector<uint32_t> get_cluster_chain(
        const Fat32Image& image,
        uint32_t start_cluster);

    // Cluster to sector conversion
    static uint32_t cluster_to_sector(
        const Fat32Image& image,
        uint32_t cluster);

    // Read cluster data
    static void read_cluster(
        const Fat32Image& image,
        uint32_t cluster,
        uint8_t* buffer);

    // Convert 8.3 filename to string
    static std::string decode_83_name(const char* name);

    // Convert string to 8.3 filename
    static void encode_83_name(const std::string& name, char* out);

    // Calculate checksum for LFN
    static uint8_t lfn_checksum(const char* name);

    // Allocate cluster
    static uint32_t allocate_cluster(Fat32Image& image);

    // Free cluster chain
    static void free_cluster_chain(Fat32Image& image, uint32_t start_cluster);

    // Write cluster data
    static void write_cluster(
        Fat32Image& image,
        uint32_t cluster,
        const uint8_t* data);
};

} // namespace formats
} // namespace fconvert

#endif // FAT32_H
