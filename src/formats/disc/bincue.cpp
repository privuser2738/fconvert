/**
 * BIN/CUE Disc Image Format Implementation
 */

#include "bincue.h"
#include <sstream>
#include <algorithm>
#include <cstring>

namespace fconvert {
namespace formats {

std::string BinCueCodec::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string BinCueCodec::unquote(const std::string& str) {
    std::string s = trim(str);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

bool BinCueCodec::parse_msf(const std::string& msf, CueIndex& index) {
    int m, s, f;
    if (sscanf(msf.c_str(), "%d:%d:%d", &m, &s, &f) != 3) {
        return false;
    }
    index.minutes = m;
    index.seconds = s;
    index.frames = f;
    return true;
}

std::string BinCueCodec::format_msf(const CueIndex& index) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             index.minutes, index.seconds, index.frames);
    return buf;
}

TrackMode BinCueCodec::parse_mode(const std::string& mode_str) {
    std::string mode = trim(mode_str);
    std::transform(mode.begin(), mode.end(), mode.begin(), ::toupper);

    if (mode == "MODE1/2048") return TrackMode::MODE1_2048;
    if (mode == "MODE1/2352") return TrackMode::MODE1_2352;
    if (mode == "MODE2/2336") return TrackMode::MODE2_2336;
    if (mode == "MODE2/2352") return TrackMode::MODE2_2352;
    if (mode == "AUDIO") return TrackMode::AUDIO;

    return TrackMode::MODE1_2048;
}

std::string BinCueCodec::format_mode(TrackMode mode) {
    switch (mode) {
        case TrackMode::MODE1_2048: return "MODE1/2048";
        case TrackMode::MODE1_2352: return "MODE1/2352";
        case TrackMode::MODE2_2336: return "MODE2/2336";
        case TrackMode::MODE2_2352: return "MODE2/2352";
        case TrackMode::AUDIO: return "AUDIO";
    }
    return "MODE1/2048";
}

bool BinCueCodec::is_cue(const uint8_t* data, size_t size) {
    if (size < 10) return false;

    std::string content(reinterpret_cast<const char*>(data),
                       std::min(size, size_t(256)));

    // Look for CUE keywords
    std::transform(content.begin(), content.end(), content.begin(), ::toupper);
    return content.find("FILE") != std::string::npos &&
           (content.find("TRACK") != std::string::npos ||
            content.find("BINARY") != std::string::npos);
}

bool BinCueCodec::is_bin(const uint8_t* data, size_t size) {
    // BIN files are raw data, check for CD sync pattern
    if (size < 16) return false;

    // CD-ROM sync pattern: 00 FF FF FF FF FF FF FF FF FF FF 00
    static const uint8_t sync[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

    return std::memcmp(data, sync, 12) == 0;
}

fconvert_error_t BinCueCodec::parse_cue(
    const std::string& cue_content,
    CueSheet& cue) {

    cue.files.clear();

    std::istringstream iss(cue_content);
    std::string line;

    CueFile* current_file = nullptr;
    CueTrack* current_track = nullptr;

    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty()) continue;

        // Tokenize
        std::istringstream line_stream(line);
        std::string command;
        line_stream >> command;
        std::transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (command == "CATALOG") {
            std::getline(line_stream, cue.catalog);
            cue.catalog = trim(cue.catalog);
        } else if (command == "PERFORMER") {
            std::string rest;
            std::getline(line_stream, rest);
            if (current_track) {
                current_track->performer = unquote(rest);
            } else {
                cue.performer = unquote(rest);
            }
        } else if (command == "TITLE") {
            std::string rest;
            std::getline(line_stream, rest);
            if (current_track) {
                current_track->title = unquote(rest);
            } else {
                cue.title = unquote(rest);
            }
        } else if (command == "SONGWRITER") {
            std::string rest;
            std::getline(line_stream, rest);
            cue.songwriter = unquote(rest);
        } else if (command == "FILE") {
            std::string rest;
            std::getline(line_stream, rest);
            rest = trim(rest);

            CueFile file;

            // Parse filename (may be quoted)
            size_t name_end;
            if (rest[0] == '"') {
                name_end = rest.find('"', 1);
                file.filename = rest.substr(1, name_end - 1);
                file.type = trim(rest.substr(name_end + 1));
            } else {
                name_end = rest.find(' ');
                file.filename = rest.substr(0, name_end);
                file.type = trim(rest.substr(name_end + 1));
            }

            cue.files.push_back(file);
            current_file = &cue.files.back();
            current_track = nullptr;
        } else if (command == "TRACK") {
            if (!current_file) continue;

            CueTrack track;
            int track_num;
            std::string mode_str;
            line_stream >> track_num >> mode_str;

            track.number = track_num;
            track.mode = parse_mode(mode_str);
            track.type = (track.mode == TrackMode::AUDIO) ? TrackType::AUDIO : TrackType::DATA;

            current_file->tracks.push_back(track);
            current_track = &current_file->tracks.back();
        } else if (command == "INDEX") {
            if (!current_track) continue;

            int index_num;
            std::string msf;
            line_stream >> index_num >> msf;

            CueIndex index;
            index.number = index_num;
            if (parse_msf(msf, index)) {
                current_track->indices.push_back(index);
            }
        } else if (command == "PREGAP") {
            if (!current_track) continue;

            std::string msf;
            line_stream >> msf;
            CueIndex idx;
            if (parse_msf(msf, idx)) {
                current_track->pregap_frames = idx.to_frames();
            }
        } else if (command == "POSTGAP") {
            if (!current_track) continue;

            std::string msf;
            line_stream >> msf;
            CueIndex idx;
            if (parse_msf(msf, idx)) {
                current_track->postgap_frames = idx.to_frames();
            }
        }
    }

