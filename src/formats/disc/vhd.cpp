/**
 * VHD (Virtual Hard Disk) Format Implementation
 */

#include "vhd.h"
#include <cstring>
#include <ctime>
#include <random>

namespace fconvert {
namespace formats {

constexpr uint64_t VHD_DYNAMIC_COOKIE = 0x6378737061727365ULL;  // "cxsparse"

uint16_t VHDCodec::swap16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

uint32_t VHDCodec::swap32(uint32_t val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

uint64_t VHDCodec::swap64(uint64_t val) {
    return ((val >> 56) & 0xFF) |
           ((val >> 40) & 0xFF00) |
           ((val >> 24) & 0xFF0000) |
           ((val >> 8) & 0xFF000000) |
           ((val << 8) & 0xFF00000000ULL) |
           ((val << 24) & 0xFF0000000000ULL) |
           ((val << 40) & 0xFF000000000000ULL) |
           ((val << 56) & 0xFF00000000000000ULL);
}

void VHDCodec::generate_uuid(uint8_t* uuid) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 16; i++) {
        uuid[i] = static_cast<uint8_t>(dis(gen));
    }
    // Set version 4 (random)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;
    // Set variant
    uuid[8] = (uuid[8] & 0x3F) | 0x80;
}

uint32_t VHDCodec::calculate_checksum(const VhdFooter& footer) {
    uint32_t checksum = 0;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&footer);

    for (size_t i = 0; i < sizeof(VhdFooter); i++) {
        // Skip the checksum field itself (bytes 64-67)
        if (i >= 64 && i < 68) continue;
        checksum += ptr[i];
    }

    return ~checksum;
}

uint32_t VHDCodec::calculate_checksum(const VhdDynamicHeader& header) {
    uint32_t checksum = 0;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&header);

    for (size_t i = 0; i < sizeof(VhdDynamicHeader); i++) {
        // Skip the checksum field itself (bytes 36-39)
        if (i >= 36 && i < 40) continue;
        checksum += ptr[i];
    }

    return ~checksum;
}

VhdGeometry VHDCodec::calculate_geometry(uint64_t disk_size) {
    VhdGeometry geom;
    uint64_t total_sectors = disk_size / VHD_SECTOR_SIZE;

    // VHD CHS calculation algorithm
    if (total_sectors > 65535 * 16 * 255) {
        total_sectors = 65535 * 16 * 255;
    }

    if (total_sectors >= 65535 * 16 * 63) {
        geom.sectors_per_track = 255;
        geom.heads = 16;
        geom.cylinders = static_cast<uint16_t>(total_sectors / (16 * 255));
    } else {
        geom.sectors_per_track = 17;
        uint32_t cyl_times_heads = static_cast<uint32_t>(total_sectors / 17);
        geom.heads = static_cast<uint8_t>((cyl_times_heads + 1023) / 1024);

        if (geom.heads < 4) geom.heads = 4;
        if (cyl_times_heads >= geom.heads * 1024 || geom.heads > 16) {
            geom.sectors_per_track = 31;
            geom.heads = 16;
            cyl_times_heads = static_cast<uint32_t>(total_sectors / 31);
        }
        if (cyl_times_heads >= geom.heads * 1024) {
            geom.sectors_per_track = 63;
            geom.heads = 16;
            cyl_times_heads = static_cast<uint32_t>(total_sectors / 63);
        }

        geom.cylinders = static_cast<uint16_t>(cyl_times_heads / geom.heads);
    }

    return geom;
}

bool VHDCodec::is_vhd(const uint8_t* data, size_t size) {
    if (size < sizeof(VhdFooter)) {
        return false;
    }

    // Check footer at end of file
    const VhdFooter* footer = reinterpret_cast<const VhdFooter*>(
        data + size - sizeof(VhdFooter));

    // VHD cookie is big-endian "conectix"
    return swap64(footer->cookie) == VHD_COOKIE;
}

