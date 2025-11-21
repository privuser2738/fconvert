/**
 * DEFLATE compression/decompression (RFC 1951)
 * This is the compression algorithm used by PNG, ZIP, and GZIP
 *
 * DEFLATE combines:
 * 1. LZ77 sliding window compression (finds repeated sequences)
 * 2. Huffman coding (entropy coding for the LZ77 output)
 */

#ifndef DEFLATE_H
#define DEFLATE_H

#include "../../include/fconvert.h"
#include <vector>
#include <cstdint>

namespace fconvert {
namespace utils {

// Bit stream reader for reading compressed data bit-by-bit
class BitStream {
public:
    BitStream(const uint8_t* data, size_t size);

    // Read n bits (up to 16 bits at a time)
    uint16_t read_bits(int count);

    // Read bits in reverse order (for Huffman codes)
    uint16_t read_bits_reverse(int count);

    // Align to next byte boundary
    void align_to_byte();

    // Get current byte position
    size_t get_position() const { return byte_pos_; }

    // Check if we have more data
    bool has_data() const { return byte_pos_ < size_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t byte_pos_;
    uint32_t bit_buffer_;
    int bits_available_;

    void fill_buffer();
};

// Huffman tree node for decoding
struct HuffmanNode {
    int16_t symbol;      // -1 if internal node, otherwise the symbol value
    uint16_t left;       // Index of left child (or 0 if leaf)
    uint16_t right;      // Index of right child (or 0 if leaf)
};

// Huffman tree for decoding
class HuffmanTree {
public:
    HuffmanTree();

    // Build tree from code lengths
    bool build_from_lengths(const uint8_t* lengths, int num_symbols);

    // Decode one symbol from bit stream
    int decode_symbol(BitStream& stream);

    // Check if tree is valid
    bool is_valid() const { return valid_; }

private:
    std::vector<HuffmanNode> nodes_;
    bool valid_;
    int root_;

    void clear();
};

// Bit writer for writing compressed data
class BitWriter {
public:
    BitWriter();

    void write_bits(uint32_t bits, int count);
    void write_bits_reverse(uint32_t bits, int count);
    void align_to_byte();
    void get_data(std::vector<uint8_t>& output);

private:
    std::vector<uint8_t> data_;
    uint32_t bit_buffer_;
    int bits_in_buffer_;

    void flush_byte();
};

// Huffman code for encoding
struct HuffmanCode {
    uint16_t code;
    uint8_t length;
};

// DEFLATE decompressor (inflate)
class Inflate {
public:
    Inflate();

    // Decompress DEFLATE data
    fconvert_error_t decompress(
        const uint8_t* compressed_data,
        size_t compressed_size,
        std::vector<uint8_t>& output);

private:
    // Process one DEFLATE block
    bool process_block(BitStream& stream, std::vector<uint8_t>& output);

    // Process uncompressed block
    bool process_uncompressed(BitStream& stream, std::vector<uint8_t>& output);

    // Process block with fixed Huffman codes
    bool process_fixed_huffman(BitStream& stream, std::vector<uint8_t>& output);

    // Process block with dynamic Huffman codes
    bool process_dynamic_huffman(BitStream& stream, std::vector<uint8_t>& output);

    // Decode literal/length and distance values
    bool decode_huffman_data(
        BitStream& stream,
        HuffmanTree& lit_tree,
        HuffmanTree& dist_tree,
        std::vector<uint8_t>& output);

    // LZ77 copy from history
    void lz77_copy(std::vector<uint8_t>& output, int distance, int length);

    // Build fixed Huffman trees
    void build_fixed_trees();

    HuffmanTree fixed_lit_tree_;
    HuffmanTree fixed_dist_tree_;
};

// DEFLATE compressor (deflate)
class Deflate {
public:
    Deflate();

    // Compress data using DEFLATE
    fconvert_error_t compress(
        const uint8_t* input_data,
        size_t input_size,
        std::vector<uint8_t>& output,
        int level = 6);  // Compression level 0-9

private:
    // Compress using uncompressed blocks
    fconvert_error_t compress_uncompressed(
        const uint8_t* input_data,
        size_t input_size,
        std::vector<uint8_t>& output);

    // Compress using fixed Huffman codes
    fconvert_error_t compress_fixed_huffman(
        const uint8_t* input_data,
        size_t input_size,
        std::vector<uint8_t>& output,
        int level);

    // Build Huffman codes from frequencies
    void build_huffman_codes(
        const uint32_t* freqs,
        int num_symbols,
        HuffmanCode* codes,
        int max_bits);

    // Get fixed Huffman codes
    void get_fixed_codes(HuffmanCode* lit_codes, HuffmanCode* dist_codes);
};

} // namespace utils
} // namespace fconvert

#endif // DEFLATE_H