    return FCONVERT_OK;
}

std::string BinCueCodec::generate_cue(const CueSheet& cue) {
    std::ostringstream oss;

    if (!cue.catalog.empty()) {
        oss << "CATALOG " << cue.catalog << "\n";
    }
    if (!cue.performer.empty()) {
        oss << "PERFORMER \"" << cue.performer << "\"\n";
    }
    if (!cue.title.empty()) {
        oss << "TITLE \"" << cue.title << "\"\n";
    }
    if (!cue.songwriter.empty()) {
        oss << "SONGWRITER \"" << cue.songwriter << "\"\n";
    }

    for (const auto& file : cue.files) {
        oss << "FILE \"" << file.filename << "\" " << file.type << "\n";

        for (const auto& track : file.tracks) {
            oss << "  TRACK " << (track.number < 10 ? "0" : "") << track.number
                << " " << format_mode(track.mode) << "\n";

            if (!track.title.empty()) {
                oss << "    TITLE \"" << track.title << "\"\n";
            }
            if (!track.performer.empty()) {
                oss << "    PERFORMER \"" << track.performer << "\"\n";
            }

            if (track.pregap_frames > 0) {
                oss << "    PREGAP " << format_msf(CueIndex::from_frames(track.pregap_frames)) << "\n";
            }

            for (const auto& index : track.indices) {
                oss << "    INDEX " << (index.number < 10 ? "0" : "") << index.number
                    << " " << format_msf(index) << "\n";
            }

            if (track.postgap_frames > 0) {
                oss << "    POSTGAP " << format_msf(CueIndex::from_frames(track.postgap_frames)) << "\n";
            }
        }
    }

    return oss.str();
}

fconvert_error_t BinCueCodec::decode(
    const std::vector<uint8_t>& cue_data,
    const std::vector<uint8_t>& bin_data,
    BinCueImage& image) {

    std::string cue_content(reinterpret_cast<const char*>(cue_data.data()), cue_data.size());

    fconvert_error_t result = parse_cue(cue_content, image.cue);
    if (result != FCONVERT_OK) {
        return result;
    }

    image.bin_data = bin_data;

    return FCONVERT_OK;
}

uint64_t BinCueCodec::get_track_offset(const BinCueImage& image, int track_num) {
    uint64_t offset = 0;

    for (const auto& file : image.cue.files) {
        for (const auto& track : file.tracks) {
            if (track.number == track_num) {
                if (!track.indices.empty()) {
                    // Use INDEX 01 position
                    for (const auto& idx : track.indices) {
                        if (idx.number == 1) {
                            return idx.to_frames() * track.get_sector_size();
                        }
                    }
                    return track.indices[0].to_frames() * track.get_sector_size();
                }
                return offset;
            }
        }
    }

    return 0;
}

uint64_t BinCueCodec::get_track_size(const BinCueImage& image, int track_num) {
    const CueTrack* track = nullptr;
    const CueTrack* next_track = nullptr;

    for (const auto& file : image.cue.files) {
        for (size_t i = 0; i < file.tracks.size(); i++) {
            if (file.tracks[i].number == track_num) {
                track = &file.tracks[i];
                if (i + 1 < file.tracks.size()) {
                    next_track = &file.tracks[i + 1];
                }
                break;
            }
        }
    }

    if (!track) return 0;

    uint64_t start = get_track_offset(image, track_num);
    uint64_t end;

    if (next_track) {
        end = get_track_offset(image, next_track->number);
    } else {
        end = image.bin_data.size();
    }

    return end - start;
}

fconvert_error_t BinCueCodec::extract_data_track(
    const BinCueImage& image,
    int track_num,
    std::vector<uint8_t>& iso_data) {

    // Find the track
    const CueTrack* track = nullptr;
    for (const auto& file : image.cue.files) {
        for (const auto& t : file.tracks) {
            if (t.number == track_num) {
                track = &t;
                break;
            }
        }
    }

    if (!track || track->type != TrackType::DATA) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    uint64_t offset = get_track_offset(image, track_num);
    uint64_t size = get_track_size(image, track_num);

    uint32_t sector_size = track->get_sector_size();
    uint32_t num_sectors = static_cast<uint32_t>(size / sector_size);

    // Extract data portion of each sector
    iso_data.clear();
    iso_data.reserve(num_sectors * CD_SECTOR_DATA);

    for (uint32_t i = 0; i < num_sectors; i++) {
        const uint8_t* sector = image.bin_data.data() + offset + i * sector_size;

        if (sector_size == CD_SECTOR_DATA) {
            // Already in ISO format
            iso_data.insert(iso_data.end(), sector, sector + CD_SECTOR_DATA);
        } else if (sector_size == CD_SECTOR_RAW) {
            // Extract data from raw sector (skip 16-byte header)
            iso_data.insert(iso_data.end(), sector + 16, sector + 16 + CD_SECTOR_DATA);
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t BinCueCodec::iso_to_bincue(
    const std::vector<uint8_t>& iso_data,
    BinCueImage& image,
    const std::string& bin_filename) {

    // Create CUE sheet
    image.cue.files.clear();

    CueFile file;
    file.filename = bin_filename;
    file.type = "BINARY";

    CueTrack track;
    track.number = 1;
    track.mode = TrackMode::MODE1_2048;
    track.type = TrackType::DATA;

    CueIndex index;
    index.number = 1;
    index.minutes = 0;
    index.seconds = 0;
    index.frames = 0;
    track.indices.push_back(index);

    file.tracks.push_back(track);
    image.cue.files.push_back(file);

    // Copy ISO data as BIN (same format for MODE1/2048)
    image.bin_data = iso_data;

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
