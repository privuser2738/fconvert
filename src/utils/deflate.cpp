/**
 * DEFLATE decompression implementation
 * Based on RFC 1951
 */

#include "deflate.h"
#include <algorithm>
#include <cstring>

namespace fconvert {
namespace utils {

// ============================================================================
// BitStream Implementation
// ============================================================================

BitStream::BitStream(const uint8_t* data, size_t size)
    : data_(data)
    , size_(size)
    , byte_pos_(0)
    , bit_buffer_(0)
    , bits_available_(0) {
}

void BitStream::fill_buffer() {
    while (bits_available_ < 16 && byte_pos_ < size_) {
        bit_buffer_ |= (uint32_t)data_[byte_pos_++] << bits_available_;
        bits_available_ += 8;
    }
}

uint16_t BitStream::read_bits(int count) {
    fill_buffer();

    if (count > bits_available_) {
        return 0; // Error
    }

    uint16_t result = bit_buffer_ & ((1 << count) - 1);
    bit_buffer_ >>= count;
    bits_available_ -= count;

    return result;
}

uint16_t BitStream::read_bits_reverse(int count) {
    uint16_t bits = read_bits(count);
    uint16_t result = 0;
    for (int i = 0; i < count; i++) {
        result = (result << 1) | (bits & 1);
        bits >>= 1;
    }
    return result;
}

void BitStream::align_to_byte() {
    int skip = bits_available_ & 7;
    if (skip > 0) {
        read_bits(skip);
    }
}

// ============================================================================
// HuffmanTree Implementation
// ============================================================================

HuffmanTree::HuffmanTree()
    : valid_(false)
    , root_(-1) {
}

void HuffmanTree::clear() {
    nodes_.clear();
    valid_ = false;
    root_ = -1;
}

bool HuffmanTree::build_from_lengths(const uint8_t* lengths, int num_symbols) {
    clear();

    // Count code lengths
    int max_length = 0;
    int length_counts[16] = {0};

    for (int i = 0; i < num_symbols; i++) {
        if (lengths[i] > 0) {
            length_counts[lengths[i]]++;
            if (lengths[i] > max_length) {
                max_length = lengths[i];
            }
        }
    }

    if (max_length == 0) {
        return false; // No codes
    }

    // Generate codes using canonical Huffman algorithm
    int codes[16] = {0};
    int code = 0;

    for (int len = 1; len <= max_length; len++) {
        code = (code + length_counts[len - 1]) << 1;
        codes[len] = code;
    }

    // Build decode table (simple array lookup for speed)
    // We'll use a different approach: build actual tree

    // Allocate enough nodes (worst case: full binary tree)
    nodes_.resize(num_symbols * 2);
    int next_node = 1;
    root_ = 0;

    nodes_[0].symbol = -1;
    nodes_[0].left = 0;
    nodes_[0].right = 0;

    // Insert each symbol into the tree
    for (int i = 0; i < num_symbols; i++) {
        if (lengths[i] == 0) continue;

        int len = lengths[i];
        int code_val = codes[len]++;

        // Reverse bits (Huffman codes are read MSB first)
        int reversed = 0;
        for (int b = 0; b < len; b++) {
            reversed = (reversed << 1) | (code_val & 1);
            code_val >>= 1;
        }

        // Insert into tree
        int node = root_;
        for (int b = 0; b < len; b++) {
            int bit = (reversed >> b) & 1;

            if (b == len - 1) {
                // Leaf node
                if (bit == 0) {
                    if (nodes_[node].left != 0) return false; // Duplicate
                    nodes_[node].left = next_node;
                    nodes_[next_node].symbol = i;
                    nodes_[next_node].left = 0;
                    nodes_[next_node].right = 0;
                    next_node++;
                } else {
                    if (nodes_[node].right != 0) return false; // Duplicate
                    nodes_[node].right = next_node;
                    nodes_[next_node].symbol = i;
                    nodes_[next_node].left = 0;
                    nodes_[next_node].right = 0;
                    next_node++;
                }
            } else {
                // Internal node
                uint16_t* next_ptr = (bit == 0) ? &nodes_[node].left : &nodes_[node].right;

                if (*next_ptr == 0) {
                    *next_ptr = next_node;
                    nodes_[next_node].symbol = -1;
                    nodes_[next_node].left = 0;
                    nodes_[next_node].right = 0;
                    next_node++;
                }
                node = *next_ptr;
            }
        }
    }

    valid_ = true;
    return true;
}

int HuffmanTree::decode_symbol(BitStream& stream) {
    if (!valid_) return -1;

    int node = root_;

    while (nodes_[node].symbol == -1) {
        int bit = stream.read_bits(1);

        if (bit == 0) {
            if (nodes_[node].left == 0) return -1; // Invalid code
            node = nodes_[node].left;
        } else {
            if (nodes_[node].right == 0) return -1; // Invalid code
            node = nodes_[node].right;
        }
    }

    return nodes_[node].symbol;
}

// ============================================================================
// Inflate Implementation
// ============================================================================

Inflate::Inflate() {
    build_fixed_trees();
}

void Inflate::build_fixed_trees() {
    // Fixed literal/length tree (RFC 1951 section 3.2.6)
    uint8_t lit_lengths[288];

    // 0-143: length 8
    for (int i = 0; i <= 143; i++) lit_lengths[i] = 8;
    // 144-255: length 9
    for (int i = 144; i <= 255; i++) lit_lengths[i] = 9;
    // 256-279: length 7
    for (int i = 256; i <= 279; i++) lit_lengths[i] = 7;
    // 280-287: length 8
    for (int i = 280; i <= 287; i++) lit_lengths[i] = 8;

    fixed_lit_tree_.build_from_lengths(lit_lengths, 288);

    // Fixed distance tree (all codes have length 5)
    uint8_t dist_lengths[32];
    for (int i = 0; i < 32; i++) dist_lengths[i] = 5;

    fixed_dist_tree_.build_from_lengths(dist_lengths, 32);
}

fconvert_error_t Inflate::decompress(
    const uint8_t* compressed_data,
    size_t compressed_size,
    std::vector<uint8_t>& output) {

    BitStream stream(compressed_data, compressed_size);
    output.clear();

    bool is_final = false;

    while (!is_final) {
        // Read block header
        is_final = stream.read_bits(1) != 0;
        int block_type = stream.read_bits(2);

        bool success = false;

        switch (block_type) {
            case 0: // Uncompressed
                success = process_uncompressed(stream, output);
                break;

            case 1: // Fixed Huffman
                success = process_fixed_huffman(stream, output);
                break;

            case 2: // Dynamic Huffman
                success = process_dynamic_huffman(stream, output);
                break;

            default:
                return FCONVERT_ERROR_CORRUPTED_FILE;
        }

        if (!success) {
            return FCONVERT_ERROR_CORRUPTED_FILE;
        }
    }

    return FCONVERT_OK;
}

bool Inflate::process_uncompressed(BitStream& stream, std::vector<uint8_t>& output) {
    stream.align_to_byte();

    // Read length and nlen
    uint16_t len = stream.read_bits(8) | (stream.read_bits(8) << 8);
    uint16_t nlen = stream.read_bits(8) | (stream.read_bits(8) << 8);

    // Verify one's complement
    if ((len ^ nlen) != 0xFFFF) {
        return false;
    }

    // Copy literal bytes
    for (int i = 0; i < len; i++) {
        output.push_back(stream.read_bits(8));
    }

    return true;
}

bool Inflate::process_fixed_huffman(BitStream& stream, std::vector<uint8_t>& output) {
    return decode_huffman_data(stream, fixed_lit_tree_, fixed_dist_tree_, output);
}

bool Inflate::process_dynamic_huffman(BitStream& stream, std::vector<uint8_t>& output) {
    // Read header
    int hlit = stream.read_bits(5) + 257;  // Number of literal/length codes
    int hdist = stream.read_bits(5) + 1;   // Number of distance codes
    int hclen = stream.read_bits(4) + 4;   // Number of code length codes

    // Read code length alphabet
    static const int code_length_order[19] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
    };

