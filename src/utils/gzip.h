/**
 * GZIP compression format (RFC 1952)
 * GZIP is a file format wrapper around DEFLATE compression
 */

#ifndef GZIP_H
#define GZIP_H

#include "../../include/fconvert.h"
#include <vector>
#include <cstdint>
#include <string>

namespace fconvert {
namespace utils {

// GZIP flags
enum GZIPFlags {
    FTEXT = 0x01,    // File is ASCII text
    FHCRC = 0x02,    // Header CRC16 present
    FEXTRA = 0x04,   // Extra fields present
    FNAME = 0x08,    // Original filename present
    FCOMMENT = 0x10  // Comment present
};

// GZIP compression class
class GZIP {
public:
    /**
     * Compress data to GZIP format
     * @param input_data Uncompressed data
     * @param input_size Size of input data
     * @param output Output vector for compressed data
     * @param level Compression level (0-9)
     * @param filename Optional original filename to store
     * @return Error code
     */
    static fconvert_error_t compress(
        const uint8_t* input_data,
        size_t input_size,
        std::vector<uint8_t>& output,
        int level = 6,
        const std::string& filename = "");

    /**
     * Decompress GZIP format data
     * @param compressed_data GZIP compressed data
     * @param compressed_size Size of compressed data
     * @param output Output vector for decompressed data
     * @param original_filename If not null, receives original filename
     * @return Error code
     */
    static fconvert_error_t decompress(
        const uint8_t* compressed_data,
        size_t compressed_size,
        std::vector<uint8_t>& output,
        std::string* original_filename = nullptr);

    /**
     * Check if data is GZIP format
     */
    static bool is_gzip(const uint8_t* data, size_t size);

private:
    static const uint8_t GZIP_MAGIC1 = 0x1f;
    static const uint8_t GZIP_MAGIC2 = 0x8b;
    static const uint8_t GZIP_METHOD_DEFLATE = 0x08;

    static void write_header(
        std::vector<uint8_t>& output,
        const std::string& filename,
        uint32_t mtime);

    static bool read_header(
        const uint8_t* data,
        size_t size,
        size_t& header_size,
        std::string* filename);
};

} // namespace utils
} // namespace fconvert

#endif // GZIP_H
