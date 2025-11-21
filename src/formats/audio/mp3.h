#ifndef MP3_H
#define MP3_H
#include "../../../include/fconvert.h"
#include <vector>
namespace fconvert { namespace formats {
class MP3Codec { public: static fconvert_error_t decode(const std::vector<uint8_t>&, void*); static fconvert_error_t encode(const void*, std::vector<uint8_t>&); };
}} // namespace
#endif
