/**
 * File type detection implementation
 */

#include "file_detector.h"
#include "../utils/file_utils.h"
#include <fstream>
#include <algorithm>
#include <map>

namespace fconvert {
namespace core {

FileDetector& FileDetector::instance() {
    static FileDetector detector;
    return detector;
}

FileDetector::FileDetector() {
    register_formats();
}

FileDetector::~FileDetector() {
}

void FileDetector::register_formats() {
    // Image formats
    extension_map_["png"] = {FILE_TYPE_IMAGE, "png", "image/png", "Portable Network Graphics"};
    extension_map_["jpg"] = {FILE_TYPE_IMAGE, "jpg", "image/jpeg", "JPEG Image"};
    extension_map_["jpeg"] = {FILE_TYPE_IMAGE, "jpeg", "image/jpeg", "JPEG Image"};
    extension_map_["bmp"] = {FILE_TYPE_IMAGE, "bmp", "image/bmp", "Bitmap Image"};
    extension_map_["gif"] = {FILE_TYPE_IMAGE, "gif", "image/gif", "GIF Image"};
    extension_map_["webp"] = {FILE_TYPE_IMAGE, "webp", "image/webp", "WebP Image"};
    extension_map_["tiff"] = {FILE_TYPE_IMAGE, "tiff", "image/tiff", "TIFF Image"};
    extension_map_["tif"] = {FILE_TYPE_IMAGE, "tif", "image/tiff", "TIFF Image"};
    extension_map_["tga"] = {FILE_TYPE_IMAGE, "tga", "image/tga", "Targa Image"};
    extension_map_["ppm"] = {FILE_TYPE_IMAGE, "ppm", "image/x-portable-pixmap", "PPM Image"};
    extension_map_["pgm"] = {FILE_TYPE_IMAGE, "pgm", "image/x-portable-graymap", "PGM Image"};
    extension_map_["pbm"] = {FILE_TYPE_IMAGE, "pbm", "image/x-portable-bitmap", "PBM Image"};

    // Audio formats
    extension_map_["wav"] = {FILE_TYPE_AUDIO, "wav", "audio/wav", "WAV Audio"};
    extension_map_["mp3"] = {FILE_TYPE_AUDIO, "mp3", "audio/mpeg", "MP3 Audio"};
    extension_map_["ogg"] = {FILE_TYPE_AUDIO, "ogg", "audio/ogg", "OGG Vorbis"};
    extension_map_["flac"] = {FILE_TYPE_AUDIO, "flac", "audio/flac", "FLAC Lossless"};
    extension_map_["aac"] = {FILE_TYPE_AUDIO, "aac", "audio/aac", "AAC Audio"};
    extension_map_["m4a"] = {FILE_TYPE_AUDIO, "m4a", "audio/mp4", "M4A Audio"};
    extension_map_["wma"] = {FILE_TYPE_AUDIO, "wma", "audio/x-ms-wma", "WMA Audio"};
    extension_map_["opus"] = {FILE_TYPE_AUDIO, "opus", "audio/opus", "Opus Audio"};
    extension_map_["aiff"] = {FILE_TYPE_AUDIO, "aiff", "audio/aiff", "AIFF Audio"};

    // Video formats
    extension_map_["mp4"] = {FILE_TYPE_VIDEO, "mp4", "video/mp4", "MP4 Video"};
    extension_map_["avi"] = {FILE_TYPE_VIDEO, "avi", "video/x-msvideo", "AVI Video"};
    extension_map_["webm"] = {FILE_TYPE_VIDEO, "webm", "video/webm", "WebM Video"};
    extension_map_["mov"] = {FILE_TYPE_VIDEO, "mov", "video/quicktime", "QuickTime Video"};
    extension_map_["mkv"] = {FILE_TYPE_VIDEO, "mkv", "video/x-matroska", "Matroska Video"};
    extension_map_["flv"] = {FILE_TYPE_VIDEO, "flv", "video/x-flv", "Flash Video"};
    extension_map_["wmv"] = {FILE_TYPE_VIDEO, "wmv", "video/x-ms-wmv", "Windows Media Video"};
    extension_map_["mpeg"] = {FILE_TYPE_VIDEO, "mpeg", "video/mpeg", "MPEG Video"};
    extension_map_["mpg"] = {FILE_TYPE_VIDEO, "mpg", "video/mpeg", "MPEG Video"};

    // 3D Model formats
    extension_map_["obj"] = {FILE_TYPE_MODEL3D, "obj", "model/obj", "Wavefront OBJ"};
    extension_map_["stl"] = {FILE_TYPE_MODEL3D, "stl", "model/stl", "STL Model"};
    extension_map_["fbx"] = {FILE_TYPE_MODEL3D, "fbx", "model/fbx", "FBX Model"};
    extension_map_["dae"] = {FILE_TYPE_MODEL3D, "dae", "model/vnd.collada+xml", "COLLADA"};
    extension_map_["blend"] = {FILE_TYPE_MODEL3D, "blend", "application/x-blender", "Blender File"};
    extension_map_["gltf"] = {FILE_TYPE_MODEL3D, "gltf", "model/gltf+json", "glTF"};
    extension_map_["glb"] = {FILE_TYPE_MODEL3D, "glb", "model/gltf-binary", "glTF Binary"};
    extension_map_["ply"] = {FILE_TYPE_MODEL3D, "ply", "model/ply", "PLY Format"};
    extension_map_["3ds"] = {FILE_TYPE_MODEL3D, "3ds", "model/3ds", "3DS Max"};

    // Archive formats
    extension_map_["zip"] = {FILE_TYPE_ARCHIVE, "zip", "application/zip", "ZIP Archive"};
    extension_map_["7z"] = {FILE_TYPE_ARCHIVE, "7z", "application/x-7z-compressed", "7-Zip Archive"};
    extension_map_["tar"] = {FILE_TYPE_ARCHIVE, "tar", "application/x-tar", "TAR Archive"};
    extension_map_["gz"] = {FILE_TYPE_ARCHIVE, "gz", "application/gzip", "GZip Archive"};
    extension_map_["tgz"] = {FILE_TYPE_ARCHIVE, "tgz", "application/x-gzip", "TAR.GZ Archive"};
    extension_map_["tar.gz"] = {FILE_TYPE_ARCHIVE, "tgz", "application/x-gzip", "TAR.GZ Archive"};
    extension_map_["bz2"] = {FILE_TYPE_ARCHIVE, "bz2", "application/x-bzip2", "BZip2 Archive"};
    extension_map_["xz"] = {FILE_TYPE_ARCHIVE, "xz", "application/x-xz", "XZ Archive"};
    extension_map_["rar"] = {FILE_TYPE_ARCHIVE, "rar", "application/x-rar", "RAR Archive"};
    extension_map_["iso"] = {FILE_TYPE_ARCHIVE, "iso", "application/x-iso9660-image", "ISO Image"};

    // Document formats
    extension_map_["pdf"] = {FILE_TYPE_DOCUMENT, "pdf", "application/pdf", "PDF Document"};
    extension_map_["docx"] = {FILE_TYPE_DOCUMENT, "docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document", "Word Document"};
    extension_map_["doc"] = {FILE_TYPE_DOCUMENT, "doc", "application/msword", "Word Document"};
    extension_map_["txt"] = {FILE_TYPE_DOCUMENT, "txt", "text/plain", "Text File"};
    extension_map_["rtf"] = {FILE_TYPE_DOCUMENT, "rtf", "application/rtf", "Rich Text Format"};
    extension_map_["odt"] = {FILE_TYPE_DOCUMENT, "odt", "application/vnd.oasis.opendocument.text", "OpenDocument Text"};
    extension_map_["epub"] = {FILE_TYPE_EBOOK, "epub", "application/epub+zip", "EPUB eBook"};
    extension_map_["html"] = {FILE_TYPE_DOCUMENT, "html", "text/html", "HTML Document"};
    extension_map_["htm"] = {FILE_TYPE_DOCUMENT, "htm", "text/html", "HTML Document"};
    extension_map_["md"] = {FILE_TYPE_DOCUMENT, "md", "text/markdown", "Markdown"};

    // Spreadsheet formats
    extension_map_["xlsx"] = {FILE_TYPE_SPREADSHEET, "xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", "Excel Spreadsheet"};
    extension_map_["xls"] = {FILE_TYPE_SPREADSHEET, "xls", "application/vnd.ms-excel", "Excel Spreadsheet"};
    extension_map_["csv"] = {FILE_TYPE_SPREADSHEET, "csv", "text/csv", "CSV File"};
    extension_map_["ods"] = {FILE_TYPE_SPREADSHEET, "ods", "application/vnd.oasis.opendocument.spreadsheet", "OpenDocument Spreadsheet"};
    extension_map_["tsv"] = {FILE_TYPE_SPREADSHEET, "tsv", "text/tab-separated-values", "TSV File"};

    // Vector graphics
    extension_map_["svg"] = {FILE_TYPE_VECTOR, "svg", "image/svg+xml", "SVG Vector"};
    extension_map_["ai"] = {FILE_TYPE_VECTOR, "ai", "application/illustrator", "Adobe Illustrator"};
    extension_map_["eps"] = {FILE_TYPE_VECTOR, "eps", "application/postscript", "Encapsulated PostScript"};

    // Font formats
    extension_map_["ttf"] = {FILE_TYPE_FONT, "ttf", "font/ttf", "TrueType Font"};
    extension_map_["otf"] = {FILE_TYPE_FONT, "otf", "font/otf", "OpenType Font"};
    extension_map_["woff"] = {FILE_TYPE_FONT, "woff", "font/woff", "WOFF Font"};
    extension_map_["woff2"] = {FILE_TYPE_FONT, "woff2", "font/woff2", "WOFF2 Font"};

    // Data formats
    extension_map_["json"] = {FILE_TYPE_DATA, "json", "application/json", "JSON Data"};
    extension_map_["xml"] = {FILE_TYPE_DATA, "xml", "application/xml", "XML Data"};
    extension_map_["yaml"] = {FILE_TYPE_DATA, "yaml", "application/yaml", "YAML Data"};
    extension_map_["yml"] = {FILE_TYPE_DATA, "yml", "application/yaml", "YAML Data"};
    extension_map_["toml"] = {FILE_TYPE_DATA, "toml", "application/toml", "TOML Data"};
    extension_map_["ini"] = {FILE_TYPE_DATA, "ini", "text/plain", "INI Configuration"};

    // Subtitle formats
    extension_map_["srt"] = {FILE_TYPE_SUBTITLE, "srt", "application/x-subrip", "SubRip Subtitle"};
    extension_map_["vtt"] = {FILE_TYPE_SUBTITLE, "vtt", "text/vtt", "WebVTT Subtitle"};
    extension_map_["ass"] = {FILE_TYPE_SUBTITLE, "ass", "text/x-ssa", "ASS Subtitle"};
    extension_map_["sub"] = {FILE_TYPE_SUBTITLE, "sub", "text/plain", "Subtitle File"};

    // Presentation formats
    extension_map_["pptx"] = {FILE_TYPE_PRESENTATION, "pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation", "PowerPoint"};
    extension_map_["ppt"] = {FILE_TYPE_PRESENTATION, "ppt", "application/vnd.ms-powerpoint", "PowerPoint"};
    extension_map_["odp"] = {FILE_TYPE_PRESENTATION, "odp", "application/vnd.oasis.opendocument.presentation", "OpenDocument Presentation"};
    extension_map_["key"] = {FILE_TYPE_PRESENTATION, "key", "application/x-iwork-keynote-sffkey", "Keynote"};

    // Register magic signatures
    magic_signatures_ = {
        {{0x89, 0x50, 0x4E, 0x47}, 0, {FILE_TYPE_IMAGE, "png", "image/png", "PNG"}},
        {{0xFF, 0xD8, 0xFF}, 0, {FILE_TYPE_IMAGE, "jpg", "image/jpeg", "JPEG"}},
        {{0x42, 0x4D}, 0, {FILE_TYPE_IMAGE, "bmp", "image/bmp", "BMP"}},
        {{0x47, 0x49, 0x46, 0x38}, 0, {FILE_TYPE_IMAGE, "gif", "image/gif", "GIF"}},
        {{0x52, 0x49, 0x46, 0x46}, 0, {FILE_TYPE_AUDIO, "wav", "audio/wav", "WAV"}},  // RIFF
        {{0x49, 0x44, 0x33}, 0, {FILE_TYPE_AUDIO, "mp3", "audio/mpeg", "MP3"}},  // ID3
        {{0xFF, 0xFB}, 0, {FILE_TYPE_AUDIO, "mp3", "audio/mpeg", "MP3"}},  // MP3 frame sync
        {{0x4F, 0x67, 0x67, 0x53}, 0, {FILE_TYPE_AUDIO, "ogg", "audio/ogg", "OGG"}},
        {{0x66, 0x4C, 0x61, 0x43}, 0, {FILE_TYPE_AUDIO, "flac", "audio/flac", "FLAC"}},
        {{0x50, 0x4B, 0x03, 0x04}, 0, {FILE_TYPE_ARCHIVE, "zip", "application/zip", "ZIP"}},
        {{0x25, 0x50, 0x44, 0x46}, 0, {FILE_TYPE_DOCUMENT, "pdf", "application/pdf", "PDF"}},
        {{0x73, 0x6F, 0x6C, 0x69, 0x64, 0x20}, 0, {FILE_TYPE_MODEL3D, "stl", "model/stl", "STL ASCII"}},  // "solid "
    };
}

FileTypeInfo FileDetector::detect_from_file(const std::string& path) {
    // Try magic number detection first
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        std::vector<uint8_t> header(64);
        file.read(reinterpret_cast<char*>(header.data()), header.size());
        size_t bytes_read = file.gcount();
        header.resize(bytes_read);

        FileTypeInfo info = detect_from_magic(header);
        if (info.category != FILE_TYPE_UNKNOWN) {
            return info;
        }
    }

