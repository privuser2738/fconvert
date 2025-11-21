/**
 * Math utility functions implementation
 */

#include "math_utils.h"

namespace fconvert {
namespace utils {

uint32_t MathUtils::fast_div(uint32_t dividend, uint32_t divisor) {
    if (divisor == 0) return 0;
    return dividend / divisor;
}

uint32_t MathUtils::next_power_of_2(uint32_t value) {
    if (value == 0) return 1;

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
}

bool MathUtils::is_power_of_2(uint32_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

uint32_t MathUtils::gcd(uint32_t a, uint32_t b) {
    while (b != 0) {
        uint32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

uint32_t MathUtils::lcm(uint32_t a, uint32_t b) {
    if (a == 0 || b == 0) return 0;
    return (a * b) / gcd(a, b);
}

} // namespace utils
} // namespace fconvert
