/**
 * Batch file processing implementation
 */

#include "batch_processor.h"
#include "logger.h"
#include "../utils/file_utils.h"
#include <iostream>

namespace fconvert {
namespace core {

BatchProcessor::BatchProcessor()
    : skip_errors_(true)
    , overwrite_(false) {
}

BatchProcessor::~BatchProcessor() {
}

BatchResult BatchProcessor::process_files(
    const std::vector<std::string>& input_files,
    const std::string& output_format,
    const std::string& output_folder,
    const ConversionParams& params) {

    BatchResult result;
    result.total_files = input_files.size();

    Logger::instance().info("Processing " + std::to_string(result.total_files) + " files...");

    for (size_t i = 0; i < input_files.size(); i++) {
        const std::string& input_file = input_files[i];

        // Generate output filename
        std::string filename = utils::FileUtils::get_filename(input_file);
        std::string output_file;

        if (output_folder.empty()) {
            output_file = utils::FileUtils::change_extension(input_file, output_format);
        } else {
            filename = utils::FileUtils::change_extension(filename, output_format);
            output_file = output_folder + "/" + filename;
        }

        // Check if output exists
        if (!overwrite_ && utils::FileUtils::file_exists(output_file)) {
            Logger::instance().warning("Skipping (file exists): " + output_file);
            result.failed++;
            result.failed_files.push_back(input_file);
            continue;
        }

        // Progress
        float progress = (float)(i + 1) / result.total_files * 100.0f;
        Logger::instance().progress(progress, filename);

        // Process file
        fconvert_error_t error = process_single_file(input_file, output_file, params);

        if (error == FCONVERT_OK) {
            result.successful++;
        } else {
            result.failed++;
            result.failed_files.push_back(input_file);

            if (!skip_errors_) {
                break;
            }
        }
    }

    return result;
}

BatchResult BatchProcessor::process_folder(
    const std::string& input_folder,
    const std::string& output_format,
    const std::string& output_folder,
    bool recursive,
    const ConversionParams& params) {

    Logger::instance().info("Scanning folder: " + input_folder);

    std::vector<std::string> files = utils::FileUtils::list_files(input_folder, recursive);

    Logger::instance().info("Found " + std::to_string(files.size()) + " files");

    return process_files(files, output_format, output_folder, params);
}

fconvert_error_t BatchProcessor::process_single_file(
    const std::string& input_path,
    const std::string& output_path,
    const ConversionParams& params) {

    return ConverterRegistry::instance().convert_file(input_path, output_path, params);
}

} // namespace core
} // namespace fconvert
