/**
 * BMP codec implementation
 */

#include "bmp.h"
#include <cstring>
#include <algorithm>

namespace fconvert {
namespace formats {

fconvert_error_t BMPCodec::decode(const std::vector<uint8_t>& data, BMPImage& image) {
    if (data.size() < sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    // Read headers
    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    memcpy(&file_header, data.data(), sizeof(BMPFileHeader));
    memcpy(&info_header, data.data() + sizeof(BMPFileHeader), sizeof(BMPInfoHeader));

    // Validate
    if (!validate_header(file_header, info_header)) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Only support 24-bit and 32-bit uncompressed BMPs for now
    if (info_header.bits_per_pixel != 24 && info_header.bits_per_pixel != 32) {
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    if (info_header.compression != 0) { // BI_RGB
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    // Set up image
    image.width = info_header.width;
    image.height = abs(info_header.height);
    image.channels = info_header.bits_per_pixel / 8;

    // Calculate row size (must be multiple of 4)
    uint32_t row_size = calculate_row_size(image.width, info_header.bits_per_pixel);
    uint32_t pixel_row_size = image.width * image.channels;

    // Allocate pixel buffer
    image.pixels.resize(image.width * image.height * image.channels);

    // BMP stores pixels bottom-to-top by default
    bool bottom_up = info_header.height > 0;

    // Read pixel data
    const uint8_t* pixel_data = data.data() + file_header.data_offset;

    for (uint32_t y = 0; y < image.height; y++) {
        uint32_t src_row = y;
        uint32_t dst_row = bottom_up ? (image.height - 1 - y) : y;

        const uint8_t* src = pixel_data + src_row * row_size;
        uint8_t* dst = image.pixels.data() + dst_row * pixel_row_size;

        // BMP stores pixels as BGR(A), convert to RGB(A)
        for (uint32_t x = 0; x < image.width; x++) {
            if (image.channels == 3) {
                dst[x * 3 + 0] = src[x * 3 + 2]; // R
                dst[x * 3 + 1] = src[x * 3 + 1]; // G
                dst[x * 3 + 2] = src[x * 3 + 0]; // B
            } else if (image.channels == 4) {
                dst[x * 4 + 0] = src[x * 4 + 2]; // R
                dst[x * 4 + 1] = src[x * 4 + 1]; // G
                dst[x * 4 + 2] = src[x * 4 + 0]; // B
                dst[x * 4 + 3] = src[x * 4 + 3]; // A
            }
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t BMPCodec::encode(const BMPImage& image, std::vector<uint8_t>& data) {
    if (image.width == 0 || image.height == 0) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    if (image.channels != 3 && image.channels != 4) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    // Force 24-bit output (drop alpha channel if present)
    uint32_t output_bpp = 24;
    uint32_t output_channels = 3;
    uint32_t row_size = calculate_row_size(image.width, output_bpp);
    uint32_t pixel_data_size = row_size * image.height;
    uint32_t file_size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + pixel_data_size;

    data.resize(file_size);

    // Create file header
    BMPFileHeader file_header = {};
    file_header.signature = 0x4D42; // 'BM'
    file_header.file_size = file_size;
    file_header.data_offset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    // Create info header
    BMPInfoHeader info_header = {};
    info_header.header_size = sizeof(BMPInfoHeader);
    info_header.width = image.width;
    info_header.height = image.height; // Positive for bottom-up
    info_header.planes = 1;
    info_header.bits_per_pixel = output_bpp;
    info_header.compression = 0; // BI_RGB
    info_header.image_size = pixel_data_size;
    info_header.x_pixels_per_meter = 2835; // 72 DPI
    info_header.y_pixels_per_meter = 2835;

    // Write headers
    memcpy(data.data(), &file_header, sizeof(BMPFileHeader));
    memcpy(data.data() + sizeof(BMPFileHeader), &info_header, sizeof(BMPInfoHeader));

    // Write pixel data (bottom-to-top, BGR format)
    uint8_t* pixel_data = data.data() + file_header.data_offset;
    uint32_t src_row_size = image.width * image.channels;

    for (uint32_t y = 0; y < image.height; y++) {
        uint32_t src_row = image.height - 1 - y; // Flip vertically
        uint32_t dst_row = y;

        const uint8_t* src = image.pixels.data() + src_row * src_row_size;
        uint8_t* dst = pixel_data + dst_row * row_size;

        // Convert RGB(A) to BGR and write
        for (uint32_t x = 0; x < image.width; x++) {
            dst[x * 3 + 0] = src[x * image.channels + 2]; // B
            dst[x * 3 + 1] = src[x * image.channels + 1]; // G
            dst[x * 3 + 2] = src[x * image.channels + 0]; // R
        }

        // Padding bytes are already zero from resize
    }

    return FCONVERT_OK;
}

bool BMPCodec::validate_header(const BMPFileHeader& file_header, const BMPInfoHeader& info_header) {
    // Check signature
    if (file_header.signature != 0x4D42) { // 'BM'
        return false;
    }

    // Check header size
    if (info_header.header_size != sizeof(BMPInfoHeader) &&
        info_header.header_size != 40) { // BITMAPINFOHEADER
        return false;
    }

    // Check dimensions
    if (info_header.width <= 0 || abs(info_header.height) <= 0) {
        return false;
    }

    // Check planes
    if (info_header.planes != 1) {
        return false;
    }

    return true;
}

uint32_t BMPCodec::calculate_row_size(uint32_t width, uint32_t bits_per_pixel) {
    // Row size must be multiple of 4 bytes
    uint32_t row_bits = width * bits_per_pixel;
    uint32_t row_bytes = (row_bits + 7) / 8;
    return (row_bytes + 3) & ~3;
}

} // namespace formats
} // namespace fconvert
