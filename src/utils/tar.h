/**
 * TAR archive format (POSIX.1-1988 / ustar)
 * Simple archive format with headers followed by file data
 */

#ifndef TAR_H
#define TAR_H

#include "../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>

namespace fconvert {
namespace utils {

// TAR file entry
struct TAREntry {
    std::string filename;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t mtime;
    char typeflag;  // '0' = regular file, '5' = directory
    std::vector<uint8_t> data;
};

// TAR archive handler
class TAR {
public:
    /**
     * Create TAR archive from files
     * @param entries List of files to add
     * @param output Output TAR data
     * @return Error code
     */
    static fconvert_error_t create(
        const std::vector<TAREntry>& entries,
        std::vector<uint8_t>& output);

    /**
     * Extract TAR archive
     * @param tar_data TAR archive data
     * @param tar_size Size of TAR data
     * @param entries Output list of extracted files
     * @return Error code
     */
    static fconvert_error_t extract(
        const uint8_t* tar_data,
        size_t tar_size,
        std::vector<TAREntry>& entries);

    /**
     * Check if data is TAR format
     */
    static bool is_tar(const uint8_t* data, size_t size);

    /**
     * Add single file to TAR (convenience method)
     */
    static void add_file(
        std::vector<TAREntry>& entries,
        const std::string& filename,
        const uint8_t* data,
        size_t size);

private:
    // TAR header structure (512 bytes)
    struct TARHeader {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char checksum[8];
        char typeflag;
        char linkname[100];
        char magic[6];     // "ustar\0"
        char version[2];   // "00"
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];
        char padding[12];
    };

    static void write_header(
        std::vector<uint8_t>& output,
        const TAREntry& entry);

    static bool read_header(
        const uint8_t* data,
        size_t size,
        TAREntry& entry);

    static uint32_t calculate_checksum(const TARHeader& header);
    static void write_octal(char* dest, size_t dest_size, uint64_t value);
    static uint64_t read_octal(const char* src, size_t src_size);
};

} // namespace utils
} // namespace fconvert

#endif // TAR_H
