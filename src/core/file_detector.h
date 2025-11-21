/**
 * File type detection using magic numbers and extensions
 */

#ifndef FILE_DETECTOR_H
#define FILE_DETECTOR_H

#include "../../include/fconvert.h"
#include <string>
#include <vector>
#include <map>

namespace fconvert {
namespace core {

struct FileTypeInfo {
    file_type_category_t category;
    std::string extension;
    std::string mime_type;
    std::string description;
};

class FileDetector {
public:
    static FileDetector& instance();

    FileTypeInfo detect_from_file(const std::string& path);
    FileTypeInfo detect_from_extension(const std::string& extension);
    FileTypeInfo detect_from_magic(const std::vector<uint8_t>& header);

    bool is_supported(const std::string& extension);
    std::vector<std::string> get_supported_extensions(file_type_category_t category);

private:
    FileDetector();
    ~FileDetector();

    void register_formats();

    struct MagicSignature {
        std::vector<uint8_t> signature;
        size_t offset;
        FileTypeInfo info;
    };

    std::vector<MagicSignature> magic_signatures_;
    std::map<std::string, FileTypeInfo> extension_map_;
};

} // namespace core
} // namespace fconvert

#endif // FILE_DETECTOR_H
