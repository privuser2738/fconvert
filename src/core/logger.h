/**
 * Logging system
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <sstream>

namespace fconvert {
namespace core {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level);
    void set_verbose(bool verbose);
    void set_quiet(bool quiet);
    void set_color_output(bool color);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

    void progress(float percent, const std::string& message = "");

    template<typename... Args>
    void debug(const char* format, Args... args) {
        debug(format_string(format, args...));
    }

    template<typename... Args>
    void info(const char* format, Args... args) {
        info(format_string(format, args...));
    }

    template<typename... Args>
    void warning(const char* format, Args... args) {
        warning(format_string(format, args...));
    }

    template<typename... Args>
    void error(const char* format, Args... args) {
        error(format_string(format, args...));
    }

private:
    Logger();
    ~Logger();

    LogLevel level_;
    bool verbose_;
    bool quiet_;
    bool color_output_;

    void log(LogLevel level, const std::string& message);
    std::string get_level_string(LogLevel level) const;
    std::string get_color_code(LogLevel level) const;

    template<typename... Args>
    std::string format_string(const char* format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        return std::string(buffer);
    }
};

// Convenience macros
#define LOG_DEBUG(msg) fconvert::core::Logger::instance().debug(msg)
#define LOG_INFO(msg) fconvert::core::Logger::instance().info(msg)
#define LOG_WARNING(msg) fconvert::core::Logger::instance().warning(msg)
#define LOG_ERROR(msg) fconvert::core::Logger::instance().error(msg)

} // namespace core
} // namespace fconvert

#endif // LOGGER_H
