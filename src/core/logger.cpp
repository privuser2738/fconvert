/**
 * Logging system implementation
 */

#include "logger.h"
#include <cstdio>

namespace fconvert {
namespace core {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger()
    : level_(LogLevel::INFO)
    , verbose_(false)
    , quiet_(false)
    , color_output_(true) {
}

Logger::~Logger() {
}

void Logger::set_level(LogLevel level) {
    level_ = level;
}

void Logger::set_verbose(bool verbose) {
    verbose_ = verbose;
    if (verbose) {
        level_ = LogLevel::DEBUG;
    }
}

void Logger::set_quiet(bool quiet) {
    quiet_ = quiet;
}

void Logger::set_color_output(bool color) {
    color_output_ = color;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::progress(float percent, const std::string& message) {
    if (quiet_) return;

    int bar_width = 50;
    int filled = static_cast<int>(bar_width * percent / 100.0f);

    std::cout << "\r[";
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            std::cout << "=";
        } else if (i == filled) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] " << static_cast<int>(percent) << "%";

    if (!message.empty()) {
        std::cout << " " << message;
    }

    std::cout << std::flush;

    if (percent >= 100.0f) {
        std::cout << std::endl;
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (quiet_ && level != LogLevel::ERROR) {
        return;
    }

    if (level < level_) {
        return;
    }

    std::ostream& stream = (level == LogLevel::ERROR) ? std::cerr : std::cout;

    if (color_output_) {
        stream << get_color_code(level);
    }

    stream << "[" << get_level_string(level) << "] " << message;

    if (color_output_) {
        stream << "\033[0m";
    }

    stream << std::endl;
}

std::string Logger::get_level_string(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::get_color_code(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "\033[36m";    // Cyan
        case LogLevel::INFO: return "\033[32m";     // Green
        case LogLevel::WARNING: return "\033[33m";  // Yellow
        case LogLevel::ERROR: return "\033[31m";    // Red
        default: return "";
    }
}

} // namespace core
} // namespace fconvert
