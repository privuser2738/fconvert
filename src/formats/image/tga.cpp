/**
 * TGA (Targa) image format implementation
 */

#include "tga.h"
#include <cstring>
#include <algorithm>

namespace fconvert {
namespace formats {

bool TGACodec::decode_rle(
    const uint8_t* input,
    size_t input_size,
    uint8_t* output,
    size_t output_size,
    uint8_t bytes_per_pixel) {

    size_t in_pos = 0;
    size_t out_pos = 0;

    while (in_pos < input_size && out_pos < output_size) {
        uint8_t packet_header = input[in_pos++];

        if (packet_header & 0x80) {
            // RLE packet (repeat next pixel)
            uint8_t count = (packet_header & 0x7F) + 1;

            if (in_pos + bytes_per_pixel > input_size) {
                return false;
            }

            // Read one pixel
            uint8_t pixel[4];
            for (uint8_t i = 0; i < bytes_per_pixel; i++) {
                pixel[i] = input[in_pos++];
            }

            // Repeat it
            for (uint8_t i = 0; i < count; i++) {
                if (out_pos + bytes_per_pixel > output_size) {
                    return false;
                }
                for (uint8_t j = 0; j < bytes_per_pixel; j++) {
                    output[out_pos++] = pixel[j];
                }
            }
        } else {
            // Raw packet (literal pixels)
            uint8_t count = (packet_header & 0x7F) + 1;
            size_t bytes = count * bytes_per_pixel;

            if (in_pos + bytes > input_size || out_pos + bytes > output_size) {
                return false;
            }

            std::memcpy(output + out_pos, input + in_pos, bytes);
            in_pos += bytes;
            out_pos += bytes;
        }
    }

    return out_pos == output_size;
}

void TGACodec::encode_rle(
    const uint8_t* input,
    size_t input_size,
    std::vector<uint8_t>& output,
    uint8_t bytes_per_pixel) {

    size_t pos = 0;

    while (pos < input_size) {
        size_t pixels_left = (input_size - pos) / bytes_per_pixel;

        // Check for run of identical pixels
        size_t run_length = 1;
        while (run_length < pixels_left && run_length < 128) {
            bool match = true;
            for (uint8_t i = 0; i < bytes_per_pixel; i++) {
                if (input[pos + i] != input[pos + run_length * bytes_per_pixel + i]) {
                    match = false;
                    break;
                }
            }
            if (!match) break;
            run_length++;
        }

        if (run_length > 1) {
            // RLE packet
            output.push_back(0x80 | (run_length - 1));
            for (uint8_t i = 0; i < bytes_per_pixel; i++) {
                output.push_back(input[pos + i]);
            }
            pos += run_length * bytes_per_pixel;
        } else {
            // Raw packet - find how many non-repeating pixels
            size_t raw_length = 1;
            while (raw_length < pixels_left && raw_length < 128) {
                // Check if next pixel starts a run
                size_t check_run = 1;
                size_t check_pos = pos + raw_length * bytes_per_pixel;

                if (raw_length + 1 < pixels_left) {
                    bool match = true;
                    for (uint8_t i = 0; i < bytes_per_pixel; i++) {
                        if (input[check_pos + i] != input[check_pos + bytes_per_pixel + i]) {
                            match = false;
                            break;
                        }
                    }
                    if (match) break; // Start of run, end raw packet
                }

                raw_length++;
            }

            // Write raw packet
            output.push_back(raw_length - 1);
            for (size_t i = 0; i < raw_length * bytes_per_pixel; i++) {
                output.push_back(input[pos++]);
            }
        }
    }
}

fconvert_error_t TGACodec::decode(
    const std::vector<uint8_t>& data,
    BMPImage& image) {

    if (data.size() < sizeof(TGAHeader)) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    const TGAHeader* header = reinterpret_cast<const TGAHeader*>(data.data());

    // Validate image type
    if (header->image_type != TGA_UNCOMPRESSED_TRUE_COLOR &&
        header->image_type != TGA_UNCOMPRESSED_GRAYSCALE &&
        header->image_type != TGA_RLE_TRUE_COLOR &&
        header->image_type != TGA_RLE_GRAYSCALE) {
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    // Only support 8, 24, and 32 bit depths
    if (header->pixel_depth != 8 && header->pixel_depth != 24 && header->pixel_depth != 32) {
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    image.width = header->width;
    image.height = header->height;

    uint8_t bytes_per_pixel = header->pixel_depth / 8;
    bool is_grayscale = (header->image_type == TGA_UNCOMPRESSED_GRAYSCALE ||
                        header->image_type == TGA_RLE_GRAYSCALE);
    bool is_rle = (header->image_type >= TGA_RLE_COLOR_MAPPED);

    // Skip ID field if present
    size_t data_offset = sizeof(TGAHeader) + header->id_length;

    // Skip color map if present
    if (header->color_map_type == 1) {
        size_t color_map_bytes = header->color_map_length * (header->color_map_entry_size / 8);
        data_offset += color_map_bytes;
    }

    if (data_offset >= data.size()) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    // Calculate expected image data size
    size_t image_data_size = image.width * image.height * bytes_per_pixel;
    std::vector<uint8_t> raw_data;

    if (is_rle) {
        // Decompress RLE data
        raw_data.resize(image_data_size);
        if (!decode_rle(data.data() + data_offset, data.size() - data_offset,
                       raw_data.data(), image_data_size, bytes_per_pixel)) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }
    } else {
        // Uncompressed data
        if (data_offset + image_data_size > data.size()) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }
        raw_data.assign(data.begin() + data_offset, data.begin() + data_offset + image_data_size);
    }

    // Convert to RGB/RGBA format
    if (is_grayscale) {
        // Grayscale to RGB
        image.channels = 3;
        image.pixels.resize(image.width * image.height * 3);

        for (size_t i = 0; i < image.width * image.height; i++) {
            uint8_t gray = raw_data[i];
            image.pixels[i * 3 + 0] = gray; // R
            image.pixels[i * 3 + 1] = gray; // G
            image.pixels[i * 3 + 2] = gray; // B
        }
    } else {
        // TGA uses BGR(A) format, convert to RGB(A)
        image.channels = bytes_per_pixel;
        image.pixels.resize(image.width * image.height * bytes_per_pixel);

        for (size_t i = 0; i < image.width * image.height; i++) {
            if (bytes_per_pixel == 3) {
                // BGR to RGB
                image.pixels[i * 3 + 0] = raw_data[i * 3 + 2]; // R
                image.pixels[i * 3 + 1] = raw_data[i * 3 + 1]; // G
                image.pixels[i * 3 + 2] = raw_data[i * 3 + 0]; // B
            } else if (bytes_per_pixel == 4) {
                // BGRA to RGBA
                image.pixels[i * 4 + 0] = raw_data[i * 4 + 2]; // R
                image.pixels[i * 4 + 1] = raw_data[i * 4 + 1]; // G
                image.pixels[i * 4 + 2] = raw_data[i * 4 + 0]; // B
                image.pixels[i * 4 + 3] = raw_data[i * 4 + 3]; // A
            }
        }
    }

    // Check if image is upside down (check origin bit)
    bool origin_upper = (header->image_descriptor & 0x20) != 0;

    if (!origin_upper) {
        // Image is stored bottom-up, flip it
        size_t row_size = image.width * image.channels;
        std::vector<uint8_t> flipped(image.pixels.size());

        for (uint32_t y = 0; y < image.height; y++) {
            std::memcpy(
                flipped.data() + y * row_size,
                image.pixels.data() + (image.height - 1 - y) * row_size,
                row_size);
        }

        image.pixels = std::move(flipped);
    }

    return FCONVERT_OK;
}

fconvert_error_t TGACodec::encode(
    const BMPImage& image,
    std::vector<uint8_t>& data) {

    data.clear();

    // Create TGA header
    TGAHeader header;
    std::memset(&header, 0, sizeof(TGAHeader));

    header.image_type = TGA_UNCOMPRESSED_TRUE_COLOR;
    header.width = image.width;
    header.height = image.height;
    header.pixel_depth = image.channels * 8;
    header.image_descriptor = 0x20; // Origin in upper left

    // Write header
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
    data.insert(data.end(), header_bytes, header_bytes + sizeof(TGAHeader));

    // Write pixel data (convert RGB(A) to BGR(A))
    for (size_t i = 0; i < image.width * image.height; i++) {
        if (image.channels == 3) {
            // RGB to BGR
            data.push_back(image.pixels[i * 3 + 2]); // B
            data.push_back(image.pixels[i * 3 + 1]); // G
            data.push_back(image.pixels[i * 3 + 0]); // R
        } else if (image.channels == 4) {
            // RGBA to BGRA
            data.push_back(image.pixels[i * 4 + 2]); // B
            data.push_back(image.pixels[i * 4 + 1]); // G
            data.push_back(image.pixels[i * 4 + 0]); // R
            data.push_back(image.pixels[i * 4 + 3]); // A
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t TGACodec::encode_rle(
    const BMPImage& image,
    std::vector<uint8_t>& data) {

    data.clear();

    // Create TGA header
    TGAHeader header;
    std::memset(&header, 0, sizeof(TGAHeader));

    header.image_type = TGA_RLE_TRUE_COLOR;
    header.width = image.width;
    header.height = image.height;
    header.pixel_depth = image.channels * 8;
    header.image_descriptor = 0x20; // Origin in upper left

    // Write header
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
    data.insert(data.end(), header_bytes, header_bytes + sizeof(TGAHeader));

    // Convert RGB(A) to BGR(A) first
    std::vector<uint8_t> bgr_data;
    bgr_data.reserve(image.width * image.height * image.channels);

    for (size_t i = 0; i < image.width * image.height; i++) {
        if (image.channels == 3) {
            bgr_data.push_back(image.pixels[i * 3 + 2]); // B
            bgr_data.push_back(image.pixels[i * 3 + 1]); // G
            bgr_data.push_back(image.pixels[i * 3 + 0]); // R
        } else if (image.channels == 4) {
            bgr_data.push_back(image.pixels[i * 4 + 2]); // B
            bgr_data.push_back(image.pixels[i * 4 + 1]); // G
            bgr_data.push_back(image.pixels[i * 4 + 0]); // R
            bgr_data.push_back(image.pixels[i * 4 + 3]); // A
        }
    }

    // Encode with RLE
    encode_rle(bgr_data.data(), bgr_data.size(), data, image.channels);

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
