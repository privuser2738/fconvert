/**
 * INI (Configuration) format
 */

#ifndef INI_H
#define INI_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <map>

namespace fconvert {
namespace formats {

// INI data structure
struct IniData {
    // Section name -> (key -> value)
    std::map<std::string, std::map<std::string, std::string>> sections;

    // Global keys (no section)
    std::map<std::string, std::string> global;
};

/**
 * INI codec
 */
class INICodec {
public:
    /**
     * Decode INI file
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        IniData& ini);

    /**
     * Encode INI data
     */
    static fconvert_error_t encode(
        const IniData& ini,
        std::vector<uint8_t>& data);

    /**
     * Check if data is INI format
     */
    static bool is_ini(const uint8_t* data, size_t size);

private:
    // Trim whitespace from string
    static std::string trim(const std::string& str);
};

} // namespace formats
} // namespace fconvert

#endif // INI_H