fconvert_error_t VHDCodec::decode(
    const std::vector<uint8_t>& data,
    VhdImage& image) {

    if (!is_vhd(data.data(), data.size())) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Read footer from end of file
    const VhdFooter* footer = reinterpret_cast<const VhdFooter*>(
        data.data() + data.size() - sizeof(VhdFooter));

    image.type = static_cast<VhdDiskType>(swap32(footer->disk_type));
    image.disk_size = swap64(footer->current_size);

    // Parse geometry
    uint32_t geom = swap32(footer->disk_geometry);
    image.geometry.cylinders = static_cast<uint16_t>((geom >> 16) & 0xFFFF);
    image.geometry.heads = static_cast<uint8_t>((geom >> 8) & 0xFF);
    image.geometry.sectors_per_track = static_cast<uint8_t>(geom & 0xFF);

    std::memcpy(image.unique_id.data(), footer->unique_id, 16);

    if (image.type == VhdDiskType::Fixed) {
        // Fixed disk: data is raw before the footer
        size_t data_size = data.size() - sizeof(VhdFooter);
        image.data.resize(data_size);
        std::memcpy(image.data.data(), data.data(), data_size);
        image.block_size = 0;
    } else if (image.type == VhdDiskType::Dynamic) {
        // Read dynamic header
        uint64_t header_offset = swap64(footer->data_offset);
        if (header_offset + sizeof(VhdDynamicHeader) > data.size()) {
            return FCONVERT_ERROR_INVALID_FORMAT;
        }

        const VhdDynamicHeader* dyn_header = reinterpret_cast<const VhdDynamicHeader*>(
            data.data() + header_offset);

        image.block_size = swap32(dyn_header->block_size);
        uint32_t bat_entries = swap32(dyn_header->max_table_entries);
        uint64_t bat_offset = swap64(dyn_header->table_offset);

        // Read BAT
        if (bat_offset + bat_entries * 4 > data.size()) {
            return FCONVERT_ERROR_INVALID_FORMAT;
        }

        image.bat.resize(bat_entries);
        const uint32_t* bat_ptr = reinterpret_cast<const uint32_t*>(data.data() + bat_offset);
        for (uint32_t i = 0; i < bat_entries; i++) {
            image.bat[i] = swap32(bat_ptr[i]);
        }

        // Read blocks
        uint32_t bitmap_size = (image.block_size / VHD_SECTOR_SIZE + 7) / 8;
        bitmap_size = ((bitmap_size + VHD_SECTOR_SIZE - 1) / VHD_SECTOR_SIZE) * VHD_SECTOR_SIZE;

        image.blocks.resize(bat_entries);
        for (uint32_t i = 0; i < bat_entries; i++) {
            if (image.bat[i] != 0xFFFFFFFF) {
                uint64_t block_offset = static_cast<uint64_t>(image.bat[i]) * VHD_SECTOR_SIZE;
                if (block_offset + bitmap_size + image.block_size <= data.size()) {
                    image.blocks[i].resize(image.block_size);
                    std::memcpy(image.blocks[i].data(),
                               data.data() + block_offset + bitmap_size,
                               image.block_size);
                }
            }
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t VHDCodec::encode_fixed(
    const VhdImage& image,
    std::vector<uint8_t>& data) {

    // Fixed VHD: raw data + footer
    data.resize(image.disk_size + sizeof(VhdFooter));
    std::memcpy(data.data(), image.data.data(),
               std::min(image.data.size(), static_cast<size_t>(image.disk_size)));

    // Create footer
    VhdFooter* footer = reinterpret_cast<VhdFooter*>(
        data.data() + image.disk_size);
    std::memset(footer, 0, sizeof(VhdFooter));

    footer->cookie = swap64(VHD_COOKIE);
    footer->features = swap32(0x00000002);
    footer->file_format_version = swap32(VHD_VERSION);
    footer->data_offset = swap64(0xFFFFFFFFFFFFFFFFULL);  // No dynamic header

    // Timestamp: seconds since Jan 1, 2000 12:00:00 UTC
    time_t now = time(nullptr);
    uint32_t vhd_time = static_cast<uint32_t>(now - 946684800);  // 2000-01-01 epoch
    footer->timestamp = swap32(vhd_time);

    footer->creator_app = swap32(0x6663766E);  // "fcvn"
    footer->creator_version = swap32(0x00010000);
    footer->creator_host_os = swap32(0x5769326B);  // "Wi2k"

    footer->original_size = swap64(image.disk_size);
    footer->current_size = swap64(image.disk_size);

    // Geometry
    uint32_t geom = (static_cast<uint32_t>(image.geometry.cylinders) << 16) |
                    (static_cast<uint32_t>(image.geometry.heads) << 8) |
                    image.geometry.sectors_per_track;
    footer->disk_geometry = swap32(geom);

    footer->disk_type = swap32(static_cast<uint32_t>(VhdDiskType::Fixed));
    std::memcpy(footer->unique_id, image.unique_id.data(), 16);

    footer->checksum = swap32(calculate_checksum(*footer));

    return FCONVERT_OK;
}

fconvert_error_t VHDCodec::encode_dynamic(
    const VhdImage& image,
    std::vector<uint8_t>& data) {

    uint32_t block_size = image.block_size > 0 ? image.block_size : VHD_DEFAULT_BLOCK_SIZE;
    uint32_t num_blocks = static_cast<uint32_t>((image.disk_size + block_size - 1) / block_size);
    uint32_t bitmap_size = (block_size / VHD_SECTOR_SIZE + 7) / 8;
    bitmap_size = ((bitmap_size + VHD_SECTOR_SIZE - 1) / VHD_SECTOR_SIZE) * VHD_SECTOR_SIZE;

    // Calculate offsets
    uint64_t footer_copy_offset = 0;  // Copy of footer at start
    uint64_t dyn_header_offset = sizeof(VhdFooter);
    uint64_t bat_offset = dyn_header_offset + 1024;
    uint64_t bat_size = ((num_blocks * 4 + VHD_SECTOR_SIZE - 1) / VHD_SECTOR_SIZE) * VHD_SECTOR_SIZE;
    uint64_t blocks_offset = bat_offset + bat_size;

    // Count non-empty blocks
    std::vector<bool> block_used(num_blocks, false);
    uint32_t used_blocks = 0;

    for (uint32_t i = 0; i < num_blocks; i++) {
        uint64_t block_start = static_cast<uint64_t>(i) * block_size;
        uint64_t block_end = std::min(block_start + block_size, image.disk_size);

        bool has_data = false;
        for (uint64_t j = block_start; j < block_end && j < image.data.size(); j++) {
            if (image.data[j] != 0) {
                has_data = true;
                break;
            }
        }

        if (has_data) {
            block_used[i] = true;
            used_blocks++;
        }
    }

    // Calculate total size
    uint64_t total_size = blocks_offset + used_blocks * (bitmap_size + block_size) + sizeof(VhdFooter);
    data.resize(total_size, 0);

    // Build BAT
    std::vector<uint32_t> bat(num_blocks, 0xFFFFFFFF);
    uint64_t current_block_offset = blocks_offset;

    for (uint32_t i = 0; i < num_blocks; i++) {
        if (block_used[i]) {
            bat[i] = static_cast<uint32_t>(current_block_offset / VHD_SECTOR_SIZE);

            // Write bitmap (all 1s = all sectors present)
            std::memset(data.data() + current_block_offset, 0xFF, bitmap_size);
            current_block_offset += bitmap_size;

            // Write block data
            uint64_t block_start = static_cast<uint64_t>(i) * block_size;
            uint64_t copy_size = std::min(static_cast<uint64_t>(block_size),
                                          image.data.size() > block_start ?
                                          image.data.size() - block_start : 0);
            if (copy_size > 0) {
                std::memcpy(data.data() + current_block_offset,
                           image.data.data() + block_start, copy_size);
            }
            current_block_offset += block_size;
        }
    }

    // Write footer copy at start
    VhdFooter* footer_copy = reinterpret_cast<VhdFooter*>(data.data());
    std::memset(footer_copy, 0, sizeof(VhdFooter));

    footer_copy->cookie = swap64(VHD_COOKIE);
    footer_copy->features = swap32(0x00000002);
    footer_copy->file_format_version = swap32(VHD_VERSION);
    footer_copy->data_offset = swap64(dyn_header_offset);

    time_t now = time(nullptr);
    footer_copy->timestamp = swap32(static_cast<uint32_t>(now - 946684800));
    footer_copy->creator_app = swap32(0x6663766E);
    footer_copy->creator_version = swap32(0x00010000);
    footer_copy->creator_host_os = swap32(0x5769326B);
    footer_copy->original_size = swap64(image.disk_size);
    footer_copy->current_size = swap64(image.disk_size);

    uint32_t geom = (static_cast<uint32_t>(image.geometry.cylinders) << 16) |
                    (static_cast<uint32_t>(image.geometry.heads) << 8) |
                    image.geometry.sectors_per_track;
    footer_copy->disk_geometry = swap32(geom);
    footer_copy->disk_type = swap32(static_cast<uint32_t>(VhdDiskType::Dynamic));
    std::memcpy(footer_copy->unique_id, image.unique_id.data(), 16);
    footer_copy->checksum = swap32(calculate_checksum(*footer_copy));

    // Write dynamic header
    VhdDynamicHeader* dyn_header = reinterpret_cast<VhdDynamicHeader*>(
        data.data() + dyn_header_offset);
    std::memset(dyn_header, 0, sizeof(VhdDynamicHeader));

    dyn_header->cookie = swap64(VHD_DYNAMIC_COOKIE);
    dyn_header->data_offset = swap64(0xFFFFFFFFFFFFFFFFULL);
    dyn_header->table_offset = swap64(bat_offset);
    dyn_header->header_version = swap32(VHD_VERSION);
    dyn_header->max_table_entries = swap32(num_blocks);
    dyn_header->block_size = swap32(block_size);
    dyn_header->checksum = swap32(calculate_checksum(*dyn_header));

    // Write BAT
    uint32_t* bat_ptr = reinterpret_cast<uint32_t*>(data.data() + bat_offset);
    for (uint32_t i = 0; i < num_blocks; i++) {
        bat_ptr[i] = swap32(bat[i]);
    }

    // Write footer at end
    VhdFooter* footer = reinterpret_cast<VhdFooter*>(data.data() + total_size - sizeof(VhdFooter));
    std::memcpy(footer, footer_copy, sizeof(VhdFooter));

    return FCONVERT_OK;
}

fconvert_error_t VHDCodec::create_from_raw(
    const std::vector<uint8_t>& raw_data,
    VhdImage& image,
    VhdDiskType type) {

    image.type = type;
    image.disk_size = raw_data.size();
    image.geometry = calculate_geometry(image.disk_size);
    image.block_size = VHD_DEFAULT_BLOCK_SIZE;
    image.data = raw_data;

    generate_uuid(image.unique_id.data());

    return FCONVERT_OK;
}

fconvert_error_t VHDCodec::extract_raw(
    const VhdImage& image,
    std::vector<uint8_t>& raw_data) {

    if (image.type == VhdDiskType::Fixed) {
        raw_data = image.data;
    } else if (image.type == VhdDiskType::Dynamic) {
        raw_data.resize(image.disk_size, 0);

        for (size_t i = 0; i < image.blocks.size(); i++) {
            if (!image.blocks[i].empty()) {
                uint64_t offset = i * image.block_size;
                uint64_t copy_size = std::min(
                    static_cast<uint64_t>(image.blocks[i].size()),
                    image.disk_size - offset);
                std::memcpy(raw_data.data() + offset,
                           image.blocks[i].data(), copy_size);
            }
        }
    } else {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    return FCONVERT_OK;
}

fconvert_error_t VHDCodec::read_sector(
    const VhdImage& image,
    uint64_t sector,
    uint8_t* buffer) {

    uint64_t offset = sector * VHD_SECTOR_SIZE;
    if (offset + VHD_SECTOR_SIZE > image.disk_size) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    if (image.type == VhdDiskType::Fixed) {
        if (offset + VHD_SECTOR_SIZE <= image.data.size()) {
            std::memcpy(buffer, image.data.data() + offset, VHD_SECTOR_SIZE);
        } else {
            std::memset(buffer, 0, VHD_SECTOR_SIZE);
        }
    } else if (image.type == VhdDiskType::Dynamic) {
        uint32_t block_idx = static_cast<uint32_t>(offset / image.block_size);
        uint32_t block_offset = static_cast<uint32_t>(offset % image.block_size);

        if (block_idx < image.blocks.size() && !image.blocks[block_idx].empty()) {
            std::memcpy(buffer, image.blocks[block_idx].data() + block_offset, VHD_SECTOR_SIZE);
        } else {
            std::memset(buffer, 0, VHD_SECTOR_SIZE);
        }
    } else {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
