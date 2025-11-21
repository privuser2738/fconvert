/**
 * WAV audio format implementation
 */

#include "wav.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace fconvert {
namespace formats {

bool WAVCodec::is_wav(const uint8_t* data, size_t size) {
    if (size < 12) return false;

    // Check RIFF header
    if (std::memcmp(data, "RIFF", 4) != 0) return false;
    if (std::memcmp(data + 8, "WAVE", 4) != 0) return false;

    return true;
}

fconvert_error_t WAVCodec::decode(
    const std::vector<uint8_t>& data,
    AudioData& audio) {

    if (data.size() < sizeof(WAVHeader)) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    const uint8_t* ptr = data.data();
    size_t pos = 0;

    // Verify RIFF header
    if (std::memcmp(ptr, "RIFF", 4) != 0) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }
    pos += 4;

    // Skip file size
    pos += 4;

    // Verify WAVE
    if (std::memcmp(ptr + pos, "WAVE", 4) != 0) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }
    pos += 4;

    // Parse chunks
    bool found_fmt = false;
    bool found_data = false;

    while (pos + 8 <= data.size()) {
        char chunk_id[5] = {0};
        std::memcpy(chunk_id, ptr + pos, 4);
        pos += 4;

        uint32_t chunk_size;
        std::memcpy(&chunk_size, ptr + pos, 4);
        pos += 4;

        if (std::strcmp(chunk_id, "fmt ") == 0) {
            if (chunk_size < 16 || pos + chunk_size > data.size()) {
                return FCONVERT_ERROR_INVALID_FORMAT;
            }

            uint16_t audio_format;
            std::memcpy(&audio_format, ptr + pos, 2);

            // Only support PCM (1) and IEEE float (3)
            if (audio_format != 1 && audio_format != 3) {
                return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
            }

            std::memcpy(&audio.channels, ptr + pos + 2, 2);
            std::memcpy(&audio.sample_rate, ptr + pos + 4, 4);
            // Skip byte_rate (4 bytes) and block_align (2 bytes)
            std::memcpy(&audio.bits_per_sample, ptr + pos + 14, 2);

            found_fmt = true;
            pos += chunk_size;
        } else if (std::strcmp(chunk_id, "data") == 0) {
            if (!found_fmt) {
                return FCONVERT_ERROR_INVALID_FORMAT;
            }

            size_t data_end = pos + chunk_size;
            if (data_end > data.size()) {
                // Truncated file - use what we have
                chunk_size = static_cast<uint32_t>(data.size() - pos);
            }

            audio.samples.resize(chunk_size);
            std::memcpy(audio.samples.data(), ptr + pos, chunk_size);

            found_data = true;
            break;  // We have what we need
        } else {
            // Skip unknown chunk
            pos += chunk_size;
        }

        // Chunks are word-aligned
        if (chunk_size % 2 == 1 && pos < data.size()) {
            pos++;
        }
    }

    if (!found_fmt || !found_data) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    return FCONVERT_OK;
}

fconvert_error_t WAVCodec::encode(
    const AudioData& audio,
    std::vector<uint8_t>& data) {

    if (audio.samples.empty() || audio.channels == 0 ||
        audio.sample_rate == 0 || audio.bits_per_sample == 0) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    // Calculate sizes
    uint32_t data_size = static_cast<uint32_t>(audio.samples.size());
    uint32_t file_size = 36 + data_size;  // Header (44) - 8 + data_size

    // Prepare output
    data.resize(44 + data_size);
    uint8_t* ptr = data.data();

    // RIFF header
    std::memcpy(ptr, "RIFF", 4);
    std::memcpy(ptr + 4, &file_size, 4);
    std::memcpy(ptr + 8, "WAVE", 4);

    // fmt chunk
    std::memcpy(ptr + 12, "fmt ", 4);
    uint32_t fmt_size = 16;
    std::memcpy(ptr + 16, &fmt_size, 4);

    uint16_t audio_format = 1;  // PCM
    std::memcpy(ptr + 20, &audio_format, 2);
    std::memcpy(ptr + 22, &audio.channels, 2);
    std::memcpy(ptr + 24, &audio.sample_rate, 4);

    uint32_t byte_rate = audio.sample_rate * audio.channels * audio.bits_per_sample / 8;
    std::memcpy(ptr + 28, &byte_rate, 4);

    uint16_t block_align = audio.channels * audio.bits_per_sample / 8;
    std::memcpy(ptr + 32, &block_align, 2);
    std::memcpy(ptr + 34, &audio.bits_per_sample, 2);

    // data chunk
    std::memcpy(ptr + 36, "data", 4);
    std::memcpy(ptr + 40, &data_size, 4);
    std::memcpy(ptr + 44, audio.samples.data(), data_size);

    return FCONVERT_OK;
}