    // Fall back to extension
    std::string ext = utils::FileUtils::get_file_extension(path);
    return detect_from_extension(ext);
}

FileTypeInfo FileDetector::detect_from_extension(const std::string& extension) {
    std::string ext_lower = extension;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

    auto it = extension_map_.find(ext_lower);
    if (it != extension_map_.end()) {
        return it->second;
    }

    return {FILE_TYPE_UNKNOWN, "", "", "Unknown"};
}

FileTypeInfo FileDetector::detect_from_magic(const std::vector<uint8_t>& header) {
    for (const auto& magic : magic_signatures_) {
        if (header.size() >= magic.offset + magic.signature.size()) {
            bool match = true;
            for (size_t i = 0; i < magic.signature.size(); i++) {
                if (header[magic.offset + i] != magic.signature[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return magic.info;
            }
        }
    }

    return {FILE_TYPE_UNKNOWN, "", "", "Unknown"};
}

bool FileDetector::is_supported(const std::string& extension) {
    std::string ext_lower = extension;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), [](unsigned char c){ return std::tolower(c); });
    return extension_map_.find(ext_lower) != extension_map_.end();
}

std::vector<std::string> FileDetector::get_supported_extensions(file_type_category_t category) {
    std::vector<std::string> extensions;
    for (const auto& pair : extension_map_) {
        if (pair.second.category == category) {
            extensions.push_back(pair.first);
        }
    }
    return extensions;
}

} // namespace core
} // namespace fconvert
