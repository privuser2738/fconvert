/**
 * Image transformation utilities implementation
 */

#include "image_transform.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace fconvert {
namespace utils {

fconvert_error_t ImageTransform::resize(
    const ImageData& input,
    ImageData& output,
    uint32_t new_width,
    uint32_t new_height,
    InterpolationMethod method,
    bool preserve_aspect) {

    if (new_width == 0 || new_height == 0) {
        return FCONVERT_ERROR_INVALID_ARGUMENT;
    }

    // Calculate dimensions with aspect ratio preservation if requested
    uint32_t target_width = new_width;
    uint32_t target_height = new_height;

    if (preserve_aspect) {
        float aspect = (float)input.width / (float)input.height;
        float target_aspect = (float)new_width / (float)new_height;

        if (target_aspect > aspect) {
            // Width is larger, adjust it
            target_width = (uint32_t)(new_height * aspect);
        } else {
            // Height is larger, adjust it
            target_height = (uint32_t)(new_width / aspect);
        }

        if (target_width == 0) target_width = 1;
        if (target_height == 0) target_height = 1;
    }

    // Dispatch to appropriate resize method
    switch (method) {
        case InterpolationMethod::NEAREST:
            return resize_nearest(input, output, target_width, target_height);
        case InterpolationMethod::BILINEAR:
            return resize_bilinear(input, output, target_width, target_height);
        case InterpolationMethod::BICUBIC:
            return resize_bicubic(input, output, target_width, target_height);
        default:
            return FCONVERT_ERROR_INVALID_ARGUMENT;
    }
}

fconvert_error_t ImageTransform::resize_nearest(
    const ImageData& input,
    ImageData& output,
    uint32_t new_width,
    uint32_t new_height) {

    output.width = new_width;
    output.height = new_height;
    output.channels = input.channels;
    output.pixels.resize(new_width * new_height * input.channels);

    float x_ratio = (float)input.width / (float)new_width;
    float y_ratio = (float)input.height / (float)new_height;

    for (uint32_t y = 0; y < new_height; y++) {
        for (uint32_t x = 0; x < new_width; x++) {
            // Find nearest source pixel
            uint32_t src_x = (uint32_t)(x * x_ratio);
            uint32_t src_y = (uint32_t)(y * y_ratio);

            // Clamp to valid range
            src_x = std::min(src_x, input.width - 1);
            src_y = std::min(src_y, input.height - 1);

            // Copy pixel
            size_t src_idx = (src_y * input.width + src_x) * input.channels;
            size_t dst_idx = (y * new_width + x) * input.channels;

            for (uint8_t c = 0; c < input.channels; c++) {
                output.pixels[dst_idx + c] = input.pixels[src_idx + c];
            }
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t ImageTransform::resize_bilinear(
    const ImageData& input,
    ImageData& output,
    uint32_t new_width,
    uint32_t new_height) {

    output.width = new_width;
    output.height = new_height;
    output.channels = input.channels;
    output.pixels.resize(new_width * new_height * input.channels);

    float x_ratio = (float)(input.width - 1) / (float)new_width;
    float y_ratio = (float)(input.height - 1) / (float)new_height;

    for (uint32_t y = 0; y < new_height; y++) {
        for (uint32_t x = 0; x < new_width; x++) {
            // Calculate source coordinates
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;

            int x1 = (int)src_x;
            int y1 = (int)src_y;
            int x2 = std::min(x1 + 1, (int)input.width - 1);
            int y2 = std::min(y1 + 1, (int)input.height - 1);

            float x_frac = src_x - x1;
            float y_frac = src_y - y1;

            size_t dst_idx = (y * new_width + x) * input.channels;

            // Bilinear interpolation for each channel
            for (uint8_t c = 0; c < input.channels; c++) {
                // Get four neighboring pixels
                uint8_t p11 = input.pixels[(y1 * input.width + x1) * input.channels + c];
                uint8_t p21 = input.pixels[(y1 * input.width + x2) * input.channels + c];
                uint8_t p12 = input.pixels[(y2 * input.width + x1) * input.channels + c];
                uint8_t p22 = input.pixels[(y2 * input.width + x2) * input.channels + c];

                // Interpolate in x direction
                float c1 = lerp((float)p11, (float)p21, x_frac);
                float c2 = lerp((float)p12, (float)p22, x_frac);

                // Interpolate in y direction
                float result = lerp(c1, c2, y_frac);

                output.pixels[dst_idx + c] = (uint8_t)clamp((int)result, 0, 255);
            }
        }
    }

    return FCONVERT_OK;
}

float ImageTransform::cubic_kernel(float x) {
    // Catmull-Rom spline (a = -0.5)
    x = std::abs(x);
    if (x <= 1.0f) {
        return 1.5f * x * x * x - 2.5f * x * x + 1.0f;
    } else if (x < 2.0f) {
        return -0.5f * x * x * x + 2.5f * x * x - 4.0f * x + 2.0f;
    }
    return 0.0f;
}

fconvert_error_t ImageTransform::resize_bicubic(
    const ImageData& input,
    ImageData& output,
    uint32_t new_width,
    uint32_t new_height) {

    output.width = new_width;
    output.height = new_height;
    output.channels = input.channels;
    output.pixels.resize(new_width * new_height * input.channels);

    float x_ratio = (float)(input.width - 1) / (float)new_width;
    float y_ratio = (float)(input.height - 1) / (float)new_height;

    for (uint32_t y = 0; y < new_height; y++) {
        for (uint32_t x = 0; x < new_width; x++) {
            // Calculate source coordinates
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;

            int x0 = (int)src_x;
            int y0 = (int)src_y;

            size_t dst_idx = (y * new_width + x) * input.channels;

            // Bicubic interpolation for each channel
            for (uint8_t c = 0; c < input.channels; c++) {
                float sum = 0.0f;
                float weight_sum = 0.0f;

                // Sample 4x4 neighborhood
                for (int dy = -1; dy <= 2; dy++) {
                    for (int dx = -1; dx <= 2; dx++) {
                        int sx = clamp(x0 + dx, 0, (int)input.width - 1);
                        int sy = clamp(y0 + dy, 0, (int)input.height - 1);

                        float wx = cubic_kernel(src_x - (x0 + dx));
                        float wy = cubic_kernel(src_y - (y0 + dy));
                        float weight = wx * wy;

                        uint8_t pixel = input.pixels[(sy * input.width + sx) * input.channels + c];
                        sum += pixel * weight;
                        weight_sum += weight;
                    }
                }

                float result = sum / weight_sum;
                output.pixels[dst_idx + c] = (uint8_t)clamp((int)result, 0, 255);
            }
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t ImageTransform::rotate(
    const ImageData& input,
    ImageData& output,
    int degrees) {

    // Normalize degrees to 0, 90, 180, 270
    degrees = ((degrees % 360) + 360) % 360;

    switch (degrees) {
        case 0:
            // No rotation, just copy
            output = input;
            return FCONVERT_OK;

        case 90: {
            // Rotate 90 degrees clockwise
            output.width = input.height;
            output.height = input.width;
            output.channels = input.channels;
            output.pixels.resize(output.width * output.height * output.channels);

            for (uint32_t y = 0; y < input.height; y++) {
                for (uint32_t x = 0; x < input.width; x++) {
                    // (x, y) -> (height - 1 - y, x)
                    uint32_t new_x = input.height - 1 - y;
                    uint32_t new_y = x;

                    size_t src_idx = (y * input.width + x) * input.channels;
                    size_t dst_idx = (new_y * output.width + new_x) * output.channels;

                    for (uint8_t c = 0; c < input.channels; c++) {
                        output.pixels[dst_idx + c] = input.pixels[src_idx + c];
                    }
                }
            }
            return FCONVERT_OK;
        }

        case 180: {
            // Rotate 180 degrees
            output.width = input.width;
            output.height = input.height;
            output.channels = input.channels;
            output.pixels.resize(output.width * output.height * output.channels);

            for (uint32_t y = 0; y < input.height; y++) {
                for (uint32_t x = 0; x < input.width; x++) {
                    // (x, y) -> (width - 1 - x, height - 1 - y)
                    uint32_t new_x = input.width - 1 - x;
                    uint32_t new_y = input.height - 1 - y;

                    size_t src_idx = (y * input.width + x) * input.channels;
                    size_t dst_idx = (new_y * output.width + new_x) * output.channels;

                    for (uint8_t c = 0; c < input.channels; c++) {
                        output.pixels[dst_idx + c] = input.pixels[src_idx + c];
                    }
                }
            }
            return FCONVERT_OK;
        }

        case 270: {
            // Rotate 270 degrees clockwise (90 counter-clockwise)
            output.width = input.height;
            output.height = input.width;
            output.channels = input.channels;
            output.pixels.resize(output.width * output.height * output.channels);

            for (uint32_t y = 0; y < input.height; y++) {
                for (uint32_t x = 0; x < input.width; x++) {
                    // (x, y) -> (y, width - 1 - x)
                    uint32_t new_x = y;
                    uint32_t new_y = input.width - 1 - x;

                    size_t src_idx = (y * input.width + x) * input.channels;
                    size_t dst_idx = (new_y * output.width + new_x) * output.channels;

                    for (uint8_t c = 0; c < input.channels; c++) {
                        output.pixels[dst_idx + c] = input.pixels[src_idx + c];
                    }
                }
            }
            return FCONVERT_OK;
        }

        default:
            return FCONVERT_ERROR_INVALID_ARGUMENT;
    }
}

fconvert_error_t ImageTransform::flip_horizontal(
    const ImageData& input,
    ImageData& output) {

    output.width = input.width;
    output.height = input.height;
    output.channels = input.channels;
    output.pixels.resize(input.width * input.height * input.channels);

    for (uint32_t y = 0; y < input.height; y++) {
        for (uint32_t x = 0; x < input.width; x++) {
            // (x, y) -> (width - 1 - x, y)
            uint32_t new_x = input.width - 1 - x;

            size_t src_idx = (y * input.width + x) * input.channels;
            size_t dst_idx = (y * output.width + new_x) * output.channels;

            for (uint8_t c = 0; c < input.channels; c++) {
                output.pixels[dst_idx + c] = input.pixels[src_idx + c];
            }
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t ImageTransform::flip_vertical(
    const ImageData& input,
    ImageData& output) {

    output.width = input.width;
    output.height = input.height;
    output.channels = input.channels;
    output.pixels.resize(input.width * input.height * input.channels);

    for (uint32_t y = 0; y < input.height; y++) {
        for (uint32_t x = 0; x < input.width; x++) {
            // (x, y) -> (x, height - 1 - y)
            uint32_t new_y = input.height - 1 - y;

            size_t src_idx = (y * input.width + x) * input.channels;
            size_t dst_idx = (new_y * output.width + x) * output.channels;

            for (uint8_t c = 0; c < input.channels; c++) {
                output.pixels[dst_idx + c] = input.pixels[src_idx + c];
            }
        }
    }

    return FCONVERT_OK;
}

void ImageTransform::sample_pixel(
    const ImageData& input,
    int x, int y,
    uint8_t* pixel) {

    x = clamp(x, 0, (int)input.width - 1);
    y = clamp(y, 0, (int)input.height - 1);

    size_t idx = (y * input.width + x) * input.channels;
    for (uint8_t c = 0; c < input.channels; c++) {
        pixel[c] = input.pixels[idx + c];
    }
}

} // namespace utils
} // namespace fconvert
