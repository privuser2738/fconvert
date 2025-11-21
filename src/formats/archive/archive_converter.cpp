/**
 * Archive format converter implementation
 */

#include "archive_converter.h"
#include "../../core/logger.h"
#include "../../utils/gzip.h"
#include "../../utils/tar.h"
#include "../../utils/zip.h"
#include "../../utils/crc32.h"
#include <algorithm>

namespace fconvert {
namespace formats {

ArchiveConverter::ArchiveConverter() {
}

bool ArchiveConverter::is_supported_format(const std::string& format) const {
    static const std::vector<std::string> supported = {
        "gz", "gzip", "tar", "zip", "tgz", "tar.gz"
    };

    std::string fmt_lower = format;
    std::transform(fmt_lower.begin(), fmt_lower.end(), fmt_lower.begin(),
                 [](unsigned char c){ return std::tolower(c); });

    return std::find(supported.begin(), supported.end(), fmt_lower) != supported.end();
}

bool ArchiveConverter::can_convert(const std::string& from_format, const std::string& to_format) {
    return is_supported_format(from_format) && is_supported_format(to_format);
}

fconvert_error_t ArchiveConverter::convert(
    const std::vector<uint8_t>& input_data,
    const std::string& input_format,
    std::vector<uint8_t>& output_data,
    const std::string& output_format,
    const core::ConversionParams& params) {

    std::string in_fmt = input_format;
    std::string out_fmt = output_format;
    std::transform(in_fmt.begin(), in_fmt.end(), in_fmt.begin(),
                 [](unsigned char c){ return std::tolower(c); });
    std::transform(out_fmt.begin(), out_fmt.end(), out_fmt.begin(),
                 [](unsigned char c){ return std::tolower(c); });

    core::Logger::instance().debug("Converting archive: " + in_fmt + " -> " + out_fmt);

    // Normalize format names
    if (in_fmt == "gzip") in_fmt = "gz";
    if (out_fmt == "gzip") out_fmt = "gz";
    if (in_fmt == "tar.gz" || in_fmt == "tgz") in_fmt = "tgz";
    if (out_fmt == "tar.gz" || out_fmt == "tgz") out_fmt = "tgz";

    fconvert_error_t result = FCONVERT_OK;

    // Intermediate format: we work with either raw data, TAR entries, or ZIP entries
    std::vector<uint8_t> intermediate_data;
    std::vector<utils::TAREntry> tar_entries;
    std::vector<utils::ZIPEntry> zip_entries;

    bool have_raw_data = false;
    bool have_tar = false;
    bool have_zip = false;

    // ========== DECODE INPUT FORMAT ==========

    if (in_fmt == "gz") {
        // GZIP -> raw data
        core::Logger::instance().debug("Decompressing GZIP");
        std::string filename;
        result = utils::GZIP::decompress(input_data.data(), input_data.size(),
                                        intermediate_data, &filename);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to decompress GZIP");
            return result;
        }
        have_raw_data = true;
        core::Logger::instance().debug("Decompressed to " + std::to_string(intermediate_data.size()) + " bytes");
    }
    else if (in_fmt == "tar") {
        // TAR -> entries
        core::Logger::instance().debug("Extracting TAR archive");
        result = utils::TAR::extract(input_data.data(), input_data.size(), tar_entries);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to extract TAR");
            return result;
        }
        have_tar = true;
        core::Logger::instance().debug("Extracted " + std::to_string(tar_entries.size()) + " files");
    }
    else if (in_fmt == "zip") {
        // ZIP -> entries
        core::Logger::instance().debug("Extracting ZIP archive");
        result = utils::ZIP::extract(input_data.data(), input_data.size(), zip_entries);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to extract ZIP");
            return result;
        }
        have_zip = true;
        core::Logger::instance().debug("Extracted " + std::to_string(zip_entries.size()) + " files");
    }
    else if (in_fmt == "tgz") {
        // TAR.GZ -> GZIP decompress -> TAR extract
        core::Logger::instance().debug("Decompressing TAR.GZ");
        std::vector<uint8_t> tar_data;
        result = utils::GZIP::decompress(input_data.data(), input_data.size(), tar_data);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to decompress TAR.GZ");
            return result;
        }

        core::Logger::instance().debug("Extracting TAR from decompressed data");
        result = utils::TAR::extract(tar_data.data(), tar_data.size(), tar_entries);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to extract TAR from TAR.GZ");
            return result;
        }
        have_tar = true;
        core::Logger::instance().debug("Extracted " + std::to_string(tar_entries.size()) + " files");
    }
    else {
        core::Logger::instance().error("Unsupported input format: " + input_format);
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    // ========== CONVERT BETWEEN INTERMEDIATE FORMATS IF NEEDED ==========

    // Convert ZIP entries to TAR entries if needed
    if (have_zip && (out_fmt == "tar" || out_fmt == "tgz")) {
        for (const auto& zip_entry : zip_entries) {
            utils::TAREntry tar_entry;
            tar_entry.filename = zip_entry.filename;
            tar_entry.mode = 0644;
            tar_entry.uid = 1000;
            tar_entry.gid = 1000;
            tar_entry.size = zip_entry.data.size();
            tar_entry.mtime = 0;  // TODO: convert DOS time to Unix time
            tar_entry.typeflag = '0';
            tar_entry.data = zip_entry.data;
            tar_entries.push_back(tar_entry);
        }
        have_tar = true;
        have_zip = false;
    }

    // Convert TAR entries to ZIP entries if needed
    if (have_tar && out_fmt == "zip") {
        for (const auto& tar_entry : tar_entries) {
            if (tar_entry.typeflag == '0' || tar_entry.typeflag == '\0') {
                utils::ZIPEntry zip_entry;
                zip_entry.filename = tar_entry.filename;
                zip_entry.data = tar_entry.data;
                zip_entry.uncompressed_size = tar_entry.data.size();
                zip_entry.crc32 = utils::CRC32::calculate(tar_entry.data.data(), tar_entry.data.size());
                zip_entry.mtime = utils::ZIP::dos_time();
                zip_entry.compression_method = 8;  // DEFLATE
                zip_entries.push_back(zip_entry);
            }
        }
        have_zip = true;
        have_tar = false;
    }

    // ========== ENCODE OUTPUT FORMAT ==========

    if (out_fmt == "gz") {
        // Raw data -> GZIP
        if (!have_raw_data) {
            core::Logger::instance().error("Cannot convert archive with multiple files to GZIP");
            return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
        }

        core::Logger::instance().debug("Compressing to GZIP");
        int level = params.quality >= 90 ? 9 : (params.quality / 10);
        result = utils::GZIP::compress(intermediate_data.data(), intermediate_data.size(),
                                      output_data, level);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to compress to GZIP");
            return result;
        }
    }
    else if (out_fmt == "tar") {
        // Entries -> TAR
        if (!have_tar) {
            // Create single-file TAR from raw data
            if (have_raw_data) {
                utils::TAR::add_file(tar_entries, "data.bin",
                                   intermediate_data.data(), intermediate_data.size());
                have_tar = true;
            } else {
                core::Logger::instance().error("No TAR entries to write");
                return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
            }
        }

        core::Logger::instance().debug("Creating TAR archive with " +
                                      std::to_string(tar_entries.size()) + " files");
        result = utils::TAR::create(tar_entries, output_data);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to create TAR");
            return result;
        }
    }
    else if (out_fmt == "zip") {
        // Entries -> ZIP
        if (!have_zip) {
            // Create single-file ZIP from raw data
            if (have_raw_data) {
                utils::ZIP::add_file(zip_entries, "data.bin",
                                   intermediate_data.data(), intermediate_data.size());
                have_zip = true;
            } else {
                core::Logger::instance().error("No ZIP entries to write");
                return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
            }
        }

        core::Logger::instance().debug("Creating ZIP archive with " +
                                      std::to_string(zip_entries.size()) + " files");
        int level = params.quality >= 90 ? 9 : (params.quality / 10);
        result = utils::ZIP::create(zip_entries, output_data, level);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to create ZIP");
            return result;
        }
    }
    else if (out_fmt == "tgz") {
        // Entries -> TAR -> GZIP
        if (!have_tar) {
            // Create single-file TAR from raw data
            if (have_raw_data) {
                utils::TAR::add_file(tar_entries, "data.bin",
                                   intermediate_data.data(), intermediate_data.size());
                have_tar = true;
            } else {
                core::Logger::instance().error("No TAR entries to write");
                return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
            }
        }

        core::Logger::instance().debug("Creating TAR archive");
        std::vector<uint8_t> tar_data;
        result = utils::TAR::create(tar_entries, tar_data);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to create TAR");
            return result;
        }

        core::Logger::instance().debug("Compressing TAR to GZIP");
        int level = params.quality >= 90 ? 9 : (params.quality / 10);
        result = utils::GZIP::compress(tar_data.data(), tar_data.size(),
                                      output_data, level);
        if (result != FCONVERT_OK) {
            core::Logger::instance().error("Failed to compress to GZIP");
            return result;
        }
    }
    else {
        core::Logger::instance().error("Unsupported output format: " + output_format);
        return FCONVERT_ERROR_UNSUPPORTED_CONVERSION;
    }

    core::Logger::instance().debug("Archive converted successfully (" +
                                  std::to_string(output_data.size()) + " bytes)");

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
