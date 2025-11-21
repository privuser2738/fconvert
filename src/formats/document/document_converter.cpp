/**
 * Document Format Converter Implementation
 */

#include "document_converter.h"
#include "txt.h"
#include "markdown.h"
#include <algorithm>
#include <cctype>

namespace fconvert {
namespace formats {

std::string DocumentConverter::normalize_format(const std::string& fmt) {
    std::string result = fmt;

    // Remove leading dot if present
    if (!result.empty() && result[0] == '.') {
        result = result.substr(1);
    }

    // Convert to lowercase
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return result;
}

bool DocumentConverter::can_convert(
    const std::string& input_format,
    const std::string& output_format) {

    std::string in_fmt = normalize_format(input_format);
    std::string out_fmt = normalize_format(output_format);

    // Supported formats: txt, md
    const std::vector<std::string> supported = {"txt", "md"};

    // Check if both formats are supported
    bool in_supported = std::find(supported.begin(), supported.end(), in_fmt) != supported.end();
    bool out_supported = std::find(supported.begin(), supported.end(), out_fmt) != supported.end();

    return in_supported && out_supported;
}

fconvert_error_t DocumentConverter::convert(
    const std::vector<uint8_t>& input_data,
    const std::string& input_format,
    std::vector<uint8_t>& output_data,
    const std::string& output_format,
    const core::ConversionParams& params) {

    (void)params;  // Unused for now

    if (input_data.empty()) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    std::string in_fmt = normalize_format(input_format);
    std::string out_fmt = normalize_format(output_format);

    // Check if conversion is supported
    if (!can_convert(in_fmt, out_fmt)) {
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    // Decode input to intermediate TextDocument format
    TextDocument doc;
    fconvert_error_t result;

    if (in_fmt == "txt") {
        result = TXTCodec::decode(input_data, doc);
    } else if (in_fmt == "md") {
        result = MarkdownCodec::decode(input_data, doc);
    } else {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    if (result != FCONVERT_OK) {
        return result;
    }

    // Process conversion
    if (in_fmt == "md" && out_fmt == "txt") {
        // Strip markdown syntax
        doc.content = MarkdownCodec::strip_markdown(doc.content);
    }
    // TXT -> MD: just pass through (markdown can contain plain text)
    // MD -> MD: pass through
    // TXT -> TXT: pass through

    // Encode to output format
    if (out_fmt == "txt") {
        result = TXTCodec::encode(doc, output_data);
    } else if (out_fmt == "md") {
        result = MarkdownCodec::encode(doc, output_data);
    } else {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    return result;
}

} // namespace formats
} // namespace fconvert
