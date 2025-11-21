/**
 * WAV audio format codec
 * Supports PCM audio (uncompressed)
 */

#ifndef WAV_H
#define WAV_H

#include "../../../include/fconvert.h"
#include <vector>
#include <cstdint>

namespace fconvert {
namespace formats {

#pragma pack(push, 1)
struct WAVHeader {
    // RIFF header
    char riff_id[4];        // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave_id[4];        // "WAVE"

    // fmt subchunk
    char fmt_id[4];         // "fmt "
    uint32_t fmt_size;      // Size of fmt chunk (16 for PCM)
    uint16_t audio_format;  // 1 = PCM
    uint16_t num_channels;  // 1 = mono, 2 = stereo
    uint32_t sample_rate;   // e.g., 44100
    uint32_t byte_rate;     // sample_rate * num_channels * bits_per_sample / 8
    uint16_t block_align;   // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample; // 8, 16, 24, or 32

    // data subchunk
    char data_id[4];        // "data"
    uint32_t data_size;     // Size of audio data
};
#pragma pack(pop)

// Audio data structure
struct AudioData {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
    std::vector<uint8_t> samples;  // Raw PCM data

    AudioData() : sample_rate(44100), channels(2), bits_per_sample(16) {}
};

class WAVCodec {
public:
    /**
     * Decode WAV file to AudioData
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        AudioData& audio);

    /**
     * Encode AudioData to WAV file
     */
    static fconvert_error_t encode(
        const AudioData& audio,
        std::vector<uint8_t>& data);

    /**
     * Check if data is WAV format
     */
    static bool is_wav(const uint8_t* data, size_t size);

    /**
     * Get audio duration in seconds
     */
    static double get_duration(const AudioData& audio);

    /**
     * Resample audio to different sample rate
     */
    static fconvert_error_t resample(
        const AudioData& input,
        AudioData& output,
        uint32_t new_sample_rate);

    /**
     * Convert mono to stereo or vice versa
     */
    static fconvert_error_t convert_channels(
        const AudioData& input,
        AudioData& output,
        uint16_t new_channels);
};

} // namespace formats
} // namespace fconvert

#endif // WAV_H
