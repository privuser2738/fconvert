/**
 * JPEG image codec
 * STUB: Full implementation requires DCT, quantization, Huffman coding
 */

#ifndef JPEG_H
#define JPEG_H

#include "../../../include/fconvert.h"
#include "bmp.h"
#include <vector>

namespace fconvert {
namespace formats {

class JPEGCodec {
public:
    // NOTE: Full JPEG implementation requires:
    // - Discrete Cosine Transform (DCT) - forward and inverse
    // - Quantization tables
    // - Huffman encoding/decoding
    // - YCbCr color space conversion
    // - Marker parsing (SOI, SOF, DHT, DQT, SOS, EOI, etc.)
    // - Block-based processing (8x8 MCUs)
    // - Progressive and sequential modes
    // This is a stub - real implementation is ~10000+ lines

    static fconvert_error_t decode(const std::vector<uint8_t>& data, BMPImage& image, int quality = 85);
    static fconvert_error_t encode(const BMPImage& image, std::vector<uint8_t>& data, int quality = 85);
};

} // namespace formats
} // namespace fconvert

#endif // JPEG_H
