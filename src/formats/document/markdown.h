/**
 * Markdown format
 */

#ifndef MARKDOWN_H
#define MARKDOWN_H

#include "../../../include/fconvert.h"
#include "txt.h"  // Reuse TextDocument
#include <vector>
#include <string>

namespace fconvert {
namespace formats {

/**
 * Markdown codec
 */
class MarkdownCodec {
public:
    /**
     * Decode markdown file
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        TextDocument& doc);

    /**
     * Encode to markdown
     */
    static fconvert_error_t encode(
        const TextDocument& doc,
        std::vector<uint8_t>& data);

    /**
     * Strip markdown syntax to get plain text
     */
    static std::string strip_markdown(const std::string& markdown);

    /**
     * Check if data is markdown
     */
    static bool is_markdown(const uint8_t* data, size_t size);
};

} // namespace formats
} // namespace fconvert

#endif // MARKDOWN_H
