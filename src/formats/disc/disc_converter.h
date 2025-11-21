/**
 * Disc Image Converter
 * Converts between ISO, BIN/CUE, VHD, and CHD formats
 */

#ifndef DISC_CONVERTER_H
#define DISC_CONVERTER_H

#include "../../../include/fconvert.h"
#include "iso.h"
#include "bincue.h"
#include "vhd.h"
#include "chd.h"

namespace fconvert {

// Conversion options for disc images
struct DiscConvertOptions {
    bool compress;              // Use compression if target supports it
    uint32_t block_size;        // Block/hunk size for VHD/CHD
    bool dynamic_vhd;           // Create dynamic vs fixed VHD

    DiscConvertOptions() : compress(true), block_size(0), dynamic_vhd(true) {}
};

/**
 * Disc Image Converter
 */
class DiscConverter {
public:
    /**
     * Get supported input formats
     */
    static std::vector<disc_format_t> get_input_formats();

    /**
     * Get supported output formats
     */
    static std::vector<disc_format_t> get_output_formats();

    /**
     * Check if conversion is supported
     */
    static bool can_convert(disc_format_t from, disc_format_t to);

    /**
     * Convert disc image
     */
    static fconvert_error_t convert(
        const std::vector<uint8_t>& input,
        disc_format_t input_type,
        std::vector<uint8_t>& output,
        disc_format_t output_type,
        const DiscConvertOptions* options = nullptr);

    /**
     * Detect disc image format
     */
    static disc_format_t detect_format(const uint8_t* data, size_t size);

    /**
     * Get format name
     */
    static const char* format_name(disc_format_t format);

    /**
     * Get format extension
     */
    static const char* format_extension(disc_format_t format);

private:
    // Internal conversion functions
    static fconvert_error_t iso_to_bincue(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output);

    static fconvert_error_t bincue_to_iso(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output);

    static fconvert_error_t iso_to_vhd(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output,
        const DiscConvertOptions* options);

    static fconvert_error_t vhd_to_iso(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output);

    static fconvert_error_t iso_to_chd(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output);

    static fconvert_error_t chd_to_iso(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output);

    static fconvert_error_t vhd_to_chd(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output);

    static fconvert_error_t chd_to_vhd(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output,
        const DiscConvertOptions* options);

    static fconvert_error_t bincue_to_chd(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output);

    static fconvert_error_t chd_to_bincue(
        const std::vector<uint8_t>& input,
        std::vector<uint8_t>& output);
};

} // namespace fconvert

#endif // DISC_CONVERTER_H
