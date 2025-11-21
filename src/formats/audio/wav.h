/**
 * WAV audio format codec (stub)
 */

#ifndef WAV_H
#define WAV_H

#include "../../../include/fconvert.h"
#include <vector>

namespace fconvert {
namespace formats {

// TODO: Implement WAV codec (relatively simple - RIFF chunks)
class WAVCodec {
public:
    static fconvert_error_t decode(const std::vector<uint8_t>& data, void* audio_data);
    static fconvert_error_t encode(const void* audio_data, std::vector<uint8_t>& data);
};

} // namespace formats
} // namespace fconvert

#endif // WAV_H
