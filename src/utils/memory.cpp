/**
 * Memory management utilities implementation
 */

#include "memory.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #include <malloc.h>
#endif

namespace fconvert {
namespace utils {

void* Memory::aligned_alloc(size_t size, size_t alignment) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) == 0) {
        return ptr;
    }
    return nullptr;
#endif
}

void Memory::aligned_free(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

bool Memory::safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (!dest || !src) return false;
    if (src_size > dest_size) return false;

    memcpy(dest, src, src_size);
    return true;
}

Memory::Pool::Pool(size_t block_size) : size_(block_size), used_(0) {
    buffer_ = new uint8_t[block_size];
}

Memory::Pool::~Pool() {
    delete[] buffer_;
}

void* Memory::Pool::allocate(size_t size) {
    // Align to 16 bytes
    size_t aligned_size = (size + 15) & ~15;

    if (used_ + aligned_size > size_) {
        return nullptr;
    }

    void* ptr = buffer_ + used_;
    used_ += aligned_size;
    return ptr;
}

void Memory::Pool::reset() {
    used_ = 0;
}

} // namespace utils
} // namespace fconvert
