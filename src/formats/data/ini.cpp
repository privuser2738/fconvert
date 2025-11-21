/**
 * INI format implementation
 */

#include "ini.h"
#include <sstream>
#include <algorithm>

namespace fconvert {
namespace formats {

std::string INICodec::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool INICodec::is_ini(const uint8_t* data, size_t size) {
    if (size < 3) return false;

    // Check for common INI patterns
    std::string content(reinterpret_cast<const char*>(data),
                       std::min(size, size_t(1024)));

    // Look for [section] or key=value patterns
    return (content.find('[') != std::string::npos &&
            content.find(']') != std::string::npos) ||
           (content.find('=') != std::string::npos);
}

fconvert_error_t INICodec::decode(
    const std::vector<uint8_t>& data,
    IniData& ini) {

    if (data.empty()) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    ini.sections.clear();
    ini.global.clear();

    std::string content(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream iss(content);
    std::string line;
    std::string current_section;

    while (std::getline(iss, line)) {
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        // Check for section header
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                current_section = trim(line.substr(1, end - 1));
                // Create section if it doesn't exist
                ini.sections[current_section];
            }
            continue;
        }

        // Parse key=value
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = trim(line.substr(0, eq_pos));
            std::string value = trim(line.substr(eq_pos + 1));

            // Remove quotes if present
            if (value.size() >= 2) {
                if ((value.front() == '"' && value.back() == '"') ||
                    (value.front() == '\'' && value.back() == '\'')) {
                    value = value.substr(1, value.size() - 2);
                }
            }

            if (current_section.empty()) {
                ini.global[key] = value;
            } else {
                ini.sections[current_section][key] = value;
            }
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t INICodec::encode(
    const IniData& ini,
    std::vector<uint8_t>& data) {

    std::ostringstream oss;

    // Write global keys first
    for (const auto& [key, value] : ini.global) {
        oss << key << " = " << value << "\n";
    }

    if (!ini.global.empty() && !ini.sections.empty()) {
        oss << "\n";
    }

    // Write sections
    bool first_section = true;
    for (const auto& [section, keys] : ini.sections) {
        if (!first_section) {
            oss << "\n";
        }
        first_section = false;

        oss << "[" << section << "]\n";
        for (const auto& [key, value] : keys) {
            oss << key << " = " << value << "\n";
        }
    }

    std::string result = oss.str();
    data.assign(result.begin(), result.end());

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
