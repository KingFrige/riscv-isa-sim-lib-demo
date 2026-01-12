// Utility functions for algorithms
#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <cassert>

// Sign extend w-bit value to 64-bit signed integer
static inline int64_t sign_extend(uint32_t val, unsigned w) {
    assert(w >= 1 && w <= 32);
    unsigned shift = 32 - w;
    int32_t extended = ((int32_t)(val << shift)) >> shift;
    return (int64_t)extended;
}

#endif // UTIL_H