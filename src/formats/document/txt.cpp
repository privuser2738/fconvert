/**
 * TXT (Plain Text) format implementation
 */

#include "txt.h"
#include <algorithm>

namespace fconvert {
namespace formats {

bool TXTCodec::is_text(const uint8_t* data, size_t size) {
    if (size == 0) return false;

    // Check if mostly printable ASCII/UTF-8
    size_t printable = 0;
    size_t check_size = std::min(size, size_t(1024));

    for (size_t i = 0; i < check_size; i++) {
        uint8_t c = data[i];
        // Allow printable ASCII, newlines, tabs, and UTF-8
        if ((c >= 32 && c < 127) || c == '\n' || c == '\r' || c == '\t' || c >= 128) {
            printable++;
        }
    }

    // Consider it text if >90% is printable
    return (printable * 100 / check_size) > 90;
}

fconvert_error_t TXTCodec::decode(
    const std::vector<uint8_t>& data,
    TextDocument& doc) {

    if (data.empty()) {
        doc.content = "";
        doc.encoding = "utf-8";
        return FCONVERT_OK;
    }

    // Simple UTF-8 decoding (assumes valid UTF-8)
    doc.content = std::string(reinterpret_cast<const char*>(data.data()), data.size());
    doc.encoding = "utf-8";

    return FCONVERT_OK;
}

fconvert_error_t TXTCodec::encode(
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
