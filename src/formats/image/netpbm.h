/**
 * Netpbm image formats (PPM, PGM, PBM)
 * PPM - Portable Pixmap (color)
 * PGM - Portable Graymap (grayscale)
 * PBM - Portable Bitmap (black & white)
 */

#ifndef NETPBM_H
#define NETPBM_H

#include "../../../include/fconvert.h"
#include "bmp.h"  // Reuse BMPImage structure
#include <vector>
#include <string>

namespace fconvert {
namespace formats {

enum class NetpbmFormat {
    PBM_ASCII = 1,   // P1 - Black & white, ASCII
    PGM_ASCII = 2,   // P2 - Grayscale, ASCII
    PPM_ASCII = 3,   // P3 - Color, ASCII
    PBM_BINARY = 4,  // P4 - Black & white, binary
    PGM_BINARY = 5,  // P5 - Grayscale, binary
    PPM_BINARY = 6   // P6 - Color, binary
};

/**
 * Netpbm codec - handles PPM, PGM, PBM formats
 */
class NetpbmCodec {
public:
    /**
     * Decode any Netpbm format to BMPImage
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        BMPImage& image);

    /**
     * Encode BMPImage to PPM format (P6 binary)
     */
    static fconvert_error_t encode_ppm(
        const BMPImage& image,
        std::vector<uint8_t>& data,
        bool binary = true);

    /**
     * Encode BMPImage to PGM format (P5 binary)
     */
    static fconvert_error_t encode_pgm(
        const BMPImage& image,
        std::vector<uint8_t>& data,
        bool binary = true);

    /**
     * Encode BMPImage to PBM format (P4 binary)
     */
    static fconvert_error_t encode_pbm(
        const BMPImage& image,
        std::vector<uint8_t>& data,
        bool binary = true);

    /**
     * Detect format from data
     */
    static NetpbmFormat detect_format(const uint8_t* data, size_t size);

    /**
     * Check if data is Netpbm format
     */
    static bool is_netpbm(const uint8_t* data, size_t size);

private:
    // Skip whitespace and comments
    static size_t skip_whitespace_and_comments(
        const uint8_t* data,
        size_t size,
        size_t pos);

    // Read an integer from ASCII data
    static size_t read_int(
        const uint8_t* data,
        size_t size,
        size_t pos,
        int& value);

    // Convert RGB to grayscale
    static uint8_t rgb_to_gray(uint8_t r, uint8_t g, uint8_t b);
};

} // namespace formats
} // namespace fconvert

#endif // NETPBM_H
