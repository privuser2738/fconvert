/**
 * Netpbm image formats implementation
 */

#include "netpbm.h"
#include <sstream>
#include <cstring>
#include <algorithm>

namespace fconvert {
namespace formats {

bool NetpbmCodec::is_netpbm(const uint8_t* data, size_t size) {
    if (size < 2) return false;
    if (data[0] != 'P') return false;
    return (data[1] >= '1' && data[1] <= '6');
}

NetpbmFormat NetpbmCodec::detect_format(const uint8_t* data, size_t size) {
    if (!is_netpbm(data, size)) {
        return NetpbmFormat::PPM_BINARY;  // Default
    }
    return static_cast<NetpbmFormat>(data[1] - '0');
}

size_t NetpbmCodec::skip_whitespace_and_comments(
    const uint8_t* data, size_t size, size_t pos) {

    while (pos < size) {
        // Skip whitespace
        while (pos < size && (data[pos] == ' ' || data[pos] == '\t' ||
                              data[pos] == '\n' || data[pos] == '\r')) {
            pos++;
        }

        // Skip comments
        if (pos < size && data[pos] == '#') {
            while (pos < size && data[pos] != '\n') {
                pos++;
            }
        } else {
            break;
        }
    }
    return pos;
}

size_t NetpbmCodec::read_int(
    const uint8_t* data, size_t size, size_t pos, int& value) {

    pos = skip_whitespace_and_comments(data, size, pos);
    value = 0;

    while (pos < size && data[pos] >= '0' && data[pos] <= '9') {
        value = value * 10 + (data[pos] - '0');
        pos++;
    }
    return pos;
}

uint8_t NetpbmCodec::rgb_to_gray(uint8_t r, uint8_t g, uint8_t b) {
    // Standard luminance formula
    return static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
}

fconvert_error_t NetpbmCodec::decode(
    const std::vector<uint8_t>& data,
    BMPImage& image) {

    if (data.size() < 7) {  // Minimum: "P6\n1 1\n1\n" + 3 bytes
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    const uint8_t* ptr = data.data();
    size_t size = data.size();

    // Check magic number
    if (ptr[0] != 'P' || ptr[1] < '1' || ptr[1] > '6') {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    NetpbmFormat format = static_cast<NetpbmFormat>(ptr[1] - '0');
    size_t pos = 2;

    // Read dimensions
    int width, height, maxval = 1;
    pos = read_int(ptr, size, pos, width);
    pos = read_int(ptr, size, pos, height);

    // PBM doesn't have maxval
    if (format != NetpbmFormat::PBM_ASCII && format != NetpbmFormat::PBM_BINARY) {
        pos = read_int(ptr, size, pos, maxval);
    }

    if (width <= 0 || height <= 0 || maxval <= 0 || maxval > 65535) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Skip single whitespace after header
    if (pos < size && (ptr[pos] == ' ' || ptr[pos] == '\n' || ptr[pos] == '\r')) {
        pos++;
    }

    // Allocate image
    image.width = width;
    image.height = height;
    image.channels = 3;  // Always output RGB
    image.pixels.resize(width * height * 3);

    // Decode based on format
    switch (format) {
        case NetpbmFormat::PPM_BINARY: {  // P6
            size_t expected = width * height * 3;
            if (size - pos < expected) {
                return FCONVERT_ERROR_INVALID_FORMAT;
            }
            // Copy directly (PPM is already RGB)
            std::memcpy(image.pixels.data(), ptr + pos, expected);
            break;
        }

        case NetpbmFormat::PGM_BINARY: {  // P5
            size_t expected = width * height;
            if (size - pos < expected) {
                return FCONVERT_ERROR_INVALID_FORMAT;
            }
            // Convert grayscale to RGB
            for (int i = 0; i < width * height; i++) {
                uint8_t gray = ptr[pos + i];
                image.pixels[i * 3 + 0] = gray;
                image.pixels[i * 3 + 1] = gray;
                image.pixels[i * 3 + 2] = gray;
            }
            break;
        }

        case NetpbmFormat::PBM_BINARY: {  // P4
            int row_bytes = (width + 7) / 8;
            if (size - pos < static_cast<size_t>(row_bytes * height)) {
                return FCONVERT_ERROR_INVALID_FORMAT;
            }
            // Convert 1-bit to RGB
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int byte_idx = y * row_bytes + x / 8;
                    int bit_idx = 7 - (x % 8);
                    uint8_t pixel = (ptr[pos + byte_idx] >> bit_idx) & 1;
                    uint8_t color = pixel ? 0 : 255;  // 1 = black, 0 = white
                    int idx = (y * width + x) * 3;
                    image.pixels[idx + 0] = color;
                    image.pixels[idx + 1] = color;
                    image.pixels[idx + 2] = color;
                }
            }
            break;
        }

        case NetpbmFormat::PPM_ASCII: {  // P3
            for (int i = 0; i < width * height * 3; i++) {
                int value;
                pos = read_int(ptr, size, pos, value);
                image.pixels[i] = static_cast<uint8_t>(value * 255 / maxval);
            }
            break;
        }

        case NetpbmFormat::PGM_ASCII: {  // P2
            for (int i = 0; i < width * height; i++) {
                int value;
                pos = read_int(ptr, size, pos, value);
                uint8_t gray = static_cast<uint8_t>(value * 255 / maxval);
                image.pixels[i * 3 + 0] = gray;
                image.pixels[i * 3 + 1] = gray;
                image.pixels[i * 3 + 2] = gray;
            }
            break;
        }

        case NetpbmFormat::PBM_ASCII: {  // P1
            for (int i = 0; i < width * height; i++) {
                int value;
                pos = read_int(ptr, size, pos, value);
                uint8_t color = value ? 0 : 255;  // 1 = black, 0 = white
                image.pixels[i * 3 + 0] = color;
                image.pixels[i * 3 + 1] = color;
                image.pixels[i * 3 + 2] = color;
            }
            break;
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t NetpbmCodec::encode_ppm(
    const BMPImage& image,
    std::vector<uint8_t>& data,
    bool binary) {

    if (image.width == 0 || image.height == 0) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    std::ostringstream header;
    header << (binary ? "P6" : "P3") << "\n";
    header << image.width << " " << image.height << "\n";
    header << "255\n";

    std::string hdr = header.str();

    if (binary) {
        // Binary PPM
        size_t pixel_size = image.width * image.height * 3;
        data.resize(hdr.size() + pixel_size);
        std::memcpy(data.data(), hdr.data(), hdr.size());

        // Copy pixels (convert RGBA to RGB if needed)
        if (image.channels == 4) {
            for (size_t i = 0; i < image.width * image.height; i++) {
                data[hdr.size() + i * 3 + 0] = image.pixels[i * 4 + 0];
                data[hdr.size() + i * 3 + 1] = image.pixels[i * 4 + 1];
                data[hdr.size() + i * 3 + 2] = image.pixels[i * 4 + 2];
            }
        } else {
            std::memcpy(data.data() + hdr.size(), image.pixels.data(), pixel_size);
        }
    } else {
        // ASCII PPM
        std::ostringstream oss;
        oss << hdr;
        for (size_t i = 0; i < image.width * image.height; i++) {
            size_t idx = i * image.channels;
            oss << static_cast<int>(image.pixels[idx]) << " "
                << static_cast<int>(image.pixels[idx + 1]) << " "
                << static_cast<int>(image.pixels[idx + 2]);
            if ((i + 1) % image.width == 0) {
                oss << "\n";
            } else {
                oss << "  ";
            }
        }
        std::string result = oss.str();
        data.assign(result.begin(), result.end());
    }

    return FCONVERT_OK;
}

fconvert_error_t NetpbmCodec::encode_pgm(
    const BMPImage& image,
    std::vector<uint8_t>& data,
    bool binary) {

    if (image.width == 0 || image.height == 0) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    std::ostringstream header;
    header << (binary ? "P5" : "P2") << "\n";
    header << image.width << " " << image.height << "\n";
    header << "255\n";

    std::string hdr = header.str();

    if (binary) {
        // Binary PGM
        size_t pixel_count = image.width * image.height;
        data.resize(hdr.size() + pixel_count);
        std::memcpy(data.data(), hdr.data(), hdr.size());

        // Convert to grayscale
        for (size_t i = 0; i < pixel_count; i++) {
            size_t idx = i * image.channels;
            data[hdr.size() + i] = rgb_to_gray(
                image.pixels[idx],
                image.pixels[idx + 1],
                image.pixels[idx + 2]);
        }
    } else {
        // ASCII PGM
        std::ostringstream oss;
        oss << hdr;
        for (size_t i = 0; i < image.width * image.height; i++) {
            size_t idx = i * image.channels;
            uint8_t gray = rgb_to_gray(
                image.pixels[idx],
                image.pixels[idx + 1],
                image.pixels[idx + 2]);
            oss << static_cast<int>(gray);
            if ((i + 1) % image.width == 0) {
                oss << "\n";
            } else {
                oss << " ";
            }
        }
        std::string result = oss.str();
        data.assign(result.begin(), result.end());
    }

    return FCONVERT_OK;
}

fconvert_error_t NetpbmCodec::encode_pbm(
    const BMPImage& image,
    std::vector<uint8_t>& data,
    bool binary) {

    if (image.width == 0 || image.height == 0) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    std::ostringstream header;
    header << (binary ? "P4" : "P1") << "\n";
    header << image.width << " " << image.height << "\n";

    std::string hdr = header.str();

    if (binary) {
        // Binary PBM
        int row_bytes = (image.width + 7) / 8;
        data.resize(hdr.size() + row_bytes * image.height);
        std::memcpy(data.data(), hdr.data(), hdr.size());

        // Convert to 1-bit
        std::fill(data.begin() + hdr.size(), data.end(), 0);
        for (uint32_t y = 0; y < image.height; y++) {
            for (uint32_t x = 0; x < image.width; x++) {
                size_t idx = (y * image.width + x) * image.channels;
                uint8_t gray = rgb_to_gray(
                    image.pixels[idx],
                    image.pixels[idx + 1],
                    image.pixels[idx + 2]);
                // Threshold at 128
                if (gray < 128) {
                    // Set bit (1 = black)
                    int byte_idx = hdr.size() + y * row_bytes + x / 8;
                    int bit_idx = 7 - (x % 8);
                    data[byte_idx] |= (1 << bit_idx);
                }
            }
        }
    } else {
        // ASCII PBM
        std::ostringstream oss;
        oss << hdr;
        for (uint32_t y = 0; y < image.height; y++) {
            for (uint32_t x = 0; x < image.width; x++) {
                size_t idx = (y * image.width + x) * image.channels;
                uint8_t gray = rgb_to_gray(
                    image.pixels[idx],
                    image.pixels[idx + 1],
                    image.pixels[idx + 2]);
                oss << (gray < 128 ? "1" : "0");
                if (x + 1 < image.width) oss << " ";
            }
            oss << "\n";
        }
        std::string result = oss.str();
        data.assign(result.begin(), result.end());
    }

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
