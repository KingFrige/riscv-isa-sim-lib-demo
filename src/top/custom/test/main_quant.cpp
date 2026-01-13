// +FHDR========================================================================
//  File Name:      main_quant.cpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BF16 to MxFP8 Quantization Test with Reference Implementation
// -FHDR========================================================================
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <vector>
#include "BF16.hpp"
#include "MxFp8ActQuant.hpp"
#include "util.h"
#include <cstring>

using namespace BF16;

// ============================================================================
#define ENABLE_HW_MODEL   1
#define ENABLE_REF_MODEL  1
// ============================================================================

#if !ENABLE_HW_MODEL && !ENABLE_REF_MODEL
#error "At least one model (HW or Ref) must be enabled!"
#endif

void decode_mxfp8(uint8_t mxfp8, uint8_t *sign, uint8_t *exp, uint8_t *mat) {
    *sign = (mxfp8 >> 7) & 0x1;
    *exp = (mxfp8 >> 3) & 0xF;
    *mat = mxfp8 & 0x7;
}

// Standard E4M3 (bias=7) to float
// Normal    (E>0): v = (-1)^S × 2^(E-7) × (1 + M/8)
// Subnormal (E=0): v = (-1)^S × 2^(-6) × (0 + M/8)
// NaN: E=15, M=7 (0x7F or 0xFF)
static inline float fp8_e4m3_to_float(uint8_t fp8) {
    if (fp8 == 0 || fp8 == 0x80) return 0.0f;

    uint32_t sign = (fp8 >> 7) & 0x1;
    uint32_t exp = (fp8 >> 3) & 0xF;
    uint32_t mant = fp8 & 0x7;

    // Check for NaN: exp=15 (0xF) and mant=7
    if (exp == 0xF && mant == 0x7) {
        // Return NaN with appropriate sign
        uint32_t nan_bits = (sign << 31) | 0x7FC00000;  // Quiet NaN
        float nan;
        memcpy(&nan, &nan_bits, sizeof(float));
        return nan;
    }

    int32_t fp32_exp;
    uint32_t fp32_mant;

    if (exp == 0) {
        // Subnormal: v = 2^(-6) × (mant/8)
        // Need to normalize for FP32 representation
        if (mant == 0) return sign ? -0.0f : 0.0f;

        // Find leading 1 in 3-bit mantissa (0b001, 0b010, 0b011, 0b100, 0b101, 0b110, 0b111)
        // Count leading zeros
        int lz = (mant & 0x4) ? 0 : (mant & 0x2) ? 1 : 2;

        // Normalized exponent: 2^(-6-lz-1) for FP32
        fp32_exp = (-6 - lz - 1) + 127;

        // Shift mantissa to remove leading 1 and align to FP32 format
        uint32_t normalized_mant = (mant << (lz + 1)) & 0x7;  // Remove leading 1
        fp32_mant = normalized_mant << 20;
    } else {
        // Normal: v = 2^(exp-7) × (1 + mant/8)
        // E4M3: exponent bias = 7
        int32_t exp_unbias = (int32_t)exp - 7;
        fp32_exp = exp_unbias + 127;

        if (fp32_exp >= 255) fp32_exp = 254;  // Saturate to max

        // FP32 mantissa: the 3-bit mantissa goes to bits [22:20]
        fp32_mant = mant << 20;
    }

    uint32_t fp32_bits = (sign << 31) | (fp32_exp << 23) | fp32_mant;
    float result;
    memcpy(&result, &fp32_bits, sizeof(float));
    return result;
}

// E8M0 format: 8-bit exponent only, no mantissa
// Represents: 2^(exponent - 127)
static inline uint8_t float_to_fp8_e8m0(float val) {
    if (val <= 0.0f) return 0;  // Can't represent zero or negative

    // Calculate exponent: val = 2^exp, so exp = log2(val)
    float exp_float = log2f(val);
    int32_t exp = (int32_t)roundf(exp_float);

    // Add bias and clamp to [0, 255]
    int32_t biased_exp = exp + 127;
    if (biased_exp < 0) biased_exp = 0;
    if (biased_exp > 255) biased_exp = 255;

    return (uint8_t)biased_exp;
}