    uint8_t code_lengths[19] = {0};
    for (int i = 0; i < hclen; i++) {
        code_lengths[code_length_order[i]] = stream.read_bits(3);
    }

    // Build code length tree
    HuffmanTree code_tree;
    if (!code_tree.build_from_lengths(code_lengths, 19)) {
        return false;
    }

    // Decode literal/length and distance code lengths
    std::vector<uint8_t> lengths;
    lengths.reserve(hlit + hdist);

    while ((int)lengths.size() < hlit + hdist) {
        int symbol = code_tree.decode_symbol(stream);
        if (symbol < 0) return false;

        if (symbol < 16) {
            // Literal length
            lengths.push_back(symbol);
        } else if (symbol == 16) {
            // Repeat previous length 3-6 times
            if (lengths.empty()) return false;
            int repeat = stream.read_bits(2) + 3;
            uint8_t prev = lengths.back();
            for (int i = 0; i < repeat; i++) {
                lengths.push_back(prev);
            }
        } else if (symbol == 17) {
            // Repeat 0 for 3-10 times
            int repeat = stream.read_bits(3) + 3;
            for (int i = 0; i < repeat; i++) {
                lengths.push_back(0);
            }
        } else if (symbol == 18) {
            // Repeat 0 for 11-138 times
            int repeat = stream.read_bits(7) + 11;
            for (int i = 0; i < repeat; i++) {
                lengths.push_back(0);
            }
        }
    }