double WAVCodec::get_duration(const AudioData& audio) {
    if (audio.sample_rate == 0 || audio.channels == 0 || audio.bits_per_sample == 0) {
        return 0.0;
    }

    size_t bytes_per_sample = audio.channels * audio.bits_per_sample / 8;
    size_t num_samples = audio.samples.size() / bytes_per_sample;
    return static_cast<double>(num_samples) / audio.sample_rate;
}

fconvert_error_t WAVCodec::resample(
    const AudioData& input,
    AudioData& output,
    uint32_t new_sample_rate) {

    if (input.samples.empty() || new_sample_rate == 0) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    // Simple linear interpolation resampling
    double ratio = static_cast<double>(new_sample_rate) / input.sample_rate;
    size_t bytes_per_sample = input.channels * input.bits_per_sample / 8;
    size_t input_samples = input.samples.size() / bytes_per_sample;
    size_t output_samples = static_cast<size_t>(input_samples * ratio);

    output.sample_rate = new_sample_rate;
    output.channels = input.channels;
    output.bits_per_sample = input.bits_per_sample;
    output.samples.resize(output_samples * bytes_per_sample);

    // Only support 16-bit for now
    if (input.bits_per_sample != 16) {
        // Just copy for unsupported bit depths
        output.samples = input.samples;
        output.sample_rate = new_sample_rate;
        return FCONVERT_OK;
    }

    const int16_t* in_ptr = reinterpret_cast<const int16_t*>(input.samples.data());
    int16_t* out_ptr = reinterpret_cast<int16_t*>(output.samples.data());

    for (size_t i = 0; i < output_samples; i++) {
        double src_pos = i / ratio;
        size_t src_idx = static_cast<size_t>(src_pos);
        double frac = src_pos - src_idx;

        for (uint16_t ch = 0; ch < input.channels; ch++) {
            size_t idx1 = src_idx * input.channels + ch;
            size_t idx2 = std::min((src_idx + 1) * input.channels + ch,
                                  input_samples * input.channels - 1);

            int16_t s1 = in_ptr[idx1];
            int16_t s2 = in_ptr[idx2];

            // Linear interpolation
            int16_t result = static_cast<int16_t>(s1 + (s2 - s1) * frac);
            out_ptr[i * input.channels + ch] = result;
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t WAVCodec::convert_channels(
    const AudioData& input,
    AudioData& output,
    uint16_t new_channels) {

    if (input.samples.empty() || new_channels == 0) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    // Only support mono <-> stereo conversion
    if ((input.channels != 1 && input.channels != 2) ||
        (new_channels != 1 && new_channels != 2)) {
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    output.sample_rate = input.sample_rate;
    output.channels = new_channels;
    output.bits_per_sample = input.bits_per_sample;

    // Same number of channels - just copy
    if (input.channels == new_channels) {
        output.samples = input.samples;
        return FCONVERT_OK;
    }

    size_t bytes_per_sample_in = input.channels * input.bits_per_sample / 8;
    size_t bytes_per_sample_out = new_channels * input.bits_per_sample / 8;
    size_t num_samples = input.samples.size() / bytes_per_sample_in;

    output.samples.resize(num_samples * bytes_per_sample_out);

    // Only support 16-bit for now
    if (input.bits_per_sample != 16) {
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    const int16_t* in_ptr = reinterpret_cast<const int16_t*>(input.samples.data());
    int16_t* out_ptr = reinterpret_cast<int16_t*>(output.samples.data());

    if (input.channels == 1 && new_channels == 2) {
        // Mono to stereo
        for (size_t i = 0; i < num_samples; i++) {
            out_ptr[i * 2] = in_ptr[i];
            out_ptr[i * 2 + 1] = in_ptr[i];
        }
    } else if (input.channels == 2 && new_channels == 1) {
        // Stereo to mono (average)
        for (size_t i = 0; i < num_samples; i++) {
            int32_t sum = in_ptr[i * 2] + in_ptr[i * 2 + 1];
            out_ptr[i] = static_cast<int16_t>(sum / 2);
        }
    }

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
