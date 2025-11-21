/**
 * Math utility functions
 */

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cstdint>
#include <algorithm>

namespace fconvert {
namespace utils {

class MathUtils {
public:
    // Clamp value between min and max
    template<typename T>
    static T clamp(T value, T min, T max) {
        return std::max(min, std::min(max, value));
    }

    // Linear interpolation
    template<typename T>
    static T lerp(T a, T b, float t) {
        return a + (b - a) * t;
    }

    // Map value from one range to another
    template<typename T>
    static T map_range(T value, T in_min, T in_max, T out_min, T out_max) {
        return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
    }

    // Fast integer division
    static uint32_t fast_div(uint32_t dividend, uint32_t divisor);

    // Next power of 2
    static uint32_t next_power_of_2(uint32_t value);

    // Check if power of 2
    static bool is_power_of_2(uint32_t value);

    // Greatest common divisor
    static uint32_t gcd(uint32_t a, uint32_t b);

    // Least common multiple
    static uint32_t lcm(uint32_t a, uint32_t b);
};

} // namespace utils
} // namespace fconvert

#endif // MATH_UTILS_H
