/**
 * BIN/CUE Disc Image Format
 * Raw CD image with cue sheet
 */

#ifndef BINCUE_H
#define BINCUE_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>

namespace fconvert {
namespace formats {

// CD sector sizes
constexpr uint32_t CD_SECTOR_RAW = 2352;      // Raw sector with headers
constexpr uint32_t CD_SECTOR_DATA = 2048;     // Data only (ISO mode)
constexpr uint32_t CD_SECTOR_AUDIO = 2352;    // Audio sector

// Track modes
enum class TrackMode {
    MODE1_2048,     // CD-ROM Mode 1, 2048 bytes/sector
    MODE1_2352,     // CD-ROM Mode 1 raw, 2352 bytes/sector
    MODE2_2336,     // CD-ROM Mode 2, 2336 bytes/sector
    MODE2_2352,     // CD-ROM Mode 2 raw, 2352 bytes/sector
    AUDIO           // Audio track, 2352 bytes/sector
};

// Track type
enum class TrackType {
    DATA,
    AUDIO
};

// Index within a track
struct CueIndex {
    int number;
    int minutes;
    int seconds;
    int frames;

    uint32_t to_frames() const {
        return (minutes * 60 + seconds) * 75 + frames;
    }

    static CueIndex from_frames(uint32_t total_frames) {
        CueIndex idx;
        idx.frames = total_frames % 75;
        total_frames /= 75;
        idx.seconds = total_frames % 60;
        idx.minutes = total_frames / 60;
        idx.number = 1;
        return idx;
    }
};

// Track information
struct CueTrack {
    int number;
    TrackMode mode;
    TrackType type;
    std::vector<CueIndex> indices;
    std::string performer;
    std::string title;
    int pregap_frames;      // PREGAP in frames
    int postgap_frames;     // POSTGAP in frames

    CueTrack() : number(0), mode(TrackMode::MODE1_2048), type(TrackType::DATA),
                 pregap_frames(0), postgap_frames(0) {}

    uint32_t get_sector_size() const {
        switch (mode) {
            case TrackMode::MODE1_2048: return 2048;
            case TrackMode::MODE1_2352: return 2352;
            case TrackMode::MODE2_2336: return 2336;
            case TrackMode::MODE2_2352: return 2352;
            case TrackMode::AUDIO: return 2352;
        }
        return 2048;
    }
};

// File reference in cue sheet
struct CueFile {
    std::string filename;
    std::string type;       // BINARY, WAVE, MP3, etc.
    std::vector<CueTrack> tracks;
};

// Complete cue sheet
struct CueSheet {
    std::string catalog;
    std::string performer;
    std::string title;
    std::string songwriter;
    std::vector<CueFile> files;
};

// BIN/CUE image
struct BinCueImage {
    CueSheet cue;
    std::vector<uint8_t> bin_data;
};

/**
 * BIN/CUE codec
 */
class BinCueCodec {
public:
    /**
     * Parse CUE sheet
     */
    static fconvert_error_t parse_cue(
        const std::string& cue_content,
        CueSheet& cue);

    /**
     * Generate CUE sheet
     */
    static std::string generate_cue(
        const CueSheet& cue);

    /**
     * Decode BIN/CUE image
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& cue_data,
        const std::vector<uint8_t>& bin_data,
        BinCueImage& image);

    /**
     * Check if data is CUE format
     */
    static bool is_cue(const uint8_t* data, size_t size);

    /**
     * Check if data is raw BIN format
     */
    static bool is_bin(const uint8_t* data, size_t size);

    /**
     * Extract data track to ISO
     */
    static fconvert_error_t extract_data_track(
        const BinCueImage& image,
        int track_num,
        std::vector<uint8_t>& iso_data);

    /**
     * Convert ISO to BIN/CUE
     */
    static fconvert_error_t iso_to_bincue(
        const std::vector<uint8_t>& iso_data,
        BinCueImage& image,
        const std::string& bin_filename = "image.bin");

    /**
     * Get track start position in BIN file
     */
    static uint64_t get_track_offset(
        const BinCueImage& image,
        int track_num);

    /**
     * Get track size in bytes
     */
    static uint64_t get_track_size(
        const BinCueImage& image,
        int track_num);

private:
    // Parse MSF time (MM:SS:FF)
    static bool parse_msf(const std::string& msf, CueIndex& index);

    // Format MSF time
    static std::string format_msf(const CueIndex& index);

    // Parse track mode string
    static TrackMode parse_mode(const std::string& mode_str);

    // Format track mode
    static std::string format_mode(TrackMode mode);

    // Trim whitespace
    static std::string trim(const std::string& str);

    // Remove quotes
    static std::string unquote(const std::string& str);
};

} // namespace formats
} // namespace fconvert

#endif // BINCUE_H
