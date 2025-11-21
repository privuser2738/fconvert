/**
 * LZ77 compression implementation
 * Sliding window dictionary compression
 * Used by DEFLATE
 */

#ifndef LZ77_H
#define LZ77_H

#include <cstdint>
#include <vector>

namespace fconvert {
namespace utils {

// LZ77 match structure
struct LZ77Match {
    uint16_t length;    // Length of match (3-258)
    uint16_t distance;  // Distance back in window (1-32768)
};

// LZ77 token - either a literal or a match
struct LZ77Token {
    bool is_literal;
    union {
        uint8_t literal;
        LZ77Match match;
    };
};

class LZ77 {
public:
    LZ77();

    // Compress data using LZ77
    // Returns a sequence of tokens (literals and length/distance pairs)
    void compress(
        const uint8_t* data,
        size_t size,
        std::vector<LZ77Token>& tokens,
        int level = 6);  // 0-9 compression level

private:
    // Find longest match in the sliding window
    LZ77Match find_match(
        const uint8_t* data,
        size_t pos,
        size_t size,
        size_t window_start);

    // Hash-based match finding (faster)
    LZ77Match find_match_hash(
        const uint8_t* data,
        size_t pos,
        size_t size,
        size_t window_start);

    // Constants
    static const size_t WINDOW_SIZE = 32768;  // 32KB sliding window
    static const size_t MIN_MATCH = 3;        // Minimum match length
    static const size_t MAX_MATCH = 258;      // Maximum match length
    static const size_t HASH_SIZE = 8192;     // Hash table size

    // Hash table for fast match finding
    std::vector<int> hash_table_;
    std::vector<int> prev_;

    uint32_t hash3(const uint8_t* data) const {
        return ((data[0] << 10) ^ (data[1] << 5) ^ data[2]) & (HASH_SIZE - 1);
    }
};

} // namespace utils
} // namespace fconvert

#endif // LZ77_H
