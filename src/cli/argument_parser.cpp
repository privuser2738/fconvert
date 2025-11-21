/**
 * Command-line argument parser implementation
 */

#include "argument_parser.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace fconvert {
namespace cli {

ArgumentParser::ArgumentParser() {
}

ArgumentParser::~ArgumentParser() {
}

bool ArgumentParser::parse(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return false;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_help();
            return false;
        }
        else if (arg == "-v" || arg == "--version") {
            print_version();
            return false;
        }
        else if (arg == "--formats") {
            print_supported_formats();
            return false;
        }
        else if (arg == "--openfile" || arg == "--open-config") {
            options_.open_config = true;
        }
        else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                options_.input_file = argv[++i];
            }
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                options_.output_file = argv[++i];
            }
        }
        else if (arg == "-f" || arg == "--from") {
            if (i + 1 < argc) {
                options_.input_format = argv[++i];
                std::transform(options_.input_format.begin(), options_.input_format.end(),
                             options_.input_format.begin(), [](unsigned char c){ return std::tolower(c); });
            }
        }
        else if (arg == "-t" || arg == "--to") {
            if (i + 1 < argc) {
                options_.output_format = argv[++i];
                std::transform(options_.output_format.begin(), options_.output_format.end(),
                             options_.output_format.begin(), [](unsigned char c){ return std::tolower(c); });
            }
        }
        else if (arg == "-q" || arg == "--quality") {
            if (i + 1 < argc) {
                options_.quality = std::stoi(argv[++i]);
            }
        }
        else if (arg == "--lossless") {
            options_.lossless = true;
        }
        else if (arg == "--width") {
            if (i + 1 < argc) {
                options_.width = std::stoi(argv[++i]);
            }
        }
        else if (arg == "--height") {
            if (i + 1 < argc) {
                options_.height = std::stoi(argv[++i]);
            }
        }
        else if (arg == "--rotate") {
            if (i + 1 < argc) {
                options_.rotate = std::stoi(argv[++i]);
            }
        }
        else if (arg == "--flip-horizontal" || arg == "--flip-h") {
            options_.flip_horizontal = true;
        }
        else if (arg == "--flip-vertical" || arg == "--flip-v") {
            options_.flip_vertical = true;
        }
        else if (arg == "--interpolation") {
            if (i + 1 < argc) {
                std::string method = argv[++i];
                std::transform(method.begin(), method.end(), method.begin(),
                             [](unsigned char c){ return std::tolower(c); });
                if (method == "nearest") options_.interpolation = 0;
                else if (method == "bilinear") options_.interpolation = 1;
                else if (method == "bicubic") options_.interpolation = 2;
            }
        }
        else if (arg == "--sample-rate") {
            if (i + 1 < argc) {
                options_.sample_rate = std::stoi(argv[++i]);
            }
        }
        else if (arg == "--bitrate") {
            if (i + 1 < argc) {
                options_.bitrate = std::stoi(argv[++i]);
            }
        }
        else if (arg == "--channels") {
            if (i + 1 < argc) {
                options_.channels = std::stoi(argv[++i]);
            }
        }
        else if (arg == "--fps") {
            if (i + 1 < argc) {
                options_.fps = std::stoi(argv[++i]);
            }
        }
        else if (arg == "--codec") {
            if (i + 1 < argc) {
                options_.codec = argv[++i];
            }
        }
        else if (arg == "--batch-files") {
            options_.batch_mode = BatchMode::FILES;
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                options_.input_files.push_back(argv[++i]);
            }
        }
        else if (arg == "--batch-folder") {
            if (i + 1 < argc) {
                options_.batch_mode = BatchMode::FOLDER;
                options_.input_folder = argv[++i];
            }
        }
        else if (arg == "--batch-recursive" || arg == "-r") {
            if (i + 1 < argc) {
                options_.batch_mode = BatchMode::RECURSIVE;
                options_.input_folder = argv[++i];
            }
        }
        else if (arg == "--output-folder") {
            if (i + 1 < argc) {
                options_.output_folder = argv[++i];
            }
        }
        else if (arg == "--verbose") {
            options_.verbose = true;
        }
        else if (arg == "--quiet") {
            options_.quiet = true;
        }
        else if (arg == "--overwrite" || arg == "-y") {
            options_.overwrite = true;
        }
        else if (arg == "--no-stats") {
            options_.show_statistics = false;
        }
        else if (arg == "--config") {
            if (i + 1 < argc) {
                options_.config_file = argv[++i];
            }
        }
        else if (arg[0] != '-') {
            // Positional argument (input file)
            if (options_.input_file.empty()) {
                options_.input_file = arg;
            } else if (options_.output_file.empty()) {
                options_.output_file = arg;
            }
        }
    }

    detect_formats_from_filenames();
    return validate_options();
}

