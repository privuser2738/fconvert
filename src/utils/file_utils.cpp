/**
 * File utility functions implementation
 */

#include "file_utils.h"
#include <fstream>
#include <algorithm>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define stat _stat
    #define S_IFDIR _S_IFDIR
    #define mkdir _mkdir
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif

namespace fconvert {
namespace utils {

bool FileUtils::file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool FileUtils::is_directory(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    return (buffer.st_mode & S_IFDIR) != 0;
}

bool FileUtils::create_directory(const std::string& path) {
#ifdef _WIN32
    return mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

bool FileUtils::create_directories(const std::string& path) {
    std::string current_path;
    for (size_t i = 0; i < path.length(); i++) {
        if (path[i] == '/' || path[i] == '\\') {
            if (!current_path.empty()) {
                create_directory(current_path);
            }
        }
        current_path += path[i];
    }
    return create_directory(path);
}

std::string FileUtils::get_file_extension(const std::string& path) {
    size_t dot_pos = path.find_last_of('.');
    size_t slash_pos = path.find_last_of("/\\");

    if (dot_pos != std::string::npos &&
        (slash_pos == std::string::npos || dot_pos > slash_pos)) {
        std::string ext = path.substr(dot_pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
        return ext;
    }
    return "";
}

std::string FileUtils::get_filename(const std::string& path) {
    size_t slash_pos = path.find_last_of("/\\");
    if (slash_pos != std::string::npos) {
        return path.substr(slash_pos + 1);
    }
    return path;
}

std::string FileUtils::get_directory(const std::string& path) {
    size_t slash_pos = path.find_last_of("/\\");
    if (slash_pos != std::string::npos) {
        return path.substr(0, slash_pos);
    }
    return ".";
}

std::string FileUtils::change_extension(const std::string& path, const std::string& new_ext) {
    size_t dot_pos = path.find_last_of('.');
    size_t slash_pos = path.find_last_of("/\\");

    if (dot_pos != std::string::npos &&
        (slash_pos == std::string::npos || dot_pos > slash_pos)) {
        return path.substr(0, dot_pos + 1) + new_ext;
    }
    return path + "." + new_ext;
}

uint64_t FileUtils::get_file_size(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return 0;
    }
    return buffer.st_size;
}

std::vector<std::string> FileUtils::list_files(const std::string& directory, bool recursive) {
    std::vector<std::string> files;

#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    std::string search_path = directory + "\\*";
    HANDLE handle = FindFirstFileA(search_path.c_str(), &find_data);

    if (handle == INVALID_HANDLE_VALUE) {
        return files;
    }

    do {
        std::string name = find_data.cFileName;
        if (name != "." && name != "..") {
            std::string full_path = directory + "\\" + name;
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (recursive) {
                    auto sub_files = list_files(full_path, true);
                    files.insert(files.end(), sub_files.begin(), sub_files.end());
                }
            } else {
                files.push_back(full_path);
            }
        }
    } while (FindNextFileA(handle, &find_data));

    FindClose(handle);
#else
    DIR* dir = opendir(directory.c_str());
    if (!dir) {
        return files;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            std::string full_path = directory + "/" + name;
            if (is_directory(full_path)) {
                if (recursive) {
                    auto sub_files = list_files(full_path, true);
                    files.insert(files.end(), sub_files.begin(), sub_files.end());
                }
            } else {
                files.push_back(full_path);
            }
        }
    }

    closedir(dir);
#endif

    return files;
}

std::vector<std::string> FileUtils::list_files_with_extension(const std::string& directory,
                                                              const std::string& extension,
                                                              bool recursive) {
    std::vector<std::string> all_files = list_files(directory, recursive);
    std::vector<std::string> filtered_files;

    std::string ext_lower = extension;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

    for (const auto& file : all_files) {
        if (get_file_extension(file) == ext_lower) {
            filtered_files.push_back(file);
        }
    }

    return filtered_files;
}

bool FileUtils::read_file(const std::string& path, std::vector<uint8_t>& data) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    data.resize(size);
    return file.read(reinterpret_cast<char*>(data.data()), size).good();
}

bool FileUtils::write_file(const std::string& path, const std::vector<uint8_t>& data) {
    // Create directory if it doesn't exist
    std::string dir = get_directory(path);
    if (!dir.empty() && dir != ".") {
        create_directories(dir);
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

std::string FileUtils::get_temp_directory() {
#ifdef _WIN32
    char temp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_path);
    return std::string(temp_path);
#else
    const char* temp = getenv("TMPDIR");
    if (!temp) temp = "/tmp";
    return std::string(temp);
#endif
}

std::string FileUtils::get_user_directory() {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    if (!home) home = getenv("HOMEPATH");
    return home ? std::string(home) : "";
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    return home ? std::string(home) : "";
#endif
}

} // namespace utils
} // namespace fconvert
