/**
 * Disc Image Converter Implementation
 */

#include "disc_converter.h"

namespace fconvert {

using namespace formats;

std::vector<disc_format_t> DiscConverter::get_input_formats() {
    return {
        DISC_FORMAT_ISO,
        DISC_FORMAT_BIN,
        DISC_FORMAT_VHD,
        DISC_FORMAT_CHD
    };
}

std::vector<disc_format_t> DiscConverter::get_output_formats() {
    return {
        DISC_FORMAT_ISO,
        DISC_FORMAT_BIN,
        DISC_FORMAT_VHD,
        DISC_FORMAT_CHD
    };
}

bool DiscConverter::can_convert(disc_format_t from, disc_format_t to) {
    auto inputs = get_input_formats();
    auto outputs = get_output_formats();

    bool has_input = std::find(inputs.begin(), inputs.end(), from) != inputs.end();
    bool has_output = std::find(outputs.begin(), outputs.end(), to) != outputs.end();

    return has_input && has_output && from != to;
}

disc_format_t DiscConverter::detect_format(const uint8_t* data, size_t size) {
    if (ISOCodec::is_iso(data, size)) {
        return DISC_FORMAT_ISO;
    }
    if (BinCueCodec::is_cue(data, size)) {
        return DISC_FORMAT_CUE;
    }
    if (BinCueCodec::is_bin(data, size)) {
        return DISC_FORMAT_BIN;
    }
    if (VHDCodec::is_vhd(data, size)) {
        return DISC_FORMAT_VHD;
    }
    if (CHDCodec::is_chd(data, size)) {
        return DISC_FORMAT_CHD;
    }
    return DISC_FORMAT_UNKNOWN;
}

const char* DiscConverter::format_name(disc_format_t format) {
    switch (format) {
        case DISC_FORMAT_ISO: return "ISO 9660";
        case DISC_FORMAT_BIN: return "BIN/CUE";
        case DISC_FORMAT_CUE: return "CUE Sheet";
        case DISC_FORMAT_VHD: return "VHD";
        case DISC_FORMAT_CHD: return "CHD";
        case DISC_FORMAT_VMDK: return "VMDK";
        case DISC_FORMAT_VDI: return "VDI";
        case DISC_FORMAT_QCOW2: return "QCOW2";
        case DISC_FORMAT_NRG: return "Nero Image";
        case DISC_FORMAT_MDF: return "MDF";
        case DISC_FORMAT_MDS: return "MDS";
        default: return "Unknown";
    }
}

const char* DiscConverter::format_extension(disc_format_t format) {
    switch (format) {
        case DISC_FORMAT_ISO: return ".iso";
        case DISC_FORMAT_BIN: return ".bin";
        case DISC_FORMAT_CUE: return ".cue";
        case DISC_FORMAT_VHD: return ".vhd";
        case DISC_FORMAT_CHD: return ".chd";
        case DISC_FORMAT_VMDK: return ".vmdk";
        case DISC_FORMAT_VDI: return ".vdi";
        case DISC_FORMAT_QCOW2: return ".qcow2";
        case DISC_FORMAT_NRG: return ".nrg";
        case DISC_FORMAT_MDF: return ".mdf";
        case DISC_FORMAT_MDS: return ".mds";
        default: return "";
    }
}

fconvert_error_t DiscConverter::convert(
    const std::vector<uint8_t>& input,
    disc_format_t input_type,
    std::vector<uint8_t>& output,
    disc_format_t output_type,
    const DiscConvertOptions* options) {

    if (!can_convert(input_type, output_type)) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Route to appropriate conversion function
    if (input_type == DISC_FORMAT_ISO) {
        if (output_type == DISC_FORMAT_BIN) return iso_to_bincue(input, output);
        if (output_type == DISC_FORMAT_VHD) return iso_to_vhd(input, output, options);
        if (output_type == DISC_FORMAT_CHD) return iso_to_chd(input, output);
    }
    else if (input_type == DISC_FORMAT_BIN || input_type == DISC_FORMAT_CUE) {
        if (output_type == DISC_FORMAT_ISO) return bincue_to_iso(input, output);
        if (output_type == DISC_FORMAT_CHD) return bincue_to_chd(input, output);
        if (output_type == DISC_FORMAT_VHD) {
            // BIN -> ISO -> VHD
            std::vector<uint8_t> iso_data;
            fconvert_error_t result = bincue_to_iso(input, iso_data);
            if (result != FCONVERT_OK) return result;
            return iso_to_vhd(iso_data, output, options);
        }
    }
    else if (input_type == DISC_FORMAT_VHD) {
        if (output_type == DISC_FORMAT_ISO) return vhd_to_iso(input, output);
        if (output_type == DISC_FORMAT_CHD) return vhd_to_chd(input, output);
        if (output_type == DISC_FORMAT_BIN) {
            // VHD -> ISO -> BIN
            std::vector<uint8_t> iso_data;
            fconvert_error_t result = vhd_to_iso(input, iso_data);
            if (result != FCONVERT_OK) return result;
            return iso_to_bincue(iso_data, output);
        }
    }
    else if (input_type == DISC_FORMAT_CHD) {
        if (output_type == DISC_FORMAT_ISO) return chd_to_iso(input, output);
        if (output_type == DISC_FORMAT_VHD) return chd_to_vhd(input, output, options);
        if (output_type == DISC_FORMAT_BIN) return chd_to_bincue(input, output);
    }

    return FCONVERT_ERROR_INVALID_FORMAT;
}

fconvert_error_t DiscConverter::iso_to_bincue(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output) {

    BinCueImage bincue;
    fconvert_error_t result = BinCueCodec::iso_to_bincue(input, bincue);
    if (result != FCONVERT_OK) return result;

    // Output is the BIN data; CUE would need separate handling
    output = bincue.bin_data;
    return FCONVERT_OK;
}

fconvert_error_t DiscConverter::bincue_to_iso(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output) {

    // If it's a CUE file, we need the BIN file too
    // For now, treat input as BIN data in MODE1/2048
    if (BinCueCodec::is_cue(input.data(), input.size())) {
        // Can't extract without BIN file
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    // Assume MODE1/2048 BIN
    output = input;
    return FCONVERT_OK;
}

fconvert_error_t DiscConverter::iso_to_vhd(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output,
    const DiscConvertOptions* options) {

    VhdImage vhd;
    VhdDiskType disk_type = VhdDiskType::Dynamic;

    if (options && !options->dynamic_vhd) {
        disk_type = VhdDiskType::Fixed;
    }

    fconvert_error_t result = VHDCodec::create_from_raw(input, vhd, disk_type);
    if (result != FCONVERT_OK) return result;

    if (disk_type == VhdDiskType::Dynamic) {
        return VHDCodec::encode_dynamic(vhd, output);
    } else {
        return VHDCodec::encode_fixed(vhd, output);
    }
}

fconvert_error_t DiscConverter::vhd_to_iso(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output) {

    VhdImage vhd;
    fconvert_error_t result = VHDCodec::decode(input, vhd);
    if (result != FCONVERT_OK) return result;

    return VHDCodec::extract_raw(vhd, output);
}

fconvert_error_t DiscConverter::iso_to_chd(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output) {

    ChdImage chd;
    fconvert_error_t result = CHDCodec::create_from_raw(input, chd);
    if (result != FCONVERT_OK) return result;

    return CHDCodec::encode(chd, output);
}

fconvert_error_t DiscConverter::chd_to_iso(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output) {

    ChdImage chd;
    fconvert_error_t result = CHDCodec::decode(input, chd);
    if (result != FCONVERT_OK) return result;

    return CHDCodec::extract_raw(chd, output);
}

fconvert_error_t DiscConverter::vhd_to_chd(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output) {

    // VHD -> raw -> CHD
    std::vector<uint8_t> raw_data;
    fconvert_error_t result = vhd_to_iso(input, raw_data);
    if (result != FCONVERT_OK) return result;

    return iso_to_chd(raw_data, output);
}

fconvert_error_t DiscConverter::chd_to_vhd(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output,
    const DiscConvertOptions* options) {

    // CHD -> raw -> VHD
    std::vector<uint8_t> raw_data;
    fconvert_error_t result = chd_to_iso(input, raw_data);
    if (result != FCONVERT_OK) return result;

    return iso_to_vhd(raw_data, output, options);
}

fconvert_error_t DiscConverter::bincue_to_chd(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output) {

    // BIN -> ISO -> CHD
    std::vector<uint8_t> iso_data;
    fconvert_error_t result = bincue_to_iso(input, iso_data);
    if (result != FCONVERT_OK) return result;

    return iso_to_chd(iso_data, output);
}

fconvert_error_t DiscConverter::chd_to_bincue(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output) {

    // CHD -> ISO -> BIN
    std::vector<uint8_t> iso_data;
    fconvert_error_t result = chd_to_iso(input, iso_data);
    if (result != FCONVERT_OK) return result;

    return iso_to_bincue(iso_data, output);
}

} // namespace fconvert
