#ifndef WEBM_H
#define WEBM_H
#include "../../../include/fconvert.h"
#include <vector>
namespace fconvert { namespace formats { class WEBMCodec { public: static fconvert_error_t decode(const std::vector<uint8_t>&, void*); static fconvert_error_t encode(const void*, std::vector<uint8_t>&); }; }}
#endif
