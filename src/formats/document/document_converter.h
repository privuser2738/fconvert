/**
 * Document Format Converter
 * Handles conversions between text document formats (TXT, MD, etc.)
 */

#ifndef DOCUMENT_CONVERTER_H
#define DOCUMENT_CONVERTER_H

#include "../../core/converter.h"
#include <string>
#include <vector>

namespace fconvert {
namespace formats {

/**
 * Converter for document formats
 * Supports: TXT (plain text), MD (markdown)
 */
class DocumentConverter : public core::Converter {
public:
    /**
     * Convert between document formats
     * Supported conversions:
     * - TXT <-> MD
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
        return FILE_TYPE_DOCUMENT;
    }

private:
    // Normalize format string (lowercase, remove dots)
    static std::string normalize_format(const std::string& fmt);
};

} // namespace formats
} // namespace fconvert

#endif // DOCUMENT_CONVERTER_H
