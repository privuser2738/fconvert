/**
 * fconvert - Enterprise-grade file conversion tool
 * Main entry point
 */

#include "cli/argument_parser.h"
#include "cli/config.h"
#include "core/logger.h"
#include "core/converter.h"
#include "core/batch_processor.h"
#include "core/file_detector.h"
#include "utils/file_utils.h"
#include "formats/image/image_converter.h"
#include "formats/audio/audio_converter.h"
#include "formats/video/video_converter.h"
#include "formats/archive/archive_converter.h"
#include "formats/model3d/model3d_converter.h"
#include "formats/document/document_converter.h"

#include <iostream>
#include <iomanip>
#include <memory>

using namespace fconvert;

void register_converters() {
    // Register all format converters
    core::ConverterRegistry::instance().register_converter(
        std::make_shared<formats::ImageConverter>());

    core::ConverterRegistry::instance().register_converter(
        std::make_shared<formats::AudioConverter>());

    core::ConverterRegistry::instance().register_converter(
        std::make_shared<formats::VideoConverter>());

    core::ConverterRegistry::instance().register_converter(
        std::make_shared<formats::ArchiveConverter>());

    core::ConverterRegistry::instance().register_converter(
        std::make_shared<formats::Model3dConverter>());

    core::ConverterRegistry::instance().register_converter(
        std::make_shared<formats::DocumentConverter>());

    // TODO: Register other converters as they are implemented
    // etc.
}

void print_statistics(const std::string& input_file, const std::string& output_file) {
    uint64_t input_size = utils::FileUtils::get_file_size(input_file);
    uint64_t output_size = utils::FileUtils::get_file_size(output_file);

    std::cout << "\nConversion Statistics:\n";
    std::cout << "  Input file:  " << input_file << " (" << input_size << " bytes)\n";
    std::cout << "  Output file: " << output_file << " (" << output_size << " bytes)\n";

    if (input_size > 0) {
        float ratio = (float)output_size / input_size * 100.0f;
        std::cout << "  Size ratio:  " << std::fixed << std::setprecision(1) << ratio << "%\n";
    }
}

int main(int argc, char** argv) {
    // Parse arguments
    cli::ArgumentParser parser;
    if (!parser.parse(argc, argv)) {
        return 1;
    }

    const cli::ConversionOptions& options = parser.get_options();

    // Handle config file opening
    if (options.open_config) {
        cli::Config config;
        return config.open_in_editor() ? 0 : 1;
    }

    // Load config if specified
    cli::Config config;
    if (!options.config_file.empty()) {
        if (!config.load(options.config_file)) {
            std::cerr << "Warning: Failed to load config file: " << options.config_file << "\n";
        }
    } else {
        // Try to load default config
        std::string default_config = cli::Config::get_default_config_path();
        if (utils::FileUtils::file_exists(default_config)) {
            config.load(default_config);
        }
    }

    // Configure logger
    core::Logger::instance().set_verbose(options.verbose);
    core::Logger::instance().set_quiet(options.quiet);

    // Register all converters
    register_converters();

    // Set up conversion parameters
    core::ConversionParams params;
    params.quality = options.quality;
    params.lossless = options.lossless;
    params.width = options.width;
    params.height = options.height;
    params.keep_aspect_ratio = options.keep_aspect_ratio;
    params.rotate = options.rotate;
    params.flip_horizontal = options.flip_horizontal;
    params.flip_vertical = options.flip_vertical;
    params.interpolation = options.interpolation;
    params.sample_rate = options.sample_rate;
    params.bitrate = options.bitrate;
    params.channels = options.channels;
    params.fps = options.fps;
    params.video_bitrate = options.video_bitrate;
    params.codec = options.codec;

    // Handle batch processing
    if (options.batch_mode != cli::BatchMode::NONE) {
        core::BatchProcessor processor;
        processor.set_overwrite(options.overwrite);

        core::BatchResult result;

        if (options.batch_mode == cli::BatchMode::FILES) {
            result = processor.process_files(
                options.input_files,
                options.output_format,
                options.output_folder,
                params);
        } else if (options.batch_mode == cli::BatchMode::FOLDER ||
                   options.batch_mode == cli::BatchMode::RECURSIVE) {
            bool recursive = (options.batch_mode == cli::BatchMode::RECURSIVE);
            result = processor.process_folder(
                options.input_folder,
                options.output_format,
                options.output_folder,
                recursive,
                params);
        }

        // Print batch results
        std::cout << "\nBatch Conversion Results:\n";
        std::cout << "  Total files:      " << result.total_files << "\n";
        std::cout << "  Successful:       " << result.successful << "\n";
        std::cout << "  Failed:           " << result.failed << "\n";

        if (!result.failed_files.empty() && options.verbose) {
            std::cout << "\nFailed files:\n";
            for (const auto& file : result.failed_files) {
                std::cout << "  - " << file << "\n";
            }
        }

        return (result.failed == 0) ? 0 : 1;
    }

    // Single file conversion
    fconvert_error_t error = core::ConverterRegistry::instance().convert_file(
        options.input_file,
        options.output_file,
        params);

    if (error != FCONVERT_OK) {
        std::cerr << "Conversion failed with error code: " << error << "\n";
        return 1;
    }

    // Print statistics if enabled
    if (options.show_statistics) {
        print_statistics(options.input_file, options.output_file);
    }

    return 0;
}
