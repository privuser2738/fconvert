/**
 * CRC32 checksum implementation
 * Used by PNG and other formats
 */

#ifndef CRC32_H
#define CRC32_H

#include <cstdint>
#include <cstddef>

namespace fconvert {
namespace utils {

class CRC32 {
public:
    CRC32();

    // Calculate CRC32 for a buffer
    static uint32_t calculate(const uint8_t* data, size_t length);

    // Calculate CRC32 with initial value (for streaming)
    static uint32_t calculate(const uint8_t* data, size_t length, uint32_t crc);

    // Update CRC32 with single byte
    static uint32_t update(uint32_t crc, uint8_t byte);

private:
    static const uint32_t table_[256];
    static void init_table(uint32_t* table);
};

} // namespace utils
} // namespace fconvert

#endif // CRC32_H
