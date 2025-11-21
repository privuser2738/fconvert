/**
 * LZ77 compression implementation
 */

#include "lz77.h"
#include <algorithm>
#include <cstring>

namespace fconvert {
namespace utils {

LZ77::LZ77() {
}

void LZ77::compress(
    const uint8_t* data,
    size_t size,
    std::vector<LZ77Token>& tokens,
    int level) {

    tokens.clear();

    if (size == 0) return;

    // Initialize hash table
    hash_table_.assign(HASH_SIZE, -1);
    prev_.assign(size, -1);

    size_t pos = 0;

    while (pos < size) {
        size_t window_start = (pos >= WINDOW_SIZE) ? (pos - WINDOW_SIZE) : 0;

        // Find best match
        LZ77Match match = {0, 0};

        if (level >= 6 && pos + MIN_MATCH <= size) {
            match = find_match_hash(data, pos, size, window_start);
        } else if (level >= 1 && pos + MIN_MATCH <= size) {
            match = find_match(data, pos, size, window_start);
        }

        if (match.length >= MIN_MATCH) {
            // Output match
            LZ77Token token;
            token.is_literal = false;
            token.match = match;
            tokens.push_back(token);

            // Update hash table for all bytes in match
            for (uint16_t i = 0; i < match.length && pos < size; i++, pos++) {
                if (pos + 2 < size) {
                    uint32_t h = hash3(data + pos);
                    prev_[pos] = hash_table_[h];
                    hash_table_[h] = pos;
                }
            }
        } else {
            // Output literal
            LZ77Token token;
            token.is_literal = true;
            token.literal = data[pos];
            tokens.push_back(token);

            // Update hash table
            if (pos + 2 < size) {
                uint32_t h = hash3(data + pos);
                prev_[pos] = hash_table_[h];
                hash_table_[h] = pos;
            }

            pos++;
        }
    }
}

LZ77Match LZ77::find_match(
    const uint8_t* data,
    size_t pos,
    size_t size,
    size_t window_start) {

    LZ77Match best_match = {0, 0};

    // Simple brute force search
    for (size_t i = window_start; i < pos; i++) {
        size_t match_len = 0;
        size_t max_len = std::min(MAX_MATCH, size - pos);

        while (match_len < max_len && data[i + match_len] == data[pos + match_len]) {
            match_len++;
        }

        if (match_len >= MIN_MATCH && match_len > best_match.length) {
            best_match.length = match_len;
            best_match.distance = pos - i;
        }
    }

    return best_match;
}

LZ77Match LZ77::find_match_hash(
    const uint8_t* data,
    size_t pos,
    size_t size,
    size_t window_start) {

    LZ77Match best_match = {0, 0};

    if (pos + 2 >= size) {
        return best_match;
    }

    uint32_t h = hash3(data + pos);
    int match_pos = hash_table_[h];

    // Follow hash chain
    int chain_len = 0;
    const int max_chain = 128; // Limit chain length for performance

    while (match_pos >= (int)window_start && chain_len < max_chain) {
        if (match_pos >= (int)pos) break;

        chain_len++;

        // Quick check: first and last bytes
        size_t max_len = std::min(MAX_MATCH, size - pos);

        if (data[match_pos] == data[pos] &&
            data[match_pos + best_match.length] == data[pos + best_match.length]) {

            // Count matching bytes
            size_t match_len = 0;
            while (match_len < max_len && data[match_pos + match_len] == data[pos + match_len]) {
                match_len++;
            }

            if (match_len > best_match.length) {
                best_match.length = match_len;
                best_match.distance = pos - match_pos;

                // Early exit if we found a really good match
                if (match_len >= 128) break;
            }
        }

        match_pos = prev_[match_pos];
    }

    return best_match;
}

} // namespace utils
} // namespace fconvert
