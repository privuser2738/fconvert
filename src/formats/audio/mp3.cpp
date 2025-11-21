#include "mp3.h"
namespace fconvert { namespace formats {
fconvert_error_t MP3Codec::decode(const std::vector<uint8_t>&, void*) { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; }
fconvert_error_t MP3Codec::encode(const void*, std::vector<uint8_t>&) { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; }
}} // namespace
