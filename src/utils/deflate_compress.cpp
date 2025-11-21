/**
 * DEFLATE compression implementation (continued)
 * Fixed Huffman and LZ77 integration
 */

#include "deflate.h"
#include "lz77.h"
#include <algorithm>

namespace fconvert {
namespace utils {

void Deflate::get_fixed_codes(HuffmanCode* lit_codes, HuffmanCode* dist_codes) {
    // Fixed literal/length codes (RFC 1951)
    // 0-143: 00110000 through 10111111 (8 bits, values 48-191)
    for (int i = 0; i <= 143; i++) {
        lit_codes[i].code = i + 48;
        lit_codes[i].length = 8;
    }

    // 144-255: 110010000 through 111111111 (9 bits, values 400-511)
    for (int i = 144; i <= 255; i++) {
        lit_codes[i].code = i + 256;
        lit_codes[i].length = 9;
    }

    // 256-279: 0000000 through 0010111 (7 bits, values 0-23)
    for (int i = 256; i <= 279; i++) {
        lit_codes[i].code = i - 256;
        lit_codes[i].length = 7;
    }

    // 280-287: 11000000 through 11000111 (8 bits, values 192-199)
    for (int i = 280; i <= 287; i++) {
        lit_codes[i].code = i - 88;
        lit_codes[i].length = 8;
    }

    // Fixed distance codes (all 5 bits)
    for (int i = 0; i < 32; i++) {
        dist_codes[i].code = i;
        dist_codes[i].length = 5;
    }
}

fconvert_error_t Deflate::compress_fixed_huffman(
    const uint8_t* input_data,
    size_t input_size,
    std::vector<uint8_t>& output,
    int level) {

    // Perform LZ77 compression
    LZ77 lz77;
    std::vector<LZ77Token> tokens;
    lz77.compress(input_data, input_size, tokens, level);

    // Get fixed Huffman codes
    HuffmanCode lit_codes[288];
    HuffmanCode dist_codes[32];
    get_fixed_codes(lit_codes, dist_codes);

    // Length codes extra bits
    static const int length_extra[29] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };
    static const int length_base[29] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
    };

    // Distance codes extra bits
    static const int dist_extra[30] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
    };
    static const int dist_base[30] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577
    };

    // Write compressed data
    BitWriter writer;

    // Block header: BFINAL=1, BTYPE=01 (fixed Huffman)
    writer.write_bits(1, 1);  // Final block
    writer.write_bits(1, 2);  // Fixed Huffman

    // Write tokens
    for (const auto& token : tokens) {
        if (token.is_literal) {
            // Write literal
            writer.write_bits_reverse(lit_codes[token.literal].code, lit_codes[token.literal].length);
        } else {
            // Write length/distance pair
            int len = token.match.length;
            int dist = token.match.distance;

            // Find length code
            int len_code = 0;
            for (int i = 0; i < 29; i++) {
                if (len < length_base[i]) break;
                len_code = i;
            }
            if (len_code < 28 && len >= length_base[len_code + 1]) {
                len_code++;
            }

            // Write length code
            int len_symbol = 257 + len_code;
            writer.write_bits_reverse(lit_codes[len_symbol].code, lit_codes[len_symbol].length);

            // Write length extra bits
            if (length_extra[len_code] > 0) {
                int extra = len - length_base[len_code];
                writer.write_bits(extra, length_extra[len_code]);
            }

            // Find distance code
            int dist_code = 0;
            for (int i = 0; i < 30; i++) {
                if (dist < dist_base[i]) break;
                dist_code = i;
            }
            if (dist_code < 29 && dist >= dist_base[dist_code + 1]) {
                dist_code++;
            }

            // Write distance code
            writer.write_bits_reverse(dist_codes[dist_code].code, dist_codes[dist_code].length);

            // Write distance extra bits
            if (dist_extra[dist_code] > 0) {
                int extra = dist - dist_base[dist_code];
                writer.write_bits(extra, dist_extra[dist_code]);
            }
        }
    }

    // Write end-of-block symbol (256)
    writer.write_bits_reverse(lit_codes[256].code, lit_codes[256].length);

    // Get compressed data
    writer.get_data(output);

    return FCONVERT_OK;
}

void Deflate::build_huffman_codes(
    const uint32_t* freqs,
    int num_symbols,
    HuffmanCode* codes,
    int max_bits) {

    // TODO: Implement optimal Huffman code generation
    // For now, we're using fixed codes
    (void)freqs;
    (void)num_symbols;
    (void)codes;
    (void)max_bits;
}

} // namespace utils
} // namespace fconvert
