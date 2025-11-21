#ifndef BZ2_H
#define BZ2_H
#include "../../../include/fconvert.h"
#include <vector>
namespace fconvert { namespace formats { class BZ2Codec { public: static fconvert_error_t decode(const std::vector<uint8_t>&, void*); static fconvert_error_t encode(const void*, std::vector<uint8_t>&); }; }}
#endif
