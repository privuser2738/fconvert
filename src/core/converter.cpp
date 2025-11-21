/**
 * Converter registry implementation
 */

#include "converter.h"
#include "file_detector.h"
#include "logger.h"
#include "../utils/file_utils.h"

namespace fconvert {
namespace core {

ConverterRegistry& ConverterRegistry::instance() {
    static ConverterRegistry registry;
    return registry;
}

ConverterRegistry::ConverterRegistry() {
}

ConverterRegistry::~ConverterRegistry() {
}

void ConverterRegistry::register_converter(std::shared_ptr<Converter> converter) {
    converters_.push_back(converter);
}

std::shared_ptr<Converter> ConverterRegistry::find_converter(const std::string& from, const std::string& to) {
    for (auto& converter : converters_) {
        if (converter->can_convert(from, to)) {
            return converter;
        }
    }
    return nullptr;
}

bool ConverterRegistry::can_convert(const std::string& from_format, const std::string& to_format) {
    return find_converter(from_format, to_format) != nullptr;
}

fconvert_error_t ConverterRegistry::convert_file(
    const std::string& input_path,
    const std::string& output_path,
    const ConversionParams& params) {

    Logger::instance().info("Converting: " + input_path + " -> " + output_path);

    // Check if input file exists
    if (!utils::FileUtils::file_exists(input_path)) {
        Logger::instance().error("Input file not found: " + input_path);
        return FCONVERT_ERROR_FILE_NOT_FOUND;
    }

    // Detect input format
    FileTypeInfo input_info = FileDetector::instance().detect_from_file(input_path);
    if (input_info.category == FILE_TYPE_UNKNOWN) {
        Logger::instance().error("Unknown input file format");
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Get output format from extension
    std::string output_ext = utils::FileUtils::get_file_extension(output_path);
    if (output_ext.empty()) {
        Logger::instance().error("No output format specified");
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Read input file
    std::vector<uint8_t> input_data;
    if (!utils::FileUtils::read_file(input_path, input_data)) {
        Logger::instance().error("Failed to read input file");
        return FCONVERT_ERROR_IO;
    }

    // Convert
    std::vector<uint8_t> output_data;
    fconvert_error_t result = convert_data(input_data, input_info.extension, output_data, output_ext, params);

    if (result != FCONVERT_OK) {
        return result;
    }

    // Write output file
    if (!utils::FileUtils::write_file(output_path, output_data)) {
        Logger::instance().error("Failed to write output file");
        return FCONVERT_ERROR_IO;
    }

    Logger::instance().info("Conversion completed successfully");
    return FCONVERT_OK;
}

fconvert_error_t ConverterRegistry::convert_data(
    const std::vector<uint8_t>& input_data,
    const std::string& input_format,
    std::vector<uint8_t>& output_data,
    const std::string& output_format,
    const ConversionParams& params) {

    auto converter = find_converter(input_format, output_format);
    if (!converter) {
        Logger::instance().error("No converter found for: " + input_format + " -> " + output_format);
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    return converter->convert(input_data, input_format, output_data, output_format, params);
}

} // namespace core
} // namespace fconvert