// MxFP8 to float: standard e4m3 (bias=7) + e8m0 scale
// Actual value = e4m3_value × 2^(scale_exponent)
// where scale_exponent = scale - 127
float mxfp8_to_float(uint8_t mxfp8, uint8_t scale) {
    // Step 1: Decode standard E4M3 (bias=7)
    float e4m3_value = fp8_e4m3_to_float(mxfp8);

    // Handle special cases
    if (e4m3_value == 0.0f) return 0.0f;
    if (std::isnan(e4m3_value)) return e4m3_value;

    // Step 2: Apply scale factor
    int32_t scale_exp = (int32_t)scale - 127;

    // Check if result would overflow to Inf
    // FP32 max exponent is 127 (exp bits = 254, not 255 which is Inf/NaN)
    // e4m3_value is at most 448 = 1.75 × 2^8
    // So we need: log2(e4m3_value) + scale_exp <= 127
    float log2_e4m3 = log2f(fabsf(e4m3_value));
    if (log2_e4m3 + scale_exp > 127.0f) {
        // Would overflow: saturate to FP32 max
        uint32_t max_bits = 0x7F7FFFFF;  // FP32 max positive
        if (e4m3_value < 0) max_bits |= 0x80000000;  // Set sign bit for negative
        float result;
        memcpy(&result, &max_bits, sizeof(float));
        return result;
    }

    float scale_factor = powf(2.0f, (float)scale_exp);
    return e4m3_value * scale_factor;
}

static inline uint8_t float_to_fp8_e4m3(float val) {
    if (val == 0.0f) return 0;

    uint32_t bits;
    memcpy(&bits, &val, sizeof(float));

    uint32_t sign = (bits >> 31) & 0x1;
    int32_t exp = ((bits >> 23) & 0xFF) - 127;
    uint32_t mantissa_fp32 = bits & 0x7FFFFF;

    if (exp > 8) {
        // Saturate to max: exp=15, mant=6 (NOT 7, which is NaN)
        return (sign << 7) | 0x7E;
    } else if (exp < -6) {
        if (exp < -9) {
            return (sign << 7);
        } else {
            int shift = -6 - exp;
            uint32_t mantissa_e4m3 = (8 + ((mantissa_fp32 >> 20) & 0x7)) >> shift;
            return (sign << 7) | (mantissa_e4m3 & 0x7);
        }
    }

    int32_t exp_e4m3 = exp + 7;
    uint32_t mantissa_e4m3 = (mantissa_fp32 >> 20) & 0x7;

    // Round to nearest even
    if ((mantissa_fp32 >> 19) & 0x1) {
        mantissa_e4m3 += 1;
        if (mantissa_e4m3 > 7) {
            mantissa_e4m3 = 0;
            exp_e4m3 += 1;
            if (exp_e4m3 > 15) {
                // Overflow: saturate to max value (exp=15, mant=6)
                return (sign << 7) | 0x7E;
            }
        }
    }

    // BUG FIX: Avoid NaN encoding (exp=15, mant=7)
    // E4M3 reserves exp=15, mant=7 for NaN
    if (exp_e4m3 == 15 && mantissa_e4m3 == 7) {
        mantissa_e4m3 = 6;  // Saturate to max representable value
    }

    return (sign << 7) | ((exp_e4m3 & 0xF) << 3) | (mantissa_e4m3 & 0x7);
}

