/**
 * GZIP compression format implementation
 */

#include "gzip.h"
#include "deflate.h"
#include "crc32.h"
#include <cstring>
#include <ctime>

namespace fconvert {
namespace utils {

bool GZIP::is_gzip(const uint8_t* data, size_t size) {
    if (size < 10) return false;
    return data[0] == GZIP_MAGIC1 && data[1] == GZIP_MAGIC2;
}

void GZIP::write_header(
    std::vector<uint8_t>& output,
    const std::string& filename,
    uint32_t mtime) {

    // Magic number
    output.push_back(GZIP_MAGIC1);
    output.push_back(GZIP_MAGIC2);

    // Compression method (DEFLATE)
    output.push_back(GZIP_METHOD_DEFLATE);

    // Flags
    uint8_t flags = 0;
    if (!filename.empty()) {
        flags |= FNAME;
    }
    output.push_back(flags);

    // Modification time (little-endian)
    output.push_back(mtime & 0xFF);
    output.push_back((mtime >> 8) & 0xFF);
    output.push_back((mtime >> 16) & 0xFF);
    output.push_back((mtime >> 24) & 0xFF);

    // Extra flags (2 = maximum compression)
    output.push_back(0x02);

    // OS (255 = unknown)
    output.push_back(0xFF);

    // Original filename (if present)
    if (!filename.empty()) {
        for (char c : filename) {
            output.push_back(c);
        }
        output.push_back(0); // Null terminator
    }
}

bool GZIP::read_header(
    const uint8_t* data,
    size_t size,
    size_t& header_size,
    std::string* filename) {

    if (size < 10) return false;

    // Check magic number
    if (data[0] != GZIP_MAGIC1 || data[1] != GZIP_MAGIC2) {
        return false;
    }

    // Check compression method
    if (data[2] != GZIP_METHOD_DEFLATE) {
        return false;
    }

    uint8_t flags = data[3];
    size_t pos = 10;

    // Skip extra fields if present
    if (flags & FEXTRA) {
        if (pos + 2 > size) return false;
        uint16_t xlen = data[pos] | (data[pos + 1] << 8);
        pos += 2 + xlen;
    }

    // Read filename if present
    if (flags & FNAME) {
        if (filename) {
            filename->clear();
            while (pos < size && data[pos] != 0) {
                filename->push_back(data[pos++]);
            }
        } else {
            while (pos < size && data[pos] != 0) {
                pos++;
            }
        }
        if (pos >= size) return false;
        pos++; // Skip null terminator
    }

    // Skip comment if present
    if (flags & FCOMMENT) {
        while (pos < size && data[pos] != 0) {
            pos++;
        }
        if (pos >= size) return false;
        pos++; // Skip null terminator
    }

    // Skip header CRC if present
    if (flags & FHCRC) {
        pos += 2;
    }

    if (pos >= size) return false;

    header_size = pos;
    return true;
}

fconvert_error_t GZIP::compress(
    const uint8_t* input_data,
    size_t input_size,
    std::vector<uint8_t>& output,
    int level,
    const std::string& filename) {

    output.clear();

    // Write GZIP header
    uint32_t mtime = (uint32_t)std::time(nullptr);
    write_header(output, filename, mtime);

    // Compress data with DEFLATE
    std::vector<uint8_t> compressed;
    Deflate deflater;
    fconvert_error_t result = deflater.compress(input_data, input_size, compressed, level);
    if (result != FCONVERT_OK) {
        return result;
    }

    // Append compressed data
    output.insert(output.end(), compressed.begin(), compressed.end());

    // Calculate CRC32 of uncompressed data
    uint32_t crc = CRC32::calculate(input_data, input_size);

    // Write footer (CRC32 + uncompressed size, both little-endian)
    output.push_back(crc & 0xFF);
    output.push_back((crc >> 8) & 0xFF);
    output.push_back((crc >> 16) & 0xFF);
    output.push_back((crc >> 24) & 0xFF);

    uint32_t isize = input_size & 0xFFFFFFFF;
    output.push_back(isize & 0xFF);
    output.push_back((isize >> 8) & 0xFF);
    output.push_back((isize >> 16) & 0xFF);
    output.push_back((isize >> 24) & 0xFF);

    return FCONVERT_OK;
}

fconvert_error_t GZIP::decompress(
    const uint8_t* compressed_data,
    size_t compressed_size,
    std::vector<uint8_t>& output,
    std::string* original_filename) {

    output.clear();

    // Read and validate header
    size_t header_size = 0;
    if (!read_header(compressed_data, compressed_size, header_size, original_filename)) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Check footer size
    if (compressed_size < header_size + 8) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    // Get compressed data (between header and 8-byte footer)
    const uint8_t* deflate_data = compressed_data + header_size;
    size_t deflate_size = compressed_size - header_size - 8;

    // Decompress DEFLATE data
    Inflate inflater;
    fconvert_error_t result = inflater.decompress(deflate_data, deflate_size, output);
    if (result != FCONVERT_OK) {
        return result;
    }

    // Read footer (CRC32 + uncompressed size)
    const uint8_t* footer = compressed_data + compressed_size - 8;
    uint32_t stored_crc = footer[0] | (footer[1] << 8) | (footer[2] << 16) | (footer[3] << 24);
    uint32_t stored_size = footer[4] | (footer[5] << 8) | (footer[6] << 16) | (footer[7] << 24);

    // Verify CRC32
    uint32_t calculated_crc = CRC32::calculate(output.data(), output.size());
    if (calculated_crc != stored_crc) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    // Verify size (only lower 32 bits)
    if ((output.size() & 0xFFFFFFFF) != stored_size) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    return FCONVERT_OK;
}

} // namespace utils
} // namespace fconvert
