/**
 * Command-line argument parser
 */

#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <string>
#include <vector>
#include <map>

namespace fconvert {
namespace cli {

enum class BatchMode {
    NONE,
    FILES,
    FOLDER,
    RECURSIVE
};

struct ConversionOptions {
    std::string input_file;
    std::string output_file;
    std::string input_format;
    std::string output_format;

    // Batch processing
    BatchMode batch_mode = BatchMode::NONE;
    std::vector<std::string> input_files;
    std::string input_folder;
    std::string output_folder;

    // Quality settings
    int quality = 85; // 0-100
    bool lossless = false;

    // Image-specific
    int width = -1;
    int height = -1;
    bool keep_aspect_ratio = true;
    int rotate = 0;  // Rotation degrees (0, 90, 180, 270)
    bool flip_horizontal = false;
    bool flip_vertical = false;
    int interpolation = 1;  // 0=nearest, 1=bilinear, 2=bicubic

    // Audio-specific
    int sample_rate = 44100;
    int bitrate = 192; // kbps
    int channels = 2;

    // Video-specific
    int fps = 30;
    int video_bitrate = 2000; // kbps
    std::string codec;

    // General options
    bool verbose = false;
    bool quiet = false;
    bool overwrite = false;
    bool show_statistics = true;
    bool use_defaults = true;
    std::string config_file;
    bool open_config = false;

    // Advanced
    std::map<std::string, std::string> custom_params;
};

class ArgumentParser {
public:
    ArgumentParser();
    ~ArgumentParser();

    bool parse(int argc, char** argv);
    const ConversionOptions& get_options() const { return options_; }

    void print_help() const;
    void print_version() const;
    void print_supported_formats() const;

private:
    ConversionOptions options_;

    void detect_formats_from_filenames();
    bool validate_options();
};

} // namespace cli
} // namespace fconvert

#endif // ARGUMENT_PARSER_H