// Reference: BF16 to MxFP8 (standard e4m3 with bias=7, e8m0 scale)
void reference_quantize_mxfp8(const uint16_t *bf16_input, uint8_t *mxfp8_output,
                               uint8_t *scale_output, uint32_t block_size) {
    // Find max absolute value
    float max_abs = 0.0f;
    for (uint32_t i = 0; i < block_size; i++) {
        float val = fabsf(bf16_to_float(bf16_input[i]));
        if (val > max_abs) max_abs = val;
    }

    if (max_abs == 0.0f) {
        *scale_output = 0;
        for (uint32_t i = 0; i < block_size; i++) mxfp8_output[i] = 0;
        return;
    }

    // Calculate scale: E4M3 max value is 448 (exp=15, bias=7 -> real_exp=8, mant=1.75 -> 1.75*256=448)
    // We want: max_abs / 2^scale_exp <= 448
    // scale_exp = floor(log2(max_abs)) - 8
    int exp_max = (int)floorf(log2f(max_abs));
    int scale_exp = exp_max - 8;
    scale_exp = (scale_exp < -127) ? -127 : (scale_exp > 127) ? 127 : scale_exp;
    *scale_output = (uint8_t)(scale_exp + 127);
    float scale_factor = powf(2.0f, (float)scale_exp);

    // Quantize each value
    for (uint32_t i = 0; i < block_size; i++) {
        float val = bf16_to_float(bf16_input[i]);
        float scaled_val = val / scale_factor;
        mxfp8_output[i] = float_to_fp8_e4m3(scaled_val);
    }
}

void compare_results(const char *name, const uint16_t *bf16_input, uint32_t count) {
    const uint32_t BLOCK_SIZE = 32;
    uint32_t num_blocks = (count + BLOCK_SIZE - 1) / BLOCK_SIZE;

    LOG_PRINTF("\n========================================\n");
    LOG_PRINTF("%s (%d values in %d blocks)\n", name, count, num_blocks);
    LOG_PRINTF("========================================\n");

    for (uint32_t blk = 0; blk < num_blocks; blk++) {
        uint32_t blk_start = blk * BLOCK_SIZE;
        uint32_t blk_count = (blk_start + BLOCK_SIZE <= count) ? BLOCK_SIZE : (count - blk_start);

#if ENABLE_HW_MODEL
        MxFp8ActQuant_Model hw_quant(BLOCK_SIZE);
        hw_quant.process(&bf16_input[blk_start]);
#endif

#if ENABLE_REF_MODEL
        uint8_t ref_output[BLOCK_SIZE];
        uint8_t ref_scale;
        reference_quantize_mxfp8(&bf16_input[blk_start], ref_output, &ref_scale, blk_count);
#endif

        LOG_PRINTF("\n--- Block %d (Index %d-%d) ---\n", blk, blk_start, blk_start + blk_count - 1);

        // Print scale info based on enabled models
#if ENABLE_HW_MODEL && ENABLE_REF_MODEL
        LOG_PRINTF("HW_Scale=0x%02x  Ref_Scale=0x%02x\n", hw_quant.o_MxFp8ActScale, ref_scale);
#elif ENABLE_HW_MODEL
        LOG_PRINTF("HW_Scale=0x%02x\n", hw_quant.o_MxFp8ActScale);
#elif ENABLE_REF_MODEL
        LOG_PRINTF("Ref_Scale=0x%02x\n", ref_scale);
#endif

        // Print table header based on enabled models
#if ENABLE_HW_MODEL && ENABLE_REF_MODEL
        LOG_PRINTF("Index | BF16_Input         | HW_Quant | HW_Dequant      | HW_Err(%%) | Ref_Quant | Ref_Dequant     | Ref_Err(%%)\n");
        LOG_PRINTF("------|--------------------|----------|-----------------|-----------|-----------|-----------------|----------\n");
#elif ENABLE_HW_MODEL
        LOG_PRINTF("Index | BF16_Input         | HW_Quant | HW_Dequant      | HW_Err(%%)\n");
        LOG_PRINTF("------|--------------------|----------|-----------------|----------\n");
#elif ENABLE_REF_MODEL
        LOG_PRINTF("Index | BF16_Input         | Ref_Quant | Ref_Dequant     | Ref_Err(%%)\n");
        LOG_PRINTF("------|--------------------|-----------|-----------------|----------\n");
#endif

        for (uint32_t i = 0; i < blk_count; i++) {
            uint32_t global_idx = blk_start + i;

            float original = bf16_to_float(bf16_input[global_idx]);

            // Skip if input is NaN or Inf
            if (!std::isfinite(original)) {
                continue;
            }

            bool skip_line = false;

#if ENABLE_HW_MODEL
            uint8_t hw_val = hw_quant.o_MxFp8Act[i];
            float hw_dequant = mxfp8_to_float(hw_val, hw_quant.o_MxFp8ActScale);
            float hw_error = 0.0f;
            float hw_rel_error = 0.0f;

            // Skip if dequant result is NaN or Inf
            if (!std::isfinite(hw_dequant)) {
                skip_line = true;
            } else {
                hw_error = fabsf(original - hw_dequant);
                // Calculate relative error for non-zero values
                hw_rel_error = (fabsf(original) > 0.0f) ? (hw_error / fabsf(original)) : 0.0f;
            }
#endif

#if ENABLE_REF_MODEL
            uint8_t ref_val = ref_output[i];
            float ref_dequant = mxfp8_to_float(ref_val, ref_scale);
            float ref_error = 0.0f;
            float ref_rel_error = 0.0f;

            // Skip if dequant result is NaN or Inf
            if (!std::isfinite(ref_dequant)) {
                skip_line = true;
            } else {
                ref_error = fabsf(original - ref_dequant);
                // Calculate relative error for non-zero values
                ref_rel_error = (fabsf(original) > 0.0f) ? (ref_error / fabsf(original)) : 0.0f;
            }
#endif

            // Skip printing if any model produced NaN/Inf
            if (skip_line) {
                continue;
            }

            // Print based on enabled models
#if ENABLE_HW_MODEL && ENABLE_REF_MODEL
            LOG_PRINTF("%5d | 0x%04x(%+.3e) |   0x%02x   | %+.6e   | %8.2f%% |   0x%02x    | %+.6e   | %7.2f%%\n",
                       global_idx, bf16_input[global_idx], original,
                       hw_val, hw_dequant, hw_rel_error * 100.0f,
                       ref_val, ref_dequant, ref_rel_error * 100.0f);
#elif ENABLE_HW_MODEL
            LOG_PRINTF("%5d | 0x%04x(%+.3e) |   0x%02x   | %+.6e   | %8.2f%%\n",
                       global_idx, bf16_input[global_idx], original,
                       hw_val, hw_dequant, hw_rel_error * 100.0f);
#elif ENABLE_REF_MODEL
            LOG_PRINTF("%5d | 0x%04x(%+.3e) |   0x%02x    | %+.6e   | %7.2f%%\n",
                       global_idx, bf16_input[global_idx], original,
                       ref_val, ref_dequant, ref_rel_error * 100.0f);
#endif

        }
    }
}

