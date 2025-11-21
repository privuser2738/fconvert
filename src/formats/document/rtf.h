#ifndef RTF_H
#define RTF_H
#include "../../../include/fconvert.h"
#include <vector>
namespace fconvert { namespace formats { class RTFCodec { public: static fconvert_error_t decode(const std::vector<uint8_t>&, void*); static fconvert_error_t encode(const void*, std::vector<uint8_t>&); }; }}
#endif
