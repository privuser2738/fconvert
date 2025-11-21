/**
 * CHD (Compressed Hunks of Data) Format Implementation
 */

#include "chd.h"
#include "../../utils/deflate.h"
#include <cstring>
#include <algorithm>

namespace fconvert {
namespace formats {

using utils::Inflate;
using utils::Deflate;

// FourCC codes for compressors
constexpr uint32_t CHD_CODEC_NONE = 0;
constexpr uint32_t CHD_CODEC_ZLIB = 0x7A6C6962;  // "zlib"
constexpr uint32_t CHD_CODEC_LZMA = 0x6C7A6D61;  // "lzma"
constexpr uint32_t CHD_CODEC_HUFF = 0x68756666;  // "huff"
constexpr uint32_t CHD_CODEC_FLAC = 0x666C6163;  // "flac"

uint32_t CHDCodec::swap32(uint32_t val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

uint64_t CHDCodec::swap64(uint64_t val) {
    return ((val >> 56) & 0xFF) |
           ((val >> 40) & 0xFF00) |
           ((val >> 24) & 0xFF0000) |
           ((val >> 8) & 0xFF000000) |
           ((val << 8) & 0xFF00000000ULL) |
           ((val << 24) & 0xFF0000000000ULL) |
           ((val << 40) & 0xFF000000000000ULL) |
           ((val << 56) & 0xFF00000000000000ULL);
}

uint32_t CHDCodec::make_fourcc(const char* str) {
    return (static_cast<uint32_t>(str[0]) << 24) |
           (static_cast<uint32_t>(str[1]) << 16) |
           (static_cast<uint32_t>(str[2]) << 8) |
           static_cast<uint32_t>(str[3]);
}

uint16_t CHDCodec::crc16(const uint8_t* data, size_t size) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool CHDCodec::is_chd(const uint8_t* data, size_t size) {
    if (size < sizeof(ChdV5Header)) {
        return false;
    }
    return std::memcmp(data, CHD_MAGIC, 8) == 0;
}

fconvert_error_t CHDCodec::decompress_hunk(
    const uint8_t* compressed,
    uint32_t compressed_size,
    uint8_t* output,
    uint32_t output_size,
    uint32_t compressor) {

    if (compressor == CHD_CODEC_NONE || compressor == 0) {
        // Uncompressed
        std::memcpy(output, compressed, std::min(compressed_size, output_size));
        return FCONVERT_OK;
    }

    if (compressor == CHD_CODEC_ZLIB) {
        // Use our deflate decompressor
        Inflate inflater;
        std::vector<uint8_t> decompressed;
        fconvert_error_t result = inflater.decompress(compressed, compressed_size, decompressed);
        if (result != FCONVERT_OK) {
            return result;
        }
        std::memcpy(output, decompressed.data(),
                   std::min(static_cast<size_t>(output_size), decompressed.size()));
        return FCONVERT_OK;
    }

    // Unsupported compression
    return FCONVERT_ERROR_INVALID_FORMAT;
}

fconvert_error_t CHDCodec::compress_hunk(
    const uint8_t* input,
    uint32_t input_size,
    std::vector<uint8_t>& output,
    uint32_t compressor) {

    if (compressor == CHD_CODEC_NONE || compressor == 0) {
        output.resize(input_size);
        std::memcpy(output.data(), input, input_size);
        return FCONVERT_OK;
    }

    if (compressor == CHD_CODEC_ZLIB) {
        Deflate deflater;
        return deflater.compress(input, input_size, output, 6);
    }

    return FCONVERT_ERROR_INVALID_FORMAT;
}

fconvert_error_t CHDCodec::decode(
    const std::vector<uint8_t>& data,
    ChdImage& image) {

    if (!is_chd(data.data(), data.size())) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    const ChdV5Header* header = reinterpret_cast<const ChdV5Header*>(data.data());

    image.version = swap32(header->version);
    if (image.version != 5) {
        // Only support v5 for now
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    image.logical_size = swap64(header->logical_bytes);
    image.hunk_size = swap32(header->hunk_bytes);
    image.unit_size = swap32(header->unit_bytes);

    for (int i = 0; i < 4; i++) {
        image.compressors[i] = swap32(header->compressors[i]);
    }

    // Store raw file for reading hunks
    image.raw_file = data;

    // Parse map
    uint64_t map_offset = swap64(header->map_offset);
    uint32_t num_hunks = static_cast<uint32_t>((image.logical_size + image.hunk_size - 1) / image.hunk_size);

    image.map.resize(num_hunks);

    // CHD v5 map: compressed map at map_offset
    // First decompress the map
    const uint8_t* map_data = data.data() + map_offset;

    // Map entry size varies, but for v5 it's typically 12 bytes per entry compressed
    // The map is stored in a compressed format

    // Simplified: read map entries directly
    // Each entry is: offset (5 bytes) + length (3 bytes) + compressed length (3 bytes) + flags (1 byte)
    size_t map_entry_size = 12;
    size_t expected_map_size = num_hunks * map_entry_size;

    // Check if map is compressed
    if (map_offset + expected_map_size > data.size()) {
        // Map might be compressed - try to decompress
        // For simplicity, we'll try to read what we can
    }

    // Parse map entries
    for (uint32_t i = 0; i < num_hunks && map_offset + (i + 1) * map_entry_size <= data.size(); i++) {
        const uint8_t* entry = map_data + i * map_entry_size;

        // Read compressed offset (6 bytes, big-endian)
        uint64_t offset = 0;
        for (int j = 0; j < 6; j++) {
            offset = (offset << 8) | entry[j];
        }

        // Read compressed length (3 bytes)
        uint32_t comp_len = (entry[6] << 16) | (entry[7] << 8) | entry[8];

        // Read CRC16 (2 bytes)
        uint16_t crc = (entry[9] << 8) | entry[10];

        // Read type (1 byte)
        uint8_t type = entry[11];

        image.map[i].offset = offset;
        image.map[i].length = comp_len;
        image.map[i].crc = crc;
        image.map[i].type = static_cast<ChdMapType>(type & 0x0F);
    }

    // Parse metadata at meta_offset
    uint64_t meta_offset = swap64(header->meta_offset);
    while (meta_offset != 0 && meta_offset < data.size()) {
        // Metadata header: tag (4) + flags (1) + next offset (3) + length (3)
        const uint8_t* meta = data.data() + meta_offset;
        if (meta_offset + 11 > data.size()) break;

        uint32_t tag = (meta[0] << 24) | (meta[1] << 16) | (meta[2] << 8) | meta[3];
        uint8_t flags = meta[4];
        (void)flags;  // Currently unused

        uint64_t next = (meta[5] << 16) | (meta[6] << 8) | meta[7];
        uint32_t length = (meta[8] << 16) | (meta[9] << 8) | meta[10];

        if (meta_offset + 11 + length <= data.size()) {
            ChdMetadata md;
            md.tag = tag;
            md.data.resize(length);
            std::memcpy(md.data.data(), meta + 11, length);
            image.metadata.push_back(md);
        }

        meta_offset = next;
    }

    return FCONVERT_OK;
}

fconvert_error_t CHDCodec::encode(
    const ChdImage& image,
    std::vector<uint8_t>& data) {

    uint32_t num_hunks = get_hunk_count(image);
    uint32_t compressor = image.compressors[0];
    if (compressor == 0) compressor = CHD_CODEC_ZLIB;

    // Calculate sizes
    uint64_t header_size = 124;  // v5 header
    uint64_t map_size = num_hunks * 12;  // 12 bytes per entry
    uint64_t map_offset = header_size;
    uint64_t data_offset = map_offset + map_size;

    // Compress all hunks first
    std::vector<std::vector<uint8_t>> compressed_hunks(num_hunks);
    std::vector<ChdMapEntry> map_entries(num_hunks);
    uint64_t current_offset = data_offset;

    for (uint32_t i = 0; i < num_hunks; i++) {
        // Get hunk data
        uint64_t hunk_start = static_cast<uint64_t>(i) * image.hunk_size;
        uint32_t hunk_len = static_cast<uint32_t>(
            std::min(static_cast<uint64_t>(image.hunk_size),
                    image.logical_size - hunk_start));

        std::vector<uint8_t> hunk_data(image.hunk_size, 0);

        if (i < image.hunks.size() && !image.hunks[i].empty()) {
            std::memcpy(hunk_data.data(), image.hunks[i].data(),
                       std::min(image.hunks[i].size(), static_cast<size_t>(image.hunk_size)));
        }

        // Compress
        compress_hunk(hunk_data.data(), image.hunk_size, compressed_hunks[i], compressor);

        // Check if compression helped
        bool use_compression = compressed_hunks[i].size() < hunk_data.size();

        if (!use_compression) {
            compressed_hunks[i] = hunk_data;
            map_entries[i].type = ChdMapType::UNCOMPRESSED;
        } else {
            map_entries[i].type = ChdMapType::COMPRESSED;
        }

        map_entries[i].offset = current_offset;
        map_entries[i].length = static_cast<uint32_t>(compressed_hunks[i].size());
        map_entries[i].crc = crc16(compressed_hunks[i].data(), compressed_hunks[i].size());

        current_offset += compressed_hunks[i].size();
    }

    // Calculate total size
    uint64_t total_size = current_offset;
    data.resize(total_size, 0);

    // Write header
    ChdV5Header* header = reinterpret_cast<ChdV5Header*>(data.data());
    std::memcpy(header->tag, CHD_MAGIC, 8);
    header->header_length = swap32(124);
    header->version = swap32(5);
    header->compressors[0] = swap32(compressor);
    header->compressors[1] = 0;
    header->compressors[2] = 0;
    header->compressors[3] = 0;
    header->logical_bytes = swap64(image.logical_size);
    header->map_offset = swap64(map_offset);
    header->meta_offset = swap64(0);  // No metadata for now
    header->hunk_bytes = swap32(image.hunk_size);
    header->unit_bytes = swap32(image.unit_size > 0 ? image.unit_size : image.hunk_size);

    // Write map
    uint8_t* map_ptr = data.data() + map_offset;
    for (uint32_t i = 0; i < num_hunks; i++) {
        uint8_t* entry = map_ptr + i * 12;

        // Write offset (6 bytes big-endian)
        uint64_t offset = map_entries[i].offset;
        entry[0] = (offset >> 40) & 0xFF;
        entry[1] = (offset >> 32) & 0xFF;
        entry[2] = (offset >> 24) & 0xFF;
        entry[3] = (offset >> 16) & 0xFF;
        entry[4] = (offset >> 8) & 0xFF;
        entry[5] = offset & 0xFF;

        // Write length (3 bytes)
        uint32_t len = map_entries[i].length;
        entry[6] = (len >> 16) & 0xFF;
        entry[7] = (len >> 8) & 0xFF;
        entry[8] = len & 0xFF;

        // Write CRC (2 bytes)
        entry[9] = (map_entries[i].crc >> 8) & 0xFF;
        entry[10] = map_entries[i].crc & 0xFF;

        // Write type (1 byte)
        entry[11] = static_cast<uint8_t>(map_entries[i].type);
    }

    // Write compressed hunk data
    for (uint32_t i = 0; i < num_hunks; i++) {
        std::memcpy(data.data() + map_entries[i].offset,
                   compressed_hunks[i].data(),
                   compressed_hunks[i].size());
    }

    return FCONVERT_OK;
}

fconvert_error_t CHDCodec::create_from_raw(
    const std::vector<uint8_t>& raw_data,
    ChdImage& image,
    uint32_t hunk_size) {

    image.version = 5;
    image.logical_size = raw_data.size();
    image.hunk_size = hunk_size;
    image.unit_size = hunk_size;
    image.compressors = {CHD_CODEC_ZLIB, 0, 0, 0};

    uint32_t num_hunks = static_cast<uint32_t>((raw_data.size() + hunk_size - 1) / hunk_size);
    image.hunks.resize(num_hunks);

    for (uint32_t i = 0; i < num_hunks; i++) {
        uint64_t offset = static_cast<uint64_t>(i) * hunk_size;
        uint32_t size = static_cast<uint32_t>(
            std::min(static_cast<uint64_t>(hunk_size), raw_data.size() - offset));

        image.hunks[i].resize(hunk_size, 0);
        std::memcpy(image.hunks[i].data(), raw_data.data() + offset, size);
    }

    return FCONVERT_OK;
}

fconvert_error_t CHDCodec::extract_raw(
    const ChdImage& image,
    std::vector<uint8_t>& raw_data) {

    raw_data.resize(image.logical_size, 0);
    uint32_t num_hunks = get_hunk_count(image);

    for (uint32_t i = 0; i < num_hunks; i++) {
        std::vector<uint8_t> hunk_data;
        fconvert_error_t result = read_hunk(image, i, hunk_data);
        if (result != FCONVERT_OK) {
            return result;
        }

        uint64_t offset = static_cast<uint64_t>(i) * image.hunk_size;
        uint64_t copy_size = std::min(
            static_cast<uint64_t>(hunk_data.size()),
            image.logical_size - offset);

        std::memcpy(raw_data.data() + offset, hunk_data.data(), copy_size);
    }

    return FCONVERT_OK;
}

fconvert_error_t CHDCodec::read_hunk(
    const ChdImage& image,
    uint32_t hunk_num,
    std::vector<uint8_t>& hunk_data) {

    uint32_t num_hunks = get_hunk_count(image);
    if (hunk_num >= num_hunks) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    hunk_data.resize(image.hunk_size);

    // Check if we have cached decompressed hunk
    if (hunk_num < image.hunks.size() && !image.hunks[hunk_num].empty()) {
        std::memcpy(hunk_data.data(), image.hunks[hunk_num].data(),
                   std::min(image.hunks[hunk_num].size(), static_cast<size_t>(image.hunk_size)));
        return FCONVERT_OK;
    }

    // Read from raw file using map
    if (hunk_num < image.map.size() && !image.raw_file.empty()) {
        const ChdMapEntry& entry = image.map[hunk_num];

        if (entry.type == ChdMapType::UNCOMPRESSED) {
            if (entry.offset + entry.length <= image.raw_file.size()) {
                std::memcpy(hunk_data.data(),
                           image.raw_file.data() + entry.offset,
                           std::min(static_cast<size_t>(entry.length), hunk_data.size()));
            }
        } else if (entry.type == ChdMapType::COMPRESSED) {
            if (entry.offset + entry.length <= image.raw_file.size()) {
                return decompress_hunk(
                    image.raw_file.data() + entry.offset,
                    entry.length,
                    hunk_data.data(),
                    image.hunk_size,
                    image.compressors[0]);
            }
        } else {
            // Other types: return zeros
            std::memset(hunk_data.data(), 0, image.hunk_size);
        }
    }

    return FCONVERT_OK;
}

uint32_t CHDCodec::get_hunk_count(const ChdImage& image) {
    return static_cast<uint32_t>((image.logical_size + image.hunk_size - 1) / image.hunk_size);
}

void CHDCodec::add_metadata(
    ChdImage& image,
    uint32_t tag,
    const std::vector<uint8_t>& data) {

    ChdMetadata md;
    md.tag = tag;
    md.data = data;
    image.metadata.push_back(md);
}

const ChdMetadata* CHDCodec::get_metadata(
    const ChdImage& image,
    uint32_t tag) {

    for (const auto& md : image.metadata) {
        if (md.tag == tag) {
            return &md;
        }
    }
    return nullptr;
}

} // namespace formats
} // namespace fconvert