void ArgumentParser::detect_formats_from_filenames() {
    if (!options_.input_file.empty() && options_.input_format.empty()) {
        size_t dot_pos = options_.input_file.find_last_of('.');
        if (dot_pos != std::string::npos) {
            options_.input_format = options_.input_file.substr(dot_pos + 1);
            std::transform(options_.input_format.begin(), options_.input_format.end(),
                         options_.input_format.begin(), [](unsigned char c){ return std::tolower(c); });
        }
    }

    if (!options_.output_file.empty() && options_.output_format.empty()) {
        size_t dot_pos = options_.output_file.find_last_of('.');
        if (dot_pos != std::string::npos) {
            options_.output_format = options_.output_file.substr(dot_pos + 1);
            std::transform(options_.output_format.begin(), options_.output_format.end(),
                         options_.output_format.begin(), [](unsigned char c){ return std::tolower(c); });
        }
    }
}

bool ArgumentParser::validate_options() {
    if (options_.open_config) {
        return true;
    }

    if (options_.batch_mode == BatchMode::NONE && options_.input_file.empty()) {
        std::cerr << "Error: No input file specified\n";
        return false;
    }

    if (options_.batch_mode == BatchMode::FILES && options_.input_files.empty()) {
        std::cerr << "Error: No input files specified for batch processing\n";
        return false;
    }

    if ((options_.batch_mode == BatchMode::FOLDER || options_.batch_mode == BatchMode::RECURSIVE)
        && options_.input_folder.empty()) {
        std::cerr << "Error: No input folder specified for batch processing\n";
        return false;
    }

    if (options_.output_format.empty()) {
        std::cerr << "Error: No output format specified\n";
        return false;
    }

    return true;
}

