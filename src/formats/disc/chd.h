/**
 * CHD (Compressed Hunks of Data) Format
 * MAME's compressed disc image format
 */

#ifndef CHD_H
#define CHD_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>
#include <array>

namespace fconvert {
namespace formats {

// CHD constants
constexpr char CHD_MAGIC[] = "MComprHD";
constexpr uint32_t CHD_V5_VERSION = 5;
constexpr uint32_t CHD_DEFAULT_HUNK_SIZE = 8 * 2048;  // 8 CD sectors

// CHD compression types
enum class ChdCompression : uint32_t {
    NONE = 0,
    ZLIB = 1,           // CHD_CODEC_ZLIB
    ZLIB_PLUS = 2,      // CHD_CODEC_ZLIB+
    AV = 3,             // A/V codec
    CD_ZLIB = 4,        // CD + zlib
    CD_LZMA = 5,        // CD + LZMA
    CD_FLAC = 6         // CD + FLAC
};

// Map entry types for CHD v5
enum class ChdMapType : uint8_t {
    COMPRESSED = 0,     // Compressed with parent codec
    UNCOMPRESSED = 1,   // Stored uncompressed
    MINI = 2,           // Compressed length <= 8
    SELF_HUNK = 3,      // Another hunk in this file
    PARENT_HUNK = 4,    // Hunk from parent
    SECOND_COMPRESS = 5 // Compressed with secondary codec
};

#pragma pack(push, 1)
// CHD v5 header (124 bytes)
struct ChdV5Header {
    char tag[8];                    // "MComprHD"
    uint32_t header_length;         // Length of header
    uint32_t version;               // CHD version (5)
    uint32_t compressors[4];        // Compression types (FourCC)
    uint64_t logical_bytes;         // Logical size of data
    uint64_t map_offset;            // Offset to map
    uint64_t meta_offset;           // Offset to metadata
    uint32_t hunk_bytes;            // Bytes per hunk
    uint32_t unit_bytes;            // Bytes per unit (CD: 2448)
    uint8_t raw_sha1[20];           // Raw data SHA1
    uint8_t sha1[20];               // Combined SHA1
    uint8_t parent_sha1[20];        // Parent SHA1 (if applicable)
};
#pragma pack(pop)

// Map entry for CHD v5
struct ChdMapEntry {
    ChdMapType type;
    uint32_t length;        // Compressed length
    uint64_t offset;        // Offset in file
    uint16_t crc;           // CRC16 of data
};

// Metadata entry
struct ChdMetadata {
    uint32_t tag;           // FourCC tag
    std::vector<uint8_t> data;
};

// CHD image representation
struct ChdImage {
    uint32_t version;
    uint64_t logical_size;
    uint32_t hunk_size;
    uint32_t unit_size;
    std::array<uint32_t, 4> compressors;

    // Map of hunks
    std::vector<ChdMapEntry> map;

    // Metadata entries
    std::vector<ChdMetadata> metadata;

    // Raw file data (for reading compressed hunks)
    std::vector<uint8_t> raw_file;

    // Decompressed hunks (cached)
    std::vector<std::vector<uint8_t>> hunks;
};

/**
 * CHD Codec
 */
class CHDCodec {
public:
    /**
     * Check if data is CHD format
     */
    static bool is_chd(const uint8_t* data, size_t size);

    /**
     * Decode CHD image
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        ChdImage& image);

    /**
     * Encode CHD image (v5 with zlib)
     */
    static fconvert_error_t encode(
        const ChdImage& image,
        std::vector<uint8_t>& data);

    /**
     * Create CHD from raw data
     */
    static fconvert_error_t create_from_raw(
        const std::vector<uint8_t>& raw_data,
        ChdImage& image,
        uint32_t hunk_size = CHD_DEFAULT_HUNK_SIZE);

    /**
     * Extract raw data from CHD
     */
    static fconvert_error_t extract_raw(
        const ChdImage& image,
        std::vector<uint8_t>& raw_data);

    /**
     * Read a specific hunk
     */
    static fconvert_error_t read_hunk(
        const ChdImage& image,
        uint32_t hunk_num,
        std::vector<uint8_t>& hunk_data);

    /**
     * Get total number of hunks
     */
    static uint32_t get_hunk_count(const ChdImage& image);

    /**
     * Add metadata entry
     */
    static void add_metadata(
        ChdImage& image,
        uint32_t tag,
        const std::vector<uint8_t>& data);

    /**
     * Get metadata by tag
     */
    static const ChdMetadata* get_metadata(
        const ChdImage& image,
        uint32_t tag);

private:
    // Byte swapping for big-endian CHD format
    static uint32_t swap32(uint32_t val);
    static uint64_t swap64(uint64_t val);

    // FourCC helpers
    static uint32_t make_fourcc(const char* str);

    // Decompress hunk data
    static fconvert_error_t decompress_hunk(
        const uint8_t* compressed,
        uint32_t compressed_size,
        uint8_t* output,
        uint32_t output_size,
        uint32_t compressor);

    // Compress hunk data
    static fconvert_error_t compress_hunk(
        const uint8_t* input,
        uint32_t input_size,
        std::vector<uint8_t>& output,
        uint32_t compressor);

    // Calculate CRC16
    static uint16_t crc16(const uint8_t* data, size_t size);
};

} // namespace formats
} // namespace fconvert

#endif // CHD_H
