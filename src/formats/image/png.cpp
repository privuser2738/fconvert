/**
 * PNG codec implementation
 * Implements PNG decoding and encoding using our custom DEFLATE implementation
 */

#include "png.h"
#include "../../utils/deflate.h"
#include "../../utils/crc32.h"
#include <cstring>

namespace fconvert {
namespace formats {

// PNG signature: 89 50 4E 47 0D 0A 1A 0A
static const uint8_t PNG_SIGNATURE[8] = {137, 80, 78, 71, 13, 10, 26, 10};

// PNG color types
enum PNGColorType {
    PNG_GRAYSCALE = 0,
    PNG_RGB = 2,
    PNG_PALETTE = 3,
    PNG_GRAYSCALE_ALPHA = 4,
    PNG_RGBA = 6
};

// PNG filter types
enum PNGFilter {
    FILTER_NONE = 0,
    FILTER_SUB = 1,
    FILTER_UP = 2,
    FILTER_AVERAGE = 3,
    FILTER_PAETH = 4
};

// Helper: Read 32-bit big-endian integer
static uint32_t read_u32_be(const uint8_t* data) {
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           ((uint32_t)data[3]);
}

// Helper: Write 32-bit big-endian integer
static void write_u32_be(uint8_t* data, uint32_t value) {
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
}

// Paeth predictor for PNG filtering
static uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c) {
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

// Unfilter PNG scanline
static void unfilter_scanline(
    uint8_t* current,
    const uint8_t* previous,
    size_t length,
    int filter_type,
    int bytes_per_pixel) {

    switch (filter_type) {
        case FILTER_NONE:
            // No filtering
            break;

        case FILTER_SUB:
            for (size_t i = bytes_per_pixel; i < length; i++) {
                current[i] = (current[i] + current[i - bytes_per_pixel]) & 0xFF;
            }
            break;

        case FILTER_UP:
            if (previous) {
                for (size_t i = 0; i < length; i++) {
                    current[i] = (current[i] + previous[i]) & 0xFF;
                }
            }
            break;

        case FILTER_AVERAGE:
            for (size_t i = 0; i < length; i++) {
                uint8_t left = (i >= (size_t)bytes_per_pixel) ? current[i - bytes_per_pixel] : 0;
                uint8_t up = previous ? previous[i] : 0;
                current[i] = (current[i] + ((left + up) / 2)) & 0xFF;
            }
            break;

        case FILTER_PAETH:
            for (size_t i = 0; i < length; i++) {
                uint8_t left = (i >= (size_t)bytes_per_pixel) ? current[i - bytes_per_pixel] : 0;
                uint8_t up = previous ? previous[i] : 0;
                uint8_t up_left = (previous && i >= (size_t)bytes_per_pixel) ? previous[i - bytes_per_pixel] : 0;
                current[i] = (current[i] + paeth_predictor(left, up, up_left)) & 0xFF;
            }
            break;
    }
}

// Apply filter to PNG scanline (for encoding)
static void filter_scanline(
    uint8_t* current,
    const uint8_t* previous,
    size_t length,
    int filter_type,
    int bytes_per_pixel) {

    // For now, we'll just use FILTER_NONE for simplicity
    // Full implementation would try all filters and pick the best
    (void)previous;
    (void)length;
    (void)filter_type;
    (void)bytes_per_pixel;
    // No filtering needed for FILTER_NONE
}

fconvert_error_t PNGCodec::decode(const std::vector<uint8_t>& data, BMPImage& image) {
    if (data.size() < 8) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    // Verify PNG signature
    if (memcmp(data.data(), PNG_SIGNATURE, 8) != 0) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    size_t pos = 8;

    // PNG header info
    uint32_t width = 0, height = 0;
    uint8_t bit_depth = 0, color_type = 0;
    uint8_t compression = 0, filter_method = 0, interlace = 0;

    // Collect IDAT chunks
    std::vector<uint8_t> compressed_data;

    // Parse chunks
    while (pos + 12 <= data.size()) {
        uint32_t chunk_length = read_u32_be(data.data() + pos);
        pos += 4;

        if (pos + 4 + chunk_length + 4 > data.size()) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        uint8_t chunk_type[4];
        memcpy(chunk_type, data.data() + pos, 4);
        pos += 4;

        const uint8_t* chunk_data = data.data() + pos;
        pos += chunk_length;

        uint32_t crc_stored = read_u32_be(data.data() + pos);
        pos += 4;

        // Verify CRC (optional but recommended)
        uint32_t crc_calc = utils::CRC32::calculate(chunk_type, 4);
        crc_calc = utils::CRC32::calculate(chunk_data, chunk_length, crc_calc ^ 0xFFFFFFFF) ^ 0xFFFFFFFF;

        if (crc_calc != crc_stored) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        // Process chunk
        if (memcmp(chunk_type, "IHDR", 4) == 0) {
            if (chunk_length != 13) return FCONVERT_ERROR_CORRUPTED_FILE;

            width = read_u32_be(chunk_data);
            height = read_u32_be(chunk_data + 4);
            bit_depth = chunk_data[8];
            color_type = chunk_data[9];
            compression = chunk_data[10];
            filter_method = chunk_data[11];
            interlace = chunk_data[12];

            // Validate
            if (compression != 0 || filter_method != 0 || interlace != 0) {
                return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
            }

            if (bit_depth != 8) {
                return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; // Only 8-bit for now
            }

        } else if (memcmp(chunk_type, "IDAT", 4) == 0) {
            compressed_data.insert(compressed_data.end(), chunk_data, chunk_data + chunk_length);

        } else if (memcmp(chunk_type, "IEND", 4) == 0) {
            break;
        }
        // Ignore other chunks
    }

    if (width == 0 || height == 0) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    // Decompress IDAT data
    std::vector<uint8_t> raw_data;
    utils::Inflate inflater;

    // PNG uses zlib wrapper around DEFLATE
    // zlib header: 2 bytes, then DEFLATE data, then 4 bytes ADLER32
    if (compressed_data.size() < 6) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    // Skip zlib header (2 bytes) and checksum (4 bytes at end)
    fconvert_error_t result = inflater.decompress(
        compressed_data.data() + 2,
        compressed_data.size() - 6,
        raw_data);

    if (result != FCONVERT_OK) {
        return result;
    }

    // Determine bytes per pixel
    int bytes_per_pixel = 0;
    int output_channels = 0;

    switch (color_type) {
        case PNG_GRAYSCALE:
            bytes_per_pixel = 1;
            output_channels = 3; // Convert to RGB
            break;
        case PNG_RGB:
            bytes_per_pixel = 3;
            output_channels = 3;
            break;
        case PNG_RGBA:
            bytes_per_pixel = 4;
            output_channels = 4;
            break;
        default:
            return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    size_t scanline_size = width * bytes_per_pixel;
    size_t expected_size = height * (scanline_size + 1); // +1 for filter byte

    if (raw_data.size() != expected_size) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    // Unfilter and convert to output format
    image.width = width;
    image.height = height;
    image.channels = output_channels;
    image.pixels.resize(width * height * output_channels);

    std::vector<uint8_t> prev_scanline(scanline_size, 0);

    for (uint32_t y = 0; y < height; y++) {
        size_t offset = y * (scanline_size + 1);
        uint8_t filter_type = raw_data[offset];
        uint8_t* scanline = raw_data.data() + offset + 1;

        // Unfilter
        unfilter_scanline(scanline, y > 0 ? prev_scanline.data() : nullptr,
                         scanline_size, filter_type, bytes_per_pixel);

        // Copy to output
        for (uint32_t x = 0; x < width; x++) {
            size_t out_offset = (y * width + x) * output_channels;

            if (color_type == PNG_GRAYSCALE) {
                // Convert grayscale to RGB
                uint8_t gray = scanline[x];
                image.pixels[out_offset + 0] = gray;
                image.pixels[out_offset + 1] = gray;
                image.pixels[out_offset + 2] = gray;
            } else {
                // Copy RGB or RGBA
                for (int c = 0; c < bytes_per_pixel; c++) {
                    image.pixels[out_offset + c] = scanline[x * bytes_per_pixel + c];
                }
            }
        }

        memcpy(prev_scanline.data(), scanline, scanline_size);
    }

    return FCONVERT_OK;
}

fconvert_error_t PNGCodec::encode(const BMPImage& image, std::vector<uint8_t>& data) {
    data.clear();

    // Write PNG signature
    data.insert(data.end(), PNG_SIGNATURE, PNG_SIGNATURE + 8);

    // Prepare IHDR chunk
    uint8_t ihdr_data[13];
    write_u32_be(ihdr_data, image.width);
    write_u32_be(ihdr_data + 4, image.height);
    ihdr_data[8] = 8; // bit depth
    ihdr_data[9] = (image.channels == 4) ? PNG_RGBA : PNG_RGB;
    ihdr_data[10] = 0; // compression
    ihdr_data[11] = 0; // filter method
    ihdr_data[12] = 0; // no interlace

    // Write IHDR chunk
    size_t chunk_start = data.size();
    data.resize(data.size() + 4);
    write_u32_be(data.data() + chunk_start, 13);
    data.insert(data.end(), (uint8_t*)"IHDR", (uint8_t*)"IHDR" + 4);
    data.insert(data.end(), ihdr_data, ihdr_data + 13);

    uint32_t ihdr_crc = utils::CRC32::calculate((uint8_t*)"IHDR", 4);
    ihdr_crc = utils::CRC32::calculate(ihdr_data, 13, ihdr_crc ^ 0xFFFFFFFF) ^ 0xFFFFFFFF;
    chunk_start = data.size();
    data.resize(data.size() + 4);
    write_u32_be(data.data() + chunk_start, ihdr_crc);

    // Prepare pixel data with filters
    std::vector<uint8_t> filtered_data;
    size_t scanline_size = image.width * image.channels;

    for (uint32_t y = 0; y < image.height; y++) {
        filtered_data.push_back(FILTER_NONE); // Filter type

        // Copy scanline
        const uint8_t* scanline = image.pixels.data() + y * scanline_size;
        filtered_data.insert(filtered_data.end(), scanline, scanline + scanline_size);
    }

    // Compress using DEFLATE
    std::vector<uint8_t> compressed;
    utils::Deflate deflater;
    deflater.compress(filtered_data.data(), filtered_data.size(), compressed);

    // Add zlib wrapper
    std::vector<uint8_t> zlib_data;
    zlib_data.push_back(0x78); // CMF (deflate, 32K window)
    zlib_data.push_back(0x01); // FLG (no preset dict, fastest compression)
    zlib_data.insert(zlib_data.end(), compressed.begin(), compressed.end());

    // ADLER32 checksum (simplified - just use zeros for now)
    zlib_data.push_back(0);
    zlib_data.push_back(0);
    zlib_data.push_back(0);
    zlib_data.push_back(0);

    // Write IDAT chunk
    size_t idat_pos = data.size();
    data.resize(data.size() + 4);
    write_u32_be(data.data() + idat_pos, zlib_data.size());
    data.insert(data.end(), (uint8_t*)"IDAT", (uint8_t*)"IDAT" + 4);
    data.insert(data.end(), zlib_data.begin(), zlib_data.end());

    uint32_t idat_crc = utils::CRC32::calculate((uint8_t*)"IDAT", 4);
    idat_crc = utils::CRC32::calculate(zlib_data.data(), zlib_data.size(), idat_crc ^ 0xFFFFFFFF) ^ 0xFFFFFFFF;
    idat_pos = data.size();
    data.resize(data.size() + 4);
    write_u32_be(data.data() + idat_pos, idat_crc);

    // Write IEND chunk
    size_t iend_pos = data.size();
    data.resize(data.size() + 4);
    write_u32_be(data.data() + iend_pos, 0); // length = 0
    data.insert(data.end(), (uint8_t*)"IEND", (uint8_t*)"IEND" + 4);

    uint32_t iend_crc = utils::CRC32::calculate((uint8_t*)"IEND", 4);
    iend_pos = data.size();
    data.resize(data.size() + 4);
    write_u32_be(data.data() + iend_pos, iend_crc);

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
