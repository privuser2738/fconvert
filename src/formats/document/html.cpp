#include "html.h"
namespace fconvert { namespace formats { fconvert_error_t HTMLCodec::decode(const std::vector<uint8_t>&, void*) { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; } fconvert_error_t HTMLCodec::encode(const void*, std::vector<uint8_t>&) { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; } }}
