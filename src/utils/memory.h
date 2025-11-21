/**
 * Memory management utilities
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>

namespace fconvert {
namespace utils {

class Memory {
public:
    // Aligned memory allocation
    static void* aligned_alloc(size_t size, size_t alignment);
    static void aligned_free(void* ptr);

    // Memory copy with bounds checking
    static bool safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size);

    // Memory pool for temporary allocations
    class Pool {
    public:
        Pool(size_t block_size);
        ~Pool();

        void* allocate(size_t size);
        void reset();
        size_t get_used_size() const { return used_; }
        size_t get_total_size() const { return size_; }

    private:
        uint8_t* buffer_;
        size_t size_;
        size_t used_;
    };
};

} // namespace utils
} // namespace fconvert

#endif // MEMORY_H
