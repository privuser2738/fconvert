#include "ini.h"
namespace fconvert { namespace formats { fconvert_error_t INICodec::decode(const std::vector<uint8_t>&, void*) { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; } fconvert_error_t INICodec::encode(const void*, std::vector<uint8_t>&) { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; } }}
