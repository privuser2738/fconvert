#include "toml.h"
namespace fconvert { namespace formats { fconvert_error_t TOMLCodec::decode(const std::vector<uint8_t>&, void*) { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; } fconvert_error_t TOMLCodec::encode(const void*, std::vector<uint8_t>&) { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; } }}