void ArgumentParser::print_help() const {
    std::cout << R"(
fconvert - Enterprise-grade file conversion tool
Version 1.0.0

USAGE:
    fconvert [OPTIONS] <input> <output>
    fconvert -i <input> -o <output> [OPTIONS]

OPTIONS:
    -h, --help              Show this help message
    -v, --version           Show version information
    --formats               List all supported formats

INPUT/OUTPUT:
    -i, --input <file>      Input file
    -o, --output <file>     Output file
    -f, --from <format>     Input format (auto-detected if not specified)
    -t, --to <format>       Output format (required)

BATCH PROCESSING:
    --batch-files <files...>       Convert multiple files
    --batch-folder <folder>        Convert all files in folder
    -r, --batch-recursive <folder> Recursively convert all files
    --output-folder <folder>       Output folder for batch processing

QUALITY SETTINGS:
    -q, --quality <0-100>   Conversion quality (default: 85)
    --lossless              Use lossless compression

IMAGE OPTIONS:
    --width <pixels>        Output width
    --height <pixels>       Output height
    --rotate <degrees>      Rotate image (0, 90, 180, 270)
    --flip-h, --flip-horizontal   Flip image horizontally
    --flip-v, --flip-vertical     Flip image vertically
    --interpolation <method>      Resize method: nearest, bilinear, bicubic

AUDIO OPTIONS:
    --sample-rate <hz>      Sample rate (default: 44100)
    --bitrate <kbps>        Audio bitrate (default: 192)
    --channels <1|2>        Number of channels (default: 2)

VIDEO OPTIONS:
    --fps <fps>             Frames per second (default: 30)
    --bitrate <kbps>        Video bitrate (default: 2000)
    --codec <name>          Video codec

GENERAL OPTIONS:
    --verbose               Enable verbose output
    --quiet                 Suppress all output except errors
    -y, --overwrite         Overwrite existing files without prompting
    --no-stats              Don't show conversion statistics
    --config <file>         Use custom configuration file
    --openfile, --open-config    Open configuration file

EXAMPLES:
    # Convert single image
    fconvert input.png output.jpg

    # Convert with quality setting
    fconvert -i input.jpg -o output.png -q 95

    # Batch convert all PNGs to JPG
    fconvert --batch-folder ./images --to jpg

    # Convert audio file
    fconvert song.wav song.mp3 --bitrate 320

    # Convert video
    fconvert video.avi video.mp4 --codec h264

    # Recursive batch conversion
    fconvert -r ./photos --to webp --quality 90

SUPPORTED FILE TYPES:
    Images:         PNG, BMP, JPG, JPEG, GIF, WebP, TIFF, TGA
    Audio:          MP3, WAV, OGG, FLAC, AAC, ALAC, OPUS, WMA
    Video:          MP4, AVI, WEBM, MOV, MKV, FLV
    3D Models:      OBJ, FBX, STL, BLEND, DAE, GLTF, PLY
    Archives:       ZIP, 7Z, TAR, GZ, BZ2, XZ, ISO
    Documents:      PDF, DOCX, TXT, RTF, ODT, EPUB, HTML, MD
    Spreadsheets:   XLSX, CSV, ODS, TSV
    Vectors:        SVG, AI, EPS
    Fonts:          TTF, OTF, WOFF, WOFF2
    Data:           JSON, XML, YAML, TOML, INI
    And many more...

For full documentation, visit: https://github.com/yourusername/fconvert
)" << std::endl;
}

void ArgumentParser::print_version() const {
    std::cout << "fconvert version 1.0.0\n";
    std::cout << "Enterprise-grade file conversion tool\n";
    std::cout << "Built: " << __DATE__ << " " << __TIME__ << "\n";
}

void ArgumentParser::print_supported_formats() const {
    std::cout << R"(
SUPPORTED FILE FORMATS:

IMAGE FORMATS:
  Input/Output: PNG, BMP, JPG, JPEG, GIF, WebP, TIFF, TGA, PPM, PGM, PBM

AUDIO FORMATS:
  Input/Output: WAV, MP3, OGG, FLAC, AAC, ALAC, OPUS, WMA, M4A, AIFF

VIDEO FORMATS:
  Input/Output: MP4, AVI, WEBM, MOV, MKV, FLV, WMV, MPEG

3D MODEL FORMATS:
  Input/Output: OBJ, STL, PLY, OFF
  Input only: FBX, BLEND, DAE, GLTF, 3DS

ARCHIVE/COMPRESSION:
  Input/Output: ZIP, TAR, GZ, BZ2, XZ
  Input only: 7Z, RAR, ISO

DOCUMENT FORMATS:
  Input/Output: TXT, MD, HTML, RTF
  Input only: PDF, DOCX, ODT, EPUB

SPREADSHEET FORMATS:
  Input/Output: CSV, TSV
  Input only: XLSX, ODS

VECTOR GRAPHICS:
  Input/Output: SVG
  Input only: AI, EPS

FONT FORMATS:
  Input/Output: TTF, OTF, WOFF, WOFF2

DATA FORMATS:
  Input/Output: JSON, XML, YAML, TOML, INI, CSV

SUBTITLE FORMATS:
  Input/Output: SRT, VTT, ASS, SUB

Note: Some formats may have limited conversion options due to format complexity.
)" << std::endl;
}

} // namespace cli
} // namespace fconvert
