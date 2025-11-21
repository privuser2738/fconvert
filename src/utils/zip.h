/**
 * ZIP archive format (PKZIP)
 * Compressed archive format using DEFLATE
 */

#ifndef ZIP_H
#define ZIP_H

#include "../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>

namespace fconvert {
namespace utils {

// ZIP file entry
struct ZIPEntry {
    std::string filename;
    std::vector<uint8_t> data;  // Uncompressed data
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t compression_method;  // 0 = stored, 8 = deflate
    uint32_t mtime;  // DOS time format
};

// ZIP archive handler
class ZIP {
public:
    /**
     * Create ZIP archive from files
     * @param entries List of files to add
     * @param output Output ZIP data
     * @param compression_level Compression level (0-9)
     * @return Error code
     */
    static fconvert_error_t create(
        const std::vector<ZIPEntry>& entries,
        std::vector<uint8_t>& output,
        int compression_level = 6);

    /**
     * Extract ZIP archive
     * @param zip_data ZIP archive data
     * @param zip_size Size of ZIP data
     * @param entries Output list of extracted files
     * @return Error code
     */
    static fconvert_error_t extract(
        const uint8_t* zip_data,
        size_t zip_size,
        std::vector<ZIPEntry>& entries);

    /**
     * Check if data is ZIP format
     */
    static bool is_zip(const uint8_t* data, size_t size);

    /**
     * Add single file to ZIP (convenience method)
     */
    static void add_file(
        std::vector<ZIPEntry>& entries,
        const std::string& filename,
        const uint8_t* data,
        size_t size);

    /**
     * Get current DOS time
     */
    static uint32_t dos_time();

private:
    static const uint32_t LOCAL_FILE_HEADER_SIG = 0x04034b50;
    static const uint32_t CENTRAL_DIR_HEADER_SIG = 0x02014b50;
    static const uint32_t END_CENTRAL_DIR_SIG = 0x06054b50;

    static const uint16_t COMPRESSION_STORED = 0;
    static const uint16_t COMPRESSION_DEFLATE = 8;

    static void write_u16(std::vector<uint8_t>& output, uint16_t value);
    static void write_u32(std::vector<uint8_t>& output, uint32_t value);
    static uint16_t read_u16(const uint8_t* data);
    static uint32_t read_u32(const uint8_t* data);

    static void write_local_header(
        std::vector<uint8_t>& output,
        const ZIPEntry& entry);

    static void write_central_directory(
        std::vector<uint8_t>& output,
        const std::vector<ZIPEntry>& entries,
        const std::vector<uint32_t>& local_header_offsets,
        uint32_t central_dir_offset);

    static bool find_central_directory(
        const uint8_t* data,
        size_t size,
        size_t& central_dir_offset,
        uint16_t& num_entries);
};

} // namespace utils
} // namespace fconvert

#endif // ZIP_H
