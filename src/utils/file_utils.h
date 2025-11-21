/**
 * File utility functions
 */

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace fconvert {
namespace utils {

class FileUtils {
public:
    static bool file_exists(const std::string& path);
    static bool is_directory(const std::string& path);
    static bool create_directory(const std::string& path);
    static bool create_directories(const std::string& path);

    static std::string get_file_extension(const std::string& path);
    static std::string get_filename(const std::string& path);
    static std::string get_directory(const std::string& path);
    static std::string change_extension(const std::string& path, const std::string& new_ext);

    static uint64_t get_file_size(const std::string& path);
    static std::vector<std::string> list_files(const std::string& directory, bool recursive = false);
    static std::vector<std::string> list_files_with_extension(const std::string& directory,
                                                              const std::string& extension,
                                                              bool recursive = false);

    static bool read_file(const std::string& path, std::vector<uint8_t>& data);
    static bool write_file(const std::string& path, const std::vector<uint8_t>& data);

    static std::string get_temp_directory();
    static std::string get_user_directory();
};

} // namespace utils
} // namespace fconvert

#endif // FILE_UTILS_H
