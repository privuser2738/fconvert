/**
 * ZIP archive format implementation
 */

#include "zip.h"
#include "deflate.h"
#include "crc32.h"
#include <cstring>
#include <ctime>

namespace fconvert {
namespace utils {

void ZIP::write_u16(std::vector<uint8_t>& output, uint16_t value) {
    output.push_back(value & 0xFF);
    output.push_back((value >> 8) & 0xFF);
}

void ZIP::write_u32(std::vector<uint8_t>& output, uint32_t value) {
    output.push_back(value & 0xFF);
    output.push_back((value >> 8) & 0xFF);
    output.push_back((value >> 16) & 0xFF);
    output.push_back((value >> 24) & 0xFF);
}

uint16_t ZIP::read_u16(const uint8_t* data) {
    return data[0] | (data[1] << 8);
}

uint32_t ZIP::read_u32(const uint8_t* data) {
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

uint32_t ZIP::dos_time() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);

    uint16_t dos_date = ((tm->tm_year - 80) << 9) | ((tm->tm_mon + 1) << 5) | tm->tm_mday;
    uint16_t dos_time = (tm->tm_hour << 11) | (tm->tm_min << 5) | (tm->tm_sec / 2);

    return (dos_date << 16) | dos_time;
}

bool ZIP::is_zip(const uint8_t* data, size_t size) {
    if (size < 4) return false;
    uint32_t sig = read_u32(data);
    return sig == LOCAL_FILE_HEADER_SIG || sig == END_CENTRAL_DIR_SIG;
}

void ZIP::add_file(
    std::vector<ZIPEntry>& entries,
    const std::string& filename,
    const uint8_t* data,
    size_t size) {

    ZIPEntry entry;
    entry.filename = filename;
    entry.data.assign(data, data + size);
    entry.uncompressed_size = size;
    entry.crc32 = CRC32::calculate(data, size);
    entry.mtime = dos_time();
    entry.compression_method = COMPRESSION_DEFLATE;

    entries.push_back(entry);
}

void ZIP::write_local_header(
    std::vector<uint8_t>& output,
    const ZIPEntry& entry) {

    // Local file header signature
    write_u32(output, LOCAL_FILE_HEADER_SIG);

    // Version needed to extract (2.0)
    write_u16(output, 20);

    // General purpose bit flag
    write_u16(output, 0);

    // Compression method
    write_u16(output, entry.compression_method);

    // File modification time/date
    write_u32(output, entry.mtime);

    // CRC-32
    write_u32(output, entry.crc32);

    // Compressed size
    write_u32(output, entry.compressed_size);

    // Uncompressed size
    write_u32(output, entry.uncompressed_size);

    // Filename length
    write_u16(output, entry.filename.length());

    // Extra field length
    write_u16(output, 0);

    // Filename
    output.insert(output.end(), entry.filename.begin(), entry.filename.end());
}

void ZIP::write_central_directory(
    std::vector<uint8_t>& output,
    const std::vector<ZIPEntry>& entries,
    const std::vector<uint32_t>& local_header_offsets,
    uint32_t central_dir_offset) {

    uint32_t central_dir_size = 0;

    for (size_t i = 0; i < entries.size(); i++) {
        const ZIPEntry& entry = entries[i];
        size_t cd_start = output.size();

        // Central directory header signature
        write_u32(output, CENTRAL_DIR_HEADER_SIG);

        // Version made by (UNIX)
        write_u16(output, 0x031E);

        // Version needed to extract
        write_u16(output, 20);

        // General purpose bit flag
        write_u16(output, 0);

        // Compression method
        write_u16(output, entry.compression_method);

        // File modification time/date
        write_u32(output, entry.mtime);

        // CRC-32
        write_u32(output, entry.crc32);

        // Compressed size
        write_u32(output, entry.compressed_size);

        // Uncompressed size
        write_u32(output, entry.uncompressed_size);

        // Filename length
        write_u16(output, entry.filename.length());

        // Extra field length
        write_u16(output, 0);

        // File comment length
        write_u16(output, 0);

        // Disk number start
        write_u16(output, 0);

        // Internal file attributes
        write_u16(output, 0);

        // External file attributes
        write_u32(output, 0);

        // Relative offset of local header
        write_u32(output, local_header_offsets[i]);

        // Filename
        output.insert(output.end(), entry.filename.begin(), entry.filename.end());

        central_dir_size += (output.size() - cd_start);
    }

    // End of central directory record
    write_u32(output, END_CENTRAL_DIR_SIG);

    // Disk number
    write_u16(output, 0);

    // Disk number with start of central directory
    write_u16(output, 0);

    // Number of central directory records on this disk
    write_u16(output, entries.size());

    // Total number of central directory records
    write_u16(output, entries.size());

    // Size of central directory
    write_u32(output, central_dir_size);

    // Offset of start of central directory
    write_u32(output, central_dir_offset);

    // Comment length
    write_u16(output, 0);
}

fconvert_error_t ZIP::create(
    const std::vector<ZIPEntry>& entries,
    std::vector<uint8_t>& output,
    int compression_level) {

    output.clear();

    std::vector<ZIPEntry> processed_entries = entries;
    std::vector<uint32_t> local_header_offsets;

    // Write local file headers and data
    for (auto& entry : processed_entries) {
        local_header_offsets.push_back(output.size());

        // Compress file data if needed
        std::vector<uint8_t> compressed_data;
        if (entry.compression_method == COMPRESSION_DEFLATE) {
            Deflate deflater;
            fconvert_error_t result = deflater.compress(
                entry.data.data(),
                entry.data.size(),
                compressed_data,
                compression_level);

            if (result != FCONVERT_OK) {
                return result;
            }

            entry.compressed_size = compressed_data.size();
        } else {
            // Stored (no compression)
            compressed_data = entry.data;
            entry.compressed_size = entry.data.size();
        }

        // Write local file header
        write_local_header(output, entry);

        // Write compressed file data
        output.insert(output.end(), compressed_data.begin(), compressed_data.end());
    }

    // Write central directory
    uint32_t central_dir_offset = output.size();
    write_central_directory(output, processed_entries, local_header_offsets, central_dir_offset);

    return FCONVERT_OK;
}

bool ZIP::find_central_directory(
    const uint8_t* data,
    size_t size,
    size_t& central_dir_offset,
    uint16_t& num_entries) {

    // Search for end of central directory signature from the end
    if (size < 22) return false;

    for (size_t i = size - 22; i > 0; i--) {
        if (read_u32(data + i) == END_CENTRAL_DIR_SIG) {
            // Found end of central directory record
            num_entries = read_u16(data + i + 10);
            central_dir_offset = read_u32(data + i + 16);
            return true;
        }
    }

    return false;
}

fconvert_error_t ZIP::extract(
    const uint8_t* zip_data,
    size_t zip_size,
    std::vector<ZIPEntry>& entries) {

    entries.clear();

    // Find central directory
    size_t central_dir_offset;
    uint16_t num_entries;
    if (!find_central_directory(zip_data, zip_size, central_dir_offset, num_entries)) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Read central directory entries
    size_t pos = central_dir_offset;
    for (uint16_t i = 0; i < num_entries; i++) {
        if (pos + 46 > zip_size) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        uint32_t sig = read_u32(zip_data + pos);
        if (sig != CENTRAL_DIR_HEADER_SIG) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        ZIPEntry entry;

        // Read central directory header
        entry.compression_method = read_u16(zip_data + pos + 10);
        entry.mtime = read_u32(zip_data + pos + 12);
        entry.crc32 = read_u32(zip_data + pos + 16);
        entry.compressed_size = read_u32(zip_data + pos + 20);
        entry.uncompressed_size = read_u32(zip_data + pos + 24);
        uint16_t filename_len = read_u16(zip_data + pos + 28);
        uint16_t extra_len = read_u16(zip_data + pos + 30);
        uint16_t comment_len = read_u16(zip_data + pos + 32);
        uint32_t local_header_offset = read_u32(zip_data + pos + 42);

        pos += 46;

        // Read filename
        if (pos + filename_len > zip_size) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }
        entry.filename = std::string(
            reinterpret_cast<const char*>(zip_data + pos),
            filename_len);
        pos += filename_len + extra_len + comment_len;

        // Read file data from local header
        size_t local_pos = local_header_offset;
        if (local_pos + 30 > zip_size) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        uint32_t local_sig = read_u32(zip_data + local_pos);
        if (local_sig != LOCAL_FILE_HEADER_SIG) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        uint16_t local_filename_len = read_u16(zip_data + local_pos + 26);
        uint16_t local_extra_len = read_u16(zip_data + local_pos + 28);

        size_t data_offset = local_pos + 30 + local_filename_len + local_extra_len;

        if (data_offset + entry.compressed_size > zip_size) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        // Decompress or copy file data
        if (entry.compression_method == COMPRESSION_DEFLATE) {
            Inflate inflater;
            fconvert_error_t result = inflater.decompress(
                zip_data + data_offset,
                entry.compressed_size,
                entry.data);

            if (result != FCONVERT_OK) {
                return result;
            }
        } else if (entry.compression_method == COMPRESSION_STORED) {
            entry.data.assign(
                zip_data + data_offset,
                zip_data + data_offset + entry.compressed_size);
        } else {
            // Unsupported compression method
            continue;
        }

        // Verify CRC32
        uint32_t calculated_crc = CRC32::calculate(entry.data.data(), entry.data.size());
        if (calculated_crc != entry.crc32) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        entries.push_back(entry);
    }

    return FCONVERT_OK;
}

} // namespace utils
} // namespace fconvert