int main(void) {
    srand(time(NULL));
    log_init("./log/quant_output.log");

    TERMINAL_PRINTF("BF16 to MxFP8 Quantization Test\n");
    TERMINAL_PRINTF("Format: e4m3 data, e8m0 scale\n");


    const uint32_t SWEEP_STRIDE = 1;
    // Adjust SWEEP_STRIDE:
    //   1 = full coverage (65536 samples) - comprehensive but slow
    //   16 = high density (4096 samples) - good coverage
    //   64 = medium density (1024 samples) - balanced
    //   256 = low density (256 samples) - fast
    //   1024 = sparse (64 samples) - very fast

    uint32_t num_samples = 65536 / SWEEP_STRIDE;
    std::vector<uint16_t> bf16_sweep(num_samples);

    TERMINAL_PRINTF("BF16 Comprehensive Sweep Test (stride=%d, samples=%d)\n\n",
                    SWEEP_STRIDE, num_samples);

    for (uint32_t i = 0; i < num_samples; i++) {
        bf16_sweep[i] = (uint16_t)(i * SWEEP_STRIDE);
    }

    compare_results("BF16 Sweep", bf16_sweep.data(), num_samples);

    log_close();
    TERMINAL_PRINTF("\nResults saved to log/quant_output.log\n");
    return 0;
}
