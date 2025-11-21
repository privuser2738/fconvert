#include "wav.h"

namespace fconvert {
namespace formats {

fconvert_error_t WAVCodec::decode(const std::vector<uint8_t>& data, void* audio_data) {
    (void)data; (void)audio_data;
    return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
}

fconvert_error_t WAVCodec::encode(const void* audio_data, std::vector<uint8_t>& data) {
    (void)audio_data; (void)data;
    return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
}

} // namespace formats
} // namespace fconvert
