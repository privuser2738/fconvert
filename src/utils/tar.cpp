/**
 * TAR archive format implementation
 */

#include "tar.h"
#include <cstring>
#include <ctime>
#include <algorithm>

namespace fconvert {
namespace utils {

void TAR::write_octal(char* dest, size_t dest_size, uint64_t value) {
    // Write octal number (null-terminated)
    std::memset(dest, '0', dest_size);
    dest[dest_size - 1] = '\0';

    size_t pos = dest_size - 2;
    while (value > 0 && pos > 0) {
        dest[pos--] = '0' + (value & 7);
        value >>= 3;
    }
}

uint64_t TAR::read_octal(const char* src, size_t src_size) {
    uint64_t value = 0;
    for (size_t i = 0; i < src_size && src[i] != '\0' && src[i] != ' '; i++) {
        if (src[i] >= '0' && src[i] <= '7') {
            value = (value << 3) | (src[i] - '0');
        }
    }
    return value;
}

uint32_t TAR::calculate_checksum(const TARHeader& header) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&header);
    uint32_t sum = 0;

    // Calculate sum treating checksum field as spaces
    for (size_t i = 0; i < sizeof(TARHeader); i++) {
        if (i >= 148 && i < 156) {
            sum += ' '; // Checksum field treated as spaces
        } else {
            sum += data[i];
        }
    }

    return sum;
}

void TAR::write_header(
    std::vector<uint8_t>& output,
    const TAREntry& entry) {

    TARHeader header;
    std::memset(&header, 0, sizeof(TARHeader));

    // Filename (truncate if too long)
    size_t name_len = std::min(entry.filename.length(), sizeof(header.name) - 1);
    std::memcpy(header.name, entry.filename.c_str(), name_len);

    // Mode (permissions)
    write_octal(header.mode, sizeof(header.mode), entry.mode);

    // UID/GID
    write_octal(header.uid, sizeof(header.uid), entry.uid);
    write_octal(header.gid, sizeof(header.gid), entry.gid);

    // File size
    write_octal(header.size, sizeof(header.size), entry.size);

    // Modification time
    write_octal(header.mtime, sizeof(header.mtime), entry.mtime);

    // Type flag
    header.typeflag = entry.typeflag;

    // ustar magic
    std::memcpy(header.magic, "ustar", 5);
    header.magic[5] = '\0';
    header.version[0] = '0';
    header.version[1] = '0';

    // Calculate checksum
    uint32_t checksum = calculate_checksum(header);
    write_octal(header.checksum, sizeof(header.checksum) - 1, checksum);
    header.checksum[sizeof(header.checksum) - 1] = ' ';

    // Write header
    const uint8_t* header_data = reinterpret_cast<const uint8_t*>(&header);
    output.insert(output.end(), header_data, header_data + sizeof(TARHeader));
}

bool TAR::read_header(
    const uint8_t* data,
    size_t size,
    TAREntry& entry) {

    if (size < sizeof(TARHeader)) {
        return false;
    }

    const TARHeader* header = reinterpret_cast<const TARHeader*>(data);

    // Check if this is an empty block (end of archive)
    bool all_zero = true;
    for (size_t i = 0; i < sizeof(TARHeader); i++) {
        if (data[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (all_zero) {
        return false;
    }

    // Verify ustar magic (optional but recommended)
    if (std::memcmp(header->magic, "ustar", 5) != 0) {
        // Not ustar format, might be old tar format
        // Continue anyway
    }

    // Verify checksum
    uint32_t stored_checksum = read_octal(header->checksum, sizeof(header->checksum));
    uint32_t calculated_checksum = calculate_checksum(*header);
    if (stored_checksum != calculated_checksum) {
        return false;
    }

    // Extract filename
    entry.filename = std::string(header->name, strnlen(header->name, sizeof(header->name)));

    // Extract metadata
    entry.mode = read_octal(header->mode, sizeof(header->mode));
    entry.uid = read_octal(header->uid, sizeof(header->uid));
    entry.gid = read_octal(header->gid, sizeof(header->gid));
    entry.size = read_octal(header->size, sizeof(header->size));
    entry.mtime = read_octal(header->mtime, sizeof(header->mtime));
    entry.typeflag = header->typeflag;

    // If typeflag is 0 or \0, it's a regular file
    if (entry.typeflag == 0) {
        entry.typeflag = '0';
    }

    return true;
}

bool TAR::is_tar(const uint8_t* data, size_t size) {
    if (size < sizeof(TARHeader)) {
        return false;
    }

    const TARHeader* header = reinterpret_cast<const TARHeader*>(data);

    // Check ustar magic
    if (std::memcmp(header->magic, "ustar", 5) == 0) {
        return true;
    }

    // Check for valid checksum (old tar format)
    uint32_t stored_checksum = read_octal(header->checksum, sizeof(header->checksum));
    uint32_t calculated_checksum = calculate_checksum(*header);

    return stored_checksum == calculated_checksum;
}

void TAR::add_file(
    std::vector<TAREntry>& entries,
    const std::string& filename,
    const uint8_t* data,
    size_t size) {

    TAREntry entry;
    entry.filename = filename;
    entry.mode = 0644;  // rw-r--r--
    entry.uid = 1000;
    entry.gid = 1000;
    entry.size = size;
    entry.mtime = std::time(nullptr);
    entry.typeflag = '0';  // Regular file
    entry.data.assign(data, data + size);

    entries.push_back(entry);
}

fconvert_error_t TAR::create(
    const std::vector<TAREntry>& entries,
    std::vector<uint8_t>& output) {

    output.clear();

    for (const auto& entry : entries) {
        // Write header
        write_header(output, entry);

        // Write file data
        output.insert(output.end(), entry.data.begin(), entry.data.end());

        // Pad to 512-byte boundary
        size_t padding = (512 - (entry.size % 512)) % 512;
        for (size_t i = 0; i < padding; i++) {
            output.push_back(0);
        }
    }

    // Write two empty blocks to mark end of archive
    for (int i = 0; i < 1024; i++) {
        output.push_back(0);
    }

    return FCONVERT_OK;
}

fconvert_error_t TAR::extract(
    const uint8_t* tar_data,
    size_t tar_size,
    std::vector<TAREntry>& entries) {

    entries.clear();

    size_t pos = 0;
    while (pos + sizeof(TARHeader) <= tar_size) {
        TAREntry entry;

        if (!read_header(tar_data + pos, tar_size - pos, entry)) {
            // End of archive or invalid header
            break;
        }

        pos += sizeof(TARHeader);

        // Read file data
        if (entry.typeflag == '0' || entry.typeflag == '\0') {
            // Regular file
            if (pos + entry.size > tar_size) {
                return FCONVERT_ERROR_CORRUPTED_FILE;
            }

            entry.data.assign(tar_data + pos, tar_data + pos + entry.size);
            pos += entry.size;

            // Skip padding to 512-byte boundary
            size_t padding = (512 - (entry.size % 512)) % 512;
            pos += padding;
        }

        entries.push_back(entry);
    }

    return FCONVERT_OK;
}

} // namespace utils
} // namespace fconvert
