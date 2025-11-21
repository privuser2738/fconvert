/**
 * TXT (Plain Text) format
 */

#ifndef TXT_H
#define TXT_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>

namespace fconvert {
namespace formats {

// Simple text document structure
struct TextDocument {
    std::string content;
    std::string encoding; // utf-8, ascii, etc.
};

/**
 * Plain text codec
 */
class TXTCodec {
public:
    /**
     * Decode plain text file
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        TextDocument& doc);

    /**
     * Encode to plain text
     */
    static fconvert_error_t encode(
        const TextDocument& doc,
        std::vector<uint8_t>& data);

    /**
     * Check if data is plain text
     */
    static bool is_text(const uint8_t* data, size_t size);
};

} // namespace formats
} // namespace fconvert

#endif // TXT_H
