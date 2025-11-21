/**
 * TGA (Targa) image format
 * Simple format supporting uncompressed and RLE compression
 */

#ifndef TGA_H
#define TGA_H

#include "../../../include/fconvert.h"
#include "bmp.h"
#include <vector>
#include <cstdint>

namespace fconvert {
namespace formats {

// TGA image types
enum TGAImageType {
    TGA_NO_IMAGE = 0,
    TGA_UNCOMPRESSED_COLOR_MAPPED = 1,
    TGA_UNCOMPRESSED_TRUE_COLOR = 2,
    TGA_UNCOMPRESSED_GRAYSCALE = 3,
    TGA_RLE_COLOR_MAPPED = 9,
    TGA_RLE_TRUE_COLOR = 10,
    TGA_RLE_GRAYSCALE = 11
};

// TGA header structure (18 bytes)
#pragma pack(push, 1)
struct TGAHeader {
    uint8_t id_length;
    uint8_t color_map_type;
    uint8_t image_type;
    uint16_t color_map_first_entry;
    uint16_t color_map_length;
    uint8_t color_map_entry_size;
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t pixel_depth;
    uint8_t image_descriptor;
};
#pragma pack(pop)

class TGACodec {
public:
    /**
     * Decode TGA image
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        BMPImage& image);

    /**
     * Encode to TGA image (uncompressed)
     */
    static fconvert_error_t encode(
        const BMPImage& image,
        std::vector<uint8_t>& data);

    /**
     * Encode to TGA image with RLE compression
     */
    static fconvert_error_t encode_rle(
        const BMPImage& image,
        std::vector<uint8_t>& data);

private:
    // Decode RLE compressed data
    static bool decode_rle(
        const uint8_t* input,
        size_t input_size,
        uint8_t* output,
        size_t output_size,
        uint8_t bytes_per_pixel);

    // Encode with RLE compression
    static void encode_rle(
        const uint8_t* input,
        size_t input_size,
        std::vector<uint8_t>& output,
        uint8_t bytes_per_pixel);
};

} // namespace formats
} // namespace fconvert

#endif // TGA_H
