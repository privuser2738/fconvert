/**
 * Image format converter implementation
 */

#include "image_converter.h"
#include "bmp.h"
#include "png.h"
#include "jpeg.h"
#include "tga.h"
#include "../../core/logger.h"
#include "../../utils/image_transform.h"
#include <algorithm>

namespace fconvert {
namespace formats {

ImageConverter::ImageConverter() {
}

bool ImageConverter::is_supported_format(const std::string& format) const {
    static const std::vector<std::string> supported = {
        "bmp", "png", "jpg", "jpeg", "gif", "webp", "tiff", "tif", "tga", "ppm", "pgm", "pbm"
    };

    std::string fmt_lower = format;
    std::transform(fmt_lower.begin(), fmt_lower.end(), fmt_lower.begin(), [](unsigned char c){ return std::tolower(c); });

    return std::find(supported.begin(), supported.end(), fmt_lower) != supported.end();
}

bool ImageConverter::can_convert(const std::string& from_format, const std::string& to_format) {
    return is_supported_format(from_format) && is_supported_format(to_format);
}

fconvert_error_t ImageConverter::convert(
    const std::vector<uint8_t>& input_data,
    const std::string& input_format,
    std::vector<uint8_t>& output_data,
    const std::string& output_format,
    const core::ConversionParams& params) {

    std::string in_fmt = input_format;
    std::string out_fmt = output_format;
    std::transform(in_fmt.begin(), in_fmt.end(), in_fmt.begin(), [](unsigned char c){ return std::tolower(c); });
    std::transform(out_fmt.begin(), out_fmt.end(), out_fmt.begin(), [](unsigned char c){ return std::tolower(c); });

    // Normalize jpeg extension
    if (in_fmt == "jpeg") in_fmt = "jpg";
    if (out_fmt == "jpeg") out_fmt = "jpg";

    core::Logger::instance().debug("Converting image: " + in_fmt + " -> " + out_fmt);

    // Decode input format to intermediate format
    BMPImage image;
    fconvert_error_t result = FCONVERT_OK;

    if (in_fmt == "bmp") {
        result = BMPCodec::decode(input_data, image);
    } else if (in_fmt == "png") {
        result = PNGCodec::decode(input_data, image);
    } else if (in_fmt == "jpg") {
        result = JPEGCodec::decode(input_data, image, params.quality);
    } else if (in_fmt == "tga") {
        result = TGACodec::decode(input_data, image);
    } else {
        core::Logger::instance().error("Unsupported input format: " + input_format);
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    if (result != FCONVERT_OK) {
        core::Logger::instance().error("Failed to decode input image");
        return result;
    }

    core::Logger::instance().debug("Image decoded: " + std::to_string(image.width) + "x" +
                                  std::to_string(image.height) + " (" +
                                  std::to_string(image.channels) + " channels)");

    // Apply transformations if requested
    bool needs_transform = (params.width > 0 || params.height > 0 ||
                           params.rotate != 0 || params.flip_horizontal || params.flip_vertical);

    if (needs_transform) {
        // Convert BMPImage to ImageData
        utils::ImageData img_data;
        img_data.width = image.width;
        img_data.height = image.height;
        img_data.channels = image.channels;
        img_data.pixels = std::move(image.pixels);

        // Apply flip transformations
        if (params.flip_horizontal) {
            core::Logger::instance().debug("Applying horizontal flip");
            utils::ImageData flipped;
            result = utils::ImageTransform::flip_horizontal(img_data, flipped);
            if (result != FCONVERT_OK) {
                core::Logger::instance().error("Failed to flip image horizontally");
                return result;
            }
            img_data = std::move(flipped);
        }

        if (params.flip_vertical) {
            core::Logger::instance().debug("Applying vertical flip");
            utils::ImageData flipped;
            result = utils::ImageTransform::flip_vertical(img_data, flipped);
            if (result != FCONVERT_OK) {
                core::Logger::instance().error("Failed to flip image vertically");
                return result;
            }
            img_data = std::move(flipped);
        }

        // Apply rotation
        if (params.rotate != 0) {
            core::Logger::instance().debug("Rotating image by " + std::to_string(params.rotate) + " degrees");
            utils::ImageData rotated;
            result = utils::ImageTransform::rotate(img_data, rotated, params.rotate);
            if (result != FCONVERT_OK) {
                core::Logger::instance().error("Failed to rotate image");
                return result;
            }
            img_data = std::move(rotated);
        }

        // Apply resize
        if (params.width > 0 || params.height > 0) {
            uint32_t target_width = (params.width > 0) ? params.width : img_data.width;
            uint32_t target_height = (params.height > 0) ? params.height : img_data.height;

            // Select interpolation method
            utils::InterpolationMethod method = utils::InterpolationMethod::BILINEAR;
            if (params.interpolation == 0) {
                method = utils::InterpolationMethod::NEAREST;
            } else if (params.interpolation == 1) {
                method = utils::InterpolationMethod::BILINEAR;
            } else if (params.interpolation == 2) {
                method = utils::InterpolationMethod::BICUBIC;
            }

            core::Logger::instance().debug("Resizing image to " + std::to_string(target_width) + "x" +
                                          std::to_string(target_height));
            utils::ImageData resized;
            result = utils::ImageTransform::resize(img_data, resized, target_width, target_height,
                                                  method, params.keep_aspect_ratio);
            if (result != FCONVERT_OK) {
                core::Logger::instance().error("Failed to resize image");
                return result;
            }
            img_data = std::move(resized);
        }

        // Convert back to BMPImage
        image.width = img_data.width;
        image.height = img_data.height;
        image.channels = img_data.channels;
        image.pixels = std::move(img_data.pixels);

        core::Logger::instance().debug("Transformations applied: " + std::to_string(image.width) + "x" +
                                      std::to_string(image.height));
    }

    // Encode to output format
    if (out_fmt == "bmp") {
        result = BMPCodec::encode(image, output_data);
    } else if (out_fmt == "png") {
        result = PNGCodec::encode(image, output_data);
    } else if (out_fmt == "jpg") {
        result = JPEGCodec::encode(image, output_data, params.quality);
    } else if (out_fmt == "tga") {
        // Use RLE compression for TGA if quality is high
        if (params.quality >= 80) {
            result = TGACodec::encode_rle(image, output_data);
        } else {
            result = TGACodec::encode(image, output_data);
        }
    } else {
        core::Logger::instance().error("Unsupported output format: " + output_format);
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    if (result != FCONVERT_OK) {
        core::Logger::instance().error("Failed to encode output image");
        return result;
    }

    core::Logger::instance().debug("Image encoded successfully (" +
                                  std::to_string(output_data.size()) + " bytes)");

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
