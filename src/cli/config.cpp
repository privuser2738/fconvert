/**
 * Configuration file handler implementation
 */

#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif

namespace fconvert {
namespace cli {

Config::Config() {
}

Config::~Config() {
}

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    values_.clear();
    std::string line;
    while (std::getline(file, line)) {
        parse_line(line);
    }

    return true;
}

bool Config::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << "# fconvert configuration file\n\n";
    for (const auto& pair : values_) {
        file << pair.first << " = " << pair.second << "\n";
    }

    return true;
}

bool Config::open_in_editor() {
    std::string config_path = get_default_config_path();

    if (!create_default_config()) {
        std::cerr << "Failed to create config file at: " << config_path << "\n";
        return false;
    }

    std::cout << "Opening config file: " << config_path << "\n";

#ifdef _WIN32
    std::string command = "notepad \"" + config_path + "\"";
    system(command.c_str());
#elif __APPLE__
    std::string command = "open -t \"" + config_path + "\"";
    system(command.c_str());
#else
    // Try common Linux editors
    const char* editor = getenv("EDITOR");
    if (editor) {
        std::string command = std::string(editor) + " \"" + config_path + "\"";
        system(command.c_str());
    } else {
        std::string command = "xdg-open \"" + config_path + "\"";
        system(command.c_str());
    }
#endif

    return true;
}

void Config::parse_line(const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] == '#') {
        return;
    }

    size_t eq_pos = trimmed.find('=');
    if (eq_pos == std::string::npos) {
        return;
    }

    std::string key = trim(trimmed.substr(0, eq_pos));
    std::string value = trim(trimmed.substr(eq_pos + 1));

    values_[key] = value;
}

std::string Config::trim(const std::string& str) const {
    size_t start = 0;
    size_t end = str.length();

    while (start < end && std::isspace(str[start])) {
        start++;
    }

    while (end > start && std::isspace(str[end - 1])) {
        end--;
    }

    return str.substr(start, end - start);
}

std::string Config::get_string(const std::string& key, const std::string& default_value) const {
    auto it = values_.find(key);
    return (it != values_.end()) ? it->second : default_value;
}

int Config::get_int(const std::string& key, int default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    return std::stoi(it->second);
}

bool Config::get_bool(const std::string& key, bool default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    std::string value = it->second;
    return (value == "true" || value == "1" || value == "yes");
}

float Config::get_float(const std::string& key, float default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    return std::stof(it->second);
}

void Config::set_string(const std::string& key, const std::string& value) {
    values_[key] = value;
}

void Config::set_int(const std::string& key, int value) {
    values_[key] = std::to_string(value);
}

void Config::set_bool(const std::string& key, bool value) {
    values_[key] = value ? "true" : "false";
}

void Config::set_float(const std::string& key, float value) {
    values_[key] = std::to_string(value);
}

std::string Config::get_default_config_path() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::string(path) + "\\fconvert\\config.ini";
    }
    return "fconvert_config.ini";
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    return std::string(home) + "/.config/fconvert/config.ini";
#endif
}

bool Config::create_default_config() {
    std::string config_path = get_default_config_path();

    // Create directory if it doesn't exist
#ifdef _WIN32
    std::string dir = config_path.substr(0, config_path.find_last_of("\\/"));
    std::string mkdir_cmd = "mkdir \"" + dir + "\" 2>nul";
    system(mkdir_cmd.c_str());
#else
    std::string dir = config_path.substr(0, config_path.find_last_of("/"));
    std::string mkdir_cmd = "mkdir -p \"" + dir + "\"";
    system(mkdir_cmd.c_str());
#endif

    // Create default config if it doesn't exist
    std::ifstream test(config_path);
    if (test.good()) {
        test.close();
        return true;
    }

    std::ofstream file(config_path);
    if (!file.is_open()) {
        return false;
    }

    file << R"(# fconvert configuration file
# This file contains default settings for fconvert

# General settings
verbose = false
quiet = false
overwrite = false
show_statistics = true

# Image conversion defaults
image_quality = 85
image_keep_aspect_ratio = true

# Audio conversion defaults
audio_sample_rate = 44100
audio_bitrate = 192
audio_channels = 2

# Video conversion defaults
video_fps = 30
video_bitrate = 2000
video_codec = h264

# Batch processing
batch_recursive = false
batch_skip_errors = true

# Performance
thread_count = 0  # 0 = auto-detect

# Output
color_output = true
progress_bar = true
)";

    file.close();
    return true;
}

} // namespace cli
} // namespace fconvert
