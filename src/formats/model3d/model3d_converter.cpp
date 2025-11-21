/**
 * 3D Model Format Converter Implementation
 */

#include "model3d_converter.h"
#include "stl.h"
#include "obj.h"
#include <algorithm>
#include <cctype>

namespace fconvert {
namespace formats {

std::string Model3dConverter::normalize_format(const std::string& fmt) {
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

bool Model3dConverter::can_convert(
    const std::string& input_format,
    const std::string& output_format) {

    std::string in_fmt = normalize_format(input_format);
    std::string out_fmt = normalize_format(output_format);

    // Supported formats: stl, obj
    const std::vector<std::string> supported = {"stl", "obj"};

    // Check if both formats are supported
    bool in_supported = std::find(supported.begin(), supported.end(), in_fmt) != supported.end();
    bool out_supported = std::find(supported.begin(), supported.end(), out_fmt) != supported.end();

    return in_supported && out_supported;
}

fconvert_error_t Model3dConverter::convert(
    const std::vector<uint8_t>& input_data,
    const std::string& input_format,
    std::vector<uint8_t>& output_data,
    const std::string& output_format,
    const core::ConversionParams& params) {

    if (input_data.empty()) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    std::string in_fmt = normalize_format(input_format);
    std::string out_fmt = normalize_format(output_format);

    // Check if conversion is supported
    if (!can_convert(in_fmt, out_fmt)) {
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    // Decode input to intermediate Mesh3D format
    Mesh3D mesh;
    fconvert_error_t result;

    if (in_fmt == "stl") {
        result = STLCodec::decode(input_data, mesh);
    } else if (in_fmt == "obj") {
        result = OBJCodec::decode(input_data, mesh);
    } else {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    if (result != FCONVERT_OK) {
        return result;
    }

    // Validate mesh
    if (mesh.triangles.empty()) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Encode to output format
    if (out_fmt == "stl") {
        // Use quality parameter to choose ASCII vs Binary
        // quality < 50: Binary (compact)
        // quality >= 50: ASCII (human-readable)
        if (params.quality < 50) {
            result = STLCodec::encode_binary(mesh, output_data);
        } else {
            result = STLCodec::encode_ascii(mesh, output_data);
        }
    } else if (out_fmt == "obj") {
        result = OBJCodec::encode(mesh, output_data);
    } else {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    return result;
}

} // namespace formats
} // namespace fconvert
