/**
 * Configuration file handler
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>

namespace fconvert {
namespace cli {

class Config {
public:
    Config();
    ~Config();

    bool load(const std::string& filename);
    bool save(const std::string& filename);
    bool open_in_editor();

    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    int get_int(const std::string& key, int default_value = 0) const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    float get_float(const std::string& key, float default_value = 0.0f) const;

    void set_string(const std::string& key, const std::string& value);
    void set_int(const std::string& key, int value);
    void set_bool(const std::string& key, bool value);
    void set_float(const std::string& key, float value);

    static std::string get_default_config_path();
    static bool create_default_config();

private:
    std::map<std::string, std::string> values_;

    void parse_line(const std::string& line);
    std::string trim(const std::string& str) const;
};

} // namespace cli
} // namespace fconvert

#endif // CONFIG_H
