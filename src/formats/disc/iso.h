/**
 * ISO 9660 Disc Image Format
 * Standard format for CD/DVD/BD disc images
 */

#ifndef ISO_H
#define ISO_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>
#include <map>

namespace fconvert {
namespace formats {

// ISO 9660 constants
constexpr uint32_t ISO_SECTOR_SIZE = 2048;
constexpr uint32_t ISO_SYSTEM_AREA_SIZE = 32768;  // 16 sectors
constexpr uint8_t ISO_VD_PRIMARY = 1;
constexpr uint8_t ISO_VD_TERMINATOR = 255;

#pragma pack(push, 1)
// Date/time format in directory records
struct IsoDateTime {
    uint8_t years_since_1900;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int8_t gmt_offset;  // In 15-minute intervals
};

// Extended date/time for volume descriptors
struct IsoDateTimeExt {
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char hundredths[2];
    int8_t gmt_offset;
};

// Volume Descriptor
struct IsoVolumeDescriptor {
    uint8_t type;
    char identifier[5];         // "CD001"
    uint8_t version;
    uint8_t unused1;
    char system_id[32];
    char volume_id[32];
    uint8_t unused2[8];
    uint32_t volume_space_size_le;
    uint32_t volume_space_size_be;
    uint8_t unused3[32];
    uint16_t volume_set_size_le;
    uint16_t volume_set_size_be;
    uint16_t volume_seq_number_le;
    uint16_t volume_seq_number_be;
    uint16_t logical_block_size_le;
    uint16_t logical_block_size_be;
    uint32_t path_table_size_le;
    uint32_t path_table_size_be;
    uint32_t path_table_loc_le;
    uint32_t path_table_opt_loc_le;
    uint32_t path_table_loc_be;
    uint32_t path_table_opt_loc_be;
    uint8_t root_dir_record[34];
    char volume_set_id[128];
    char publisher_id[128];
    char preparer_id[128];
    char application_id[128];
    char copyright_file[37];
    char abstract_file[37];
    char biblio_file[37];
    IsoDateTimeExt creation_date;
    IsoDateTimeExt modification_date;
    IsoDateTimeExt expiration_date;
    IsoDateTimeExt effective_date;
    uint8_t file_structure_version;
    uint8_t unused4;
    uint8_t application_use[512];
    uint8_t reserved[653];
};

// Directory Record
struct IsoDirRecord {
    uint8_t length;
    uint8_t ext_attr_length;
    uint32_t extent_location_le;
    uint32_t extent_location_be;
    uint32_t data_length_le;
    uint32_t data_length_be;
    IsoDateTime recording_date;
    uint8_t flags;
    uint8_t unit_size;
    uint8_t interleave_gap;
    uint16_t volume_seq_le;
    uint16_t volume_seq_be;
    uint8_t name_length;
    // Variable length name follows
};
#pragma pack(pop)

// Directory entry flags
constexpr uint8_t ISO_FLAG_HIDDEN = 0x01;
constexpr uint8_t ISO_FLAG_DIRECTORY = 0x02;
constexpr uint8_t ISO_FLAG_ASSOCIATED = 0x04;
constexpr uint8_t ISO_FLAG_RECORD = 0x08;
constexpr uint8_t ISO_FLAG_PROTECTION = 0x10;
constexpr uint8_t ISO_FLAG_MULTIEXTENT = 0x80;

// File entry in our representation
struct IsoFileEntry {
    std::string name;
    std::string path;
    uint32_t location;      // Sector number
    uint32_t size;
    bool is_directory;
    IsoDateTime date;
    std::vector<IsoFileEntry> children;  // For directories
};

// ISO image data
struct IsoImage {
    std::string volume_id;
    std::string system_id;
    std::string publisher_id;
    std::string application_id;
    uint32_t sector_count;
    IsoFileEntry root;
    std::vector<uint8_t> data;  // Raw image data
};

/**
 * ISO 9660 codec
 */
class ISOCodec {
public:
    /**
     * Decode ISO image
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        IsoImage& image);

    /**
     * Encode ISO image
     */
    static fconvert_error_t encode(
        const IsoImage& image,
        std::vector<uint8_t>& data);

    /**
     * Create ISO from directory structure
     */
    static fconvert_error_t create_from_directory(
        const std::string& source_path,
        IsoImage& image,
        const std::string& volume_id = "CDROM");

    /**
     * Extract ISO to directory
     */
    static fconvert_error_t extract_to_directory(
        const IsoImage& image,
        const std::string& dest_path);

    /**
     * Check if data is ISO format
     */
    static bool is_iso(const uint8_t* data, size_t size);

    /**
     * List files in ISO
     */
    static std::vector<std::string> list_files(const IsoImage& image);

    /**
     * Read file from ISO
     */
    static fconvert_error_t read_file(
        const IsoImage& image,
        const std::string& path,
        std::vector<uint8_t>& file_data);

private:
    // Parse directory records
    static fconvert_error_t parse_directory(
        const uint8_t* data,
        size_t data_size,
        uint32_t location,
        uint32_t length,
        IsoFileEntry& dir);

    // Write directory records
    static void write_directory(
        std::vector<uint8_t>& data,
        const IsoFileEntry& dir,
        uint32_t parent_location);

    // Convert ISO filename to readable name
    static std::string decode_filename(const char* name, uint8_t length);

    // Convert filename to ISO format
    static std::string encode_filename(const std::string& name, bool is_dir);

    // Calculate required sectors for directory
    static uint32_t calculate_dir_size(const IsoFileEntry& dir);
};

} // namespace formats
} // namespace fconvert

#endif // ISO_H
