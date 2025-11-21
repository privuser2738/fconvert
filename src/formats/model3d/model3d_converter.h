/**
 * 3D Model Format Converter
 * Handles conversions between 3D model formats (STL, OBJ, etc.)
 */

#ifndef MODEL3D_CONVERTER_H
#define MODEL3D_CONVERTER_H

#include "../../core/converter.h"
#include <string>
#include <vector>

namespace fconvert {
namespace formats {

/**
 * Converter for 3D model formats
 * Supports: STL (ASCII/Binary), OBJ (Wavefront)
 */
class Model3dConverter : public core::Converter {
public:
    /**
     * Convert between 3D model formats
     * Supported conversions:
     * - STL <-> OBJ
     * - STL (ASCII) <-> STL (Binary)
     */
    fconvert_error_t convert(
        const std::vector<uint8_t>& input_data,
        const std::string& input_format,
        std::vector<uint8_t>& output_data,
        const std::string& output_format,
        const core::ConversionParams& params) override;

    /**
     * Check if conversion between formats is supported
     */
    bool can_convert(
        const std::string& input_format,
        const std::string& output_format) override;

    /**
     * Get the category this converter handles
     */
    file_type_category_t get_category() const override {
        return FILE_TYPE_MODEL3D;
    }

private:
    // Normalize format string (lowercase, remove dots)
    static std::string normalize_format(const std::string& fmt);
};

} // namespace formats
} // namespace fconvert

#endif // MODEL3D_CONVERTER_H
