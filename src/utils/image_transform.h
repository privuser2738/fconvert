/**
 * Image transformation utilities
 * Provides resize, rotation, and flip operations
 */

#ifndef IMAGE_TRANSFORM_H
#define IMAGE_TRANSFORM_H

#include "../../include/fconvert.h"
#include <cstdint>
#include <vector>

namespace fconvert {
namespace utils {

// Image data structure for transformations
struct ImageData {
    uint32_t width;
    uint32_t height;
    uint8_t channels;  // 3 = RGB, 4 = RGBA
    std::vector<uint8_t> pixels;  // Row-major, top-to-bottom
};

// Interpolation methods for resizing
enum class InterpolationMethod {
    NEAREST,    // Fast, blocky
    BILINEAR,   // Good quality, fast
    BICUBIC     // Best quality, slower
};

// Image transformation operations
class ImageTransform {
public:
    /**
     * Resize image to new dimensions
     * @param input Source image
     * @param output Destination image (will be resized)
     * @param new_width Target width
     * @param new_height Target height
     * @param method Interpolation method
     * @param preserve_aspect If true, maintain aspect ratio (may result in different size)
     */
    static fconvert_error_t resize(
        const ImageData& input,
        ImageData& output,
        uint32_t new_width,
        uint32_t new_height,
        InterpolationMethod method = InterpolationMethod::BILINEAR,
        bool preserve_aspect = false);

    /**
     * Rotate image by 90, 180, or 270 degrees clockwise
     * @param input Source image
     * @param output Destination image
     * @param degrees Rotation angle (90, 180, or 270)
     */
    static fconvert_error_t rotate(
        const ImageData& input,
        ImageData& output,
        int degrees);

    /**
     * Flip image horizontally (mirror)
     * @param input Source image
     * @param output Destination image
     */
    static fconvert_error_t flip_horizontal(
        const ImageData& input,
        ImageData& output);

    /**
     * Flip image vertically
     * @param input Source image
     * @param output Destination image
     */
    static fconvert_error_t flip_vertical(
        const ImageData& input,
        ImageData& output);

private:
    // Resize implementations for different interpolation methods
    static fconvert_error_t resize_nearest(
        const ImageData& input,
        ImageData& output,
        uint32_t new_width,
        uint32_t new_height);

    static fconvert_error_t resize_bilinear(
        const ImageData& input,
        ImageData& output,
        uint32_t new_width,
        uint32_t new_height);

    static fconvert_error_t resize_bicubic(
        const ImageData& input,
        ImageData& output,
        uint32_t new_width,
        uint32_t new_height);

    // Helper: Sample pixel with bounds checking
    static void sample_pixel(
        const ImageData& input,
        int x, int y,
        uint8_t* pixel);

    // Helper: Clamp value to range
    static inline int clamp(int value, int min, int max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    // Helper: Linear interpolation
    static inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // Cubic interpolation kernel
    static float cubic_kernel(float x);
};

} // namespace utils
} // namespace fconvert

#endif // IMAGE_TRANSFORM_H
