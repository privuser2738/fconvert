/**
 * BMP (Bitmap) image format codec
 * Supports 24-bit and 32-bit uncompressed BMP files
 */

#ifndef BMP_H
#define BMP_H

#include "../../../include/fconvert.h"
#include <vector>
#include <cstdint>

namespace fconvert {
namespace formats {

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t signature;      // 'BM'
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t data_offset;
};

struct BMPInfoHeader {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
};
#pragma pack(pop)

struct BMPImage {
    uint32_t width;
    uint32_t height;
    uint32_t channels;  // 3 for RGB, 4 for RGBA
    std::vector<uint8_t> pixels;  // Row-major, bottom-to-top
};

class BMPCodec {
public:
    static fconvert_error_t decode(const std::vector<uint8_t>& data, BMPImage& image);
    static fconvert_error_t encode(const BMPImage& image, std::vector<uint8_t>& data);

private:
    static bool validate_header(const BMPFileHeader& file_header, const BMPInfoHeader& info_header);
    static uint32_t calculate_row_size(uint32_t width, uint32_t bits_per_pixel);
};

} // namespace formats
} // namespace fconvert

#endif // BMP_H
