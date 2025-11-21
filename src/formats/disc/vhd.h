/**
 * VHD (Virtual Hard Disk) Format
 * Microsoft virtual disk format for Hyper-V and Virtual PC
 */

#ifndef VHD_H
#define VHD_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>
#include <array>

namespace fconvert {
namespace formats {

// VHD Constants
constexpr uint64_t VHD_COOKIE = 0x636F6E6563746978ULL;  // "conectix"
constexpr uint32_t VHD_VERSION = 0x00010000;
constexpr uint32_t VHD_SECTOR_SIZE = 512;
constexpr uint32_t VHD_DEFAULT_BLOCK_SIZE = 2 * 1024 * 1024;  // 2MB

// Disk types
enum class VhdDiskType : uint32_t {
    None = 0,
    Reserved_Deprecated1 = 1,
    Fixed = 2,
    Dynamic = 3,
    Differencing = 4,
    Reserved_Deprecated2 = 5,
    Reserved_Deprecated3 = 6
};

#pragma pack(push, 1)
// VHD Footer (512 bytes at end of file)
struct VhdFooter {
    uint64_t cookie;            // "conectix"
    uint32_t features;          // Reserved, must be 0x00000002
    uint32_t file_format_version;
    uint64_t data_offset;       // Offset to next structure (dynamic header)
    uint32_t timestamp;         // Seconds since Jan 1, 2000 12:00:00 UTC
    uint32_t creator_app;       // Creator application
    uint32_t creator_version;
    uint32_t creator_host_os;   // 0x5769326B = "Wi2k" (Windows)
    uint64_t original_size;     // Original disk size in bytes
    uint64_t current_size;      // Current disk size in bytes
    uint32_t disk_geometry;     // CHS geometry
    uint32_t disk_type;         // Fixed, Dynamic, Differencing
    uint32_t checksum;          // One's complement checksum
    uint8_t unique_id[16];      // UUID
    uint8_t saved_state;        // 1 if VM was saved
    uint8_t reserved[427];
};

// Dynamic Disk Header (1024 bytes)
struct VhdDynamicHeader {
    uint64_t cookie;            // "cxsparse"
    uint64_t data_offset;       // Currently unused, 0xFFFFFFFF
    uint64_t table_offset;      // Absolute offset to BAT
    uint32_t header_version;
    uint32_t max_table_entries; // Number of entries in BAT
    uint32_t block_size;        // Size of each block
    uint32_t checksum;
    uint8_t parent_unique_id[16];
    uint32_t parent_timestamp;
    uint32_t reserved1;
    uint8_t parent_unicode_name[512];
    uint8_t parent_locator_entries[192];  // 8 entries × 24 bytes
    uint8_t reserved2[256];
};
#pragma pack(pop)

// Disk geometry
struct VhdGeometry {
    uint16_t cylinders;
    uint8_t heads;
    uint8_t sectors_per_track;
};

// VHD Image representation
struct VhdImage {
    VhdDiskType type;
    uint64_t disk_size;
    VhdGeometry geometry;
    uint32_t block_size;
    std::array<uint8_t, 16> unique_id;
    std::string creator_app;

    // Raw disk data (for fixed) or block data (for dynamic)
    std::vector<uint8_t> data;

    // Block allocation table (for dynamic disks)
    std::vector<uint32_t> bat;

    // Block bitmap + data for dynamic disks
    std::vector<std::vector<uint8_t>> blocks;
};

/**
 * VHD Codec
 */
class VHDCodec {
public:
    /**
     * Check if data is VHD format
     */
    static bool is_vhd(const uint8_t* data, size_t size);

    /**
     * Decode VHD image
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        VhdImage& image);

    /**
     * Encode VHD image (fixed type)
     */
    static fconvert_error_t encode_fixed(
        const VhdImage& image,
        std::vector<uint8_t>& data);

    /**
     * Encode VHD image (dynamic type)
     */
    static fconvert_error_t encode_dynamic(
        const VhdImage& image,
        std::vector<uint8_t>& data);

    /**
     * Create VHD from raw disk data
     */
    static fconvert_error_t create_from_raw(
        const std::vector<uint8_t>& raw_data,
        VhdImage& image,
        VhdDiskType type = VhdDiskType::Dynamic);

    /**
     * Extract raw disk data from VHD
     */
    static fconvert_error_t extract_raw(
        const VhdImage& image,
        std::vector<uint8_t>& raw_data);

    /**
     * Read sector from VHD
     */
    static fconvert_error_t read_sector(
        const VhdImage& image,
        uint64_t sector,
        uint8_t* buffer);

    /**
     * Calculate CHS geometry from disk size
     */
    static VhdGeometry calculate_geometry(uint64_t disk_size);

private:
    // Calculate footer checksum
    static uint32_t calculate_checksum(const VhdFooter& footer);
    static uint32_t calculate_checksum(const VhdDynamicHeader& header);

    // Byte swapping for big-endian fields
    static uint16_t swap16(uint16_t val);
    static uint32_t swap32(uint32_t val);
    static uint64_t swap64(uint64_t val);

    // Generate UUID
    static void generate_uuid(uint8_t* uuid);
};

} // namespace formats
} // namespace fconvert

#endif // VHD_H
