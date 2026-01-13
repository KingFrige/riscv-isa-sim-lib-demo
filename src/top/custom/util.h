// +FHDR========================================================================
//  License:
//      
//  =============================================================================
//  File Name:      util.h
//                  Rocky (luoqi754@gmail.com)
//  Organization:
//  Description:
//      Utility functions for the C model.
// -FHDR========================================================================
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============ Log Functions ============
void log_init(const char *log_path);
void log_close(void);

// Get log file pointer (for internal use)
FILE* log_get_file(void);

#define LOG_PRINTF(...) do { \
    FILE *logf = log_get_file(); \
    if (logf) { \
        fprintf(logf, __VA_ARGS__); \
        fflush(logf); \
    } \
} while(0)

#define TERMINAL_PRINTF(...) printf(__VA_ARGS__)


// Bit mask generation: return w-bit all-ones mask
static inline uint64_t bitmask_u64(unsigned w) {
    return (w == 64) ? ~0ULL : ((1ULL << w) - 1ULL);
}

// Sign extend w-bit value to 64-bit signed integer
static inline int64_t sign_extend(uint32_t val, unsigned w) {
    assert(w >= 1 && w <= 32);
    unsigned shift = 32 - w;
    int32_t extended = ((int32_t)(val << shift)) >> shift;
    return (int64_t)extended;
}

// 16-way signed integer adder tree (matches IntAdderTree.v)
static inline int64_t IntAdderTree(const int64_t i_psum[16], int mat_width) {
    // DFF0: 16 -> 4
    int64_t psum_l1[4];
    for (int i = 0; i < 4; ++i) {
        int64_t sum4 = sign_extend(i_psum[i*4+0], mat_width) + sign_extend(i_psum[i*4+1], mat_width) +
                       sign_extend(i_psum[i*4+2], mat_width) + sign_extend(i_psum[i*4+3], mat_width);
        psum_l1[i] = sum4;
    }

    // DFF1: 4 -> 1
    int64_t sum = psum_l1[0] + psum_l1[1] + psum_l1[2] + psum_l1[3];
    return sum >> 4;
}


// Count leading zeros from MSB (matches RTL MatShiftStep)
static inline unsigned MatShiftStep(uint64_t i_Mat,int mat_width,int mat_shift) {
    unsigned shift_amt = 0;
    for (int j = 1; j <= mat_width; ++j) {
        if (i_Mat & (1ULL << (mat_width - j))) {
            break;
        }
        shift_amt++;
    }
    // Truncate to MAT_SHIFT bits (matching RTL output width)
    return shift_amt & bitmask_u64(mat_shift);
}

// Find maximum exponent from array
uint64_t ExpMax(const uint64_t *exps,
                unsigned        exp_width,
                unsigned        data_size);

#ifdef __cplusplus
}
#endif

#endif // UTIL_H