    // Build literal/length and distance trees
    HuffmanTree lit_tree, dist_tree;

    if (!lit_tree.build_from_lengths(lengths.data(), hlit)) {
        return false;
    }

    if (!dist_tree.build_from_lengths(lengths.data() + hlit, hdist)) {
        return false;
    }

    return decode_huffman_data(stream, lit_tree, dist_tree, output);
}

bool Inflate::decode_huffman_data(
    BitStream& stream,
    HuffmanTree& lit_tree,
    HuffmanTree& dist_tree,
    std::vector<uint8_t>& output) {

    // Extra bits for length codes
    static const int length_base[29] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
    };
    static const int length_extra[29] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };

    // Extra bits for distance codes
    static const int dist_base[30] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577
    };
    static const int dist_extra[30] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
    };

    while (true) {
        int symbol = lit_tree.decode_symbol(stream);
        if (symbol < 0) return false;

        if (symbol < 256) {
            // Literal byte
            output.push_back(symbol);
        } else if (symbol == 256) {
            // End of block
            break;
        } else {
            // Length/distance pair
            int len_code = symbol - 257;
            if (len_code >= 29) return false;

            int length = length_base[len_code];
            if (length_extra[len_code] > 0) {
                length += stream.read_bits(length_extra[len_code]);
            }

            int dist_code = dist_tree.decode_symbol(stream);
            if (dist_code < 0 || dist_code >= 30) return false;

            int distance = dist_base[dist_code];
            if (dist_extra[dist_code] > 0) {
                distance += stream.read_bits(dist_extra[dist_code]);
            }

            // Copy from history
            lz77_copy(output, distance, length);
        }
    }

    return true;
}

void Inflate::lz77_copy(std::vector<uint8_t>& output, int distance, int length) {
    size_t start = output.size() - distance;

    for (int i = 0; i < length; i++) {
        output.push_back(output[start + i]);
    }
}

// ============================================================================
// BitWriter Implementation
// ============================================================================

BitWriter::BitWriter()
    : bit_buffer_(0)
    , bits_in_buffer_(0) {
}

void BitWriter::write_bits(uint32_t bits, int count) {
    bit_buffer_ |= (bits << bits_in_buffer_);
    bits_in_buffer_ += count;

    while (bits_in_buffer_ >= 8) {
        flush_byte();
    }
}

void BitWriter::write_bits_reverse(uint32_t bits, int count) {
    uint32_t reversed = 0;
    for (int i = 0; i < count; i++) {
        reversed = (reversed << 1) | (bits & 1);
        bits >>= 1;
    }
    write_bits(reversed, count);
}

void BitWriter::align_to_byte() {
    if (bits_in_buffer_ > 0) {
        data_.push_back(bit_buffer_ & 0xFF);
        bit_buffer_ = 0;
        bits_in_buffer_ = 0;
    }
}

void BitWriter::flush_byte() {
    data_.push_back(bit_buffer_ & 0xFF);
    bit_buffer_ >>= 8;
    bits_in_buffer_ -= 8;
}

void BitWriter::get_data(std::vector<uint8_t>& output) {
    if (bits_in_buffer_ > 0) {
        data_.push_back(bit_buffer_ & 0xFF);
    }
    output = std::move(data_);
}

// ============================================================================
// Deflate (Compressor) Implementation
// ============================================================================

Deflate::Deflate() {
}

fconvert_error_t Deflate::compress(
    const uint8_t* input_data,
    size_t input_size,
    std::vector<uint8_t>& output,
    int level) {

    if (level == 0) {
        return compress_uncompressed(input_data, input_size, output);
    }

    // Use fixed Huffman for simplicity (better than uncompressed)
    return compress_fixed_huffman(input_data, input_size, output, level);
}

fconvert_error_t Deflate::compress_uncompressed(
    const uint8_t* input_data,
    size_t input_size,
    std::vector<uint8_t>& output) {

    output.clear();

    size_t pos = 0;
    const size_t max_block_size = 65535;

    while (pos < input_size) {
        size_t block_size = std::min(max_block_size, input_size - pos);
        bool is_final = (pos + block_size >= input_size);

        // Block header
        uint8_t header = is_final ? 1 : 0; // BFINAL bit
        // BTYPE = 00 (uncompressed)
        output.push_back(header);

        // Length and ~length
        uint16_t len = block_size;
        uint16_t nlen = ~len;

        output.push_back(len & 0xFF);
        output.push_back((len >> 8) & 0xFF);
        output.push_back(nlen & 0xFF);
        output.push_back((nlen >> 8) & 0xFF);

        // Copy data
        output.insert(output.end(), input_data + pos, input_data + pos + block_size);

        pos += block_size;
    }

    return FCONVERT_OK;
}

} // namespace utils
} // namespace fconvert
