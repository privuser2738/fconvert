/**
 * Markdown format implementation
 */

#include "markdown.h"
#include <sstream>
#include <regex>
#include <algorithm>

namespace fconvert {
namespace formats {

bool MarkdownCodec::is_markdown(const uint8_t* data, size_t size) {
    if (size < 2) return false;

    // Check for common markdown patterns
    std::string start(reinterpret_cast<const char*>(data), std::min(size, size_t(500)));

    // Look for markdown syntax
    return (start.find("# ") != std::string::npos ||      // Headers
            start.find("## ") != std::string::npos ||
            start.find("**") != std::string::npos ||       // Bold
            start.find("__") != std::string::npos ||
            start.find("* ") != std::string::npos ||       // Lists
            start.find("- ") != std::string::npos ||
            start.find("[") != std::string::npos ||        // Links
            start.find("](") != std::string::npos ||
            start.find("```") != std::string::npos);       // Code blocks
}

std::string MarkdownCodec::strip_markdown(const std::string& markdown) {
    std::string result = markdown;

    // Remove code blocks (```)
    size_t pos = 0;
    while ((pos = result.find("```", pos)) != std::string::npos) {
        size_t end = result.find("```", pos + 3);
        if (end != std::string::npos) {
            // Keep the content inside code blocks
            result.erase(end, 3);
            result.erase(pos, 3);
            pos = end;
        } else {
            result.erase(pos, 3);
            break;
        }
    }

    // Remove headers (# ## ###)
    std::istringstream iss(result);
    std::ostringstream oss;
    std::string line;

    while (std::getline(iss, line)) {
        // Remove leading # symbols
        size_t first_non_hash = line.find_first_not_of('#');
        if (first_non_hash != std::string::npos && first_non_hash > 0 &&
            line.size() > first_non_hash && line[first_non_hash] == ' ') {
            line = line.substr(first_non_hash + 1);
        }

        // Remove bold/italic markers
        // Simple removal: ** and __
        while ((pos = line.find("**")) != std::string::npos) {
            line.erase(pos, 2);
        }
        while ((pos = line.find("__")) != std::string::npos) {
            line.erase(pos, 2);
        }
        while ((pos = line.find("*")) != std::string::npos) {
            line.erase(pos, 1);
        }
        while ((pos = line.find("_")) != std::string::npos) {
            line.erase(pos, 1);
        }

        // Remove link syntax [text](url) -> text
        while ((pos = line.find("[")) != std::string::npos) {
            size_t close = line.find("]", pos);
            size_t paren = line.find("(", close);
            size_t close_paren = line.find(")", paren);

            if (close != std::string::npos && paren == close + 1 &&
                close_paren != std::string::npos) {
                std::string link_text = line.substr(pos + 1, close - pos - 1);
                line = line.substr(0, pos) + link_text + line.substr(close_paren + 1);
            } else {
                break;
            }
        }

        // Remove list markers (- or *)
        if (line.size() > 2 && (line[0] == '-' || line[0] == '*') && line[1] == ' ') {
            line = line.substr(2);
        }

        oss << line << "\n";
    }

    return oss.str();
}

fconvert_error_t MarkdownCodec::decode(
    const std::vector<uint8_t>& data,
    TextDocument& doc) {

    if (data.empty()) {
        doc.content = "";
        doc.encoding = "utf-8";
        return FCONVERT_OK;
    }

    // Markdown is just UTF-8 text with formatting
    doc.content = std::string(reinterpret_cast<const char*>(data.data()), data.size());
    doc.encoding = "utf-8";

    return FCONVERT_OK;
}

fconvert_error_t MarkdownCodec::encode(
    const TextDocument& doc,
    std::vector<uint8_t>& data) {

    data.clear();
    data.reserve(doc.content.size());

    // Convert string to bytes
    data.assign(doc.content.begin(), doc.content.end());

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
