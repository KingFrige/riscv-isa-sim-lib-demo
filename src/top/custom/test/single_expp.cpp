// +FHDR========================================================================
//  File Name:      single_expp.cpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    Single Value BF16 Expp Test with Detailed Debug Output
// -FHDR========================================================================
#include "BF16.hpp"
#include <cmath>
#include <cstring>
#include "../util.h"

using namespace BF16;

// Process with detailed debug output
uint16_t process_with_debug(uint16_t input) {
    // Constants (same as in BF16ExppUnit_Model)
    static constexpr uint8_t BIAS = 127;
    static constexpr uint16_t ONE_BF16 = 0x3F80;
    static constexpr uint16_t LN2_FLIP = 23637;  // (1/ln2) * 2^14
    static constexpr uint8_t  ALPHA = 4;         // 0.25
    static constexpr uint8_t  BETA = 7;          // 0.4375
    static constexpr uint16_t GAMMA1 = 363;      // 2.836
    static constexpr uint16_t GAMMA2 = 278;      // 2.168

    LOG_PRINTF("\n========================================\n");
    LOG_PRINTF("BF16 Expp Processing Steps\n");
    LOG_PRINTF("========================================\n\n");

    // Parse input
    BFloat16 in(input);
    bool sign = in.fields.sign;
    uint8_t exponent = in.fields.exponent;
    uint8_t mantissa = in.fields.mantissa;

    LOG_PRINTF("Input Analysis:\n");
    LOG_PRINTF("  Raw input:    0x%04x\n", input);
    LOG_PRINTF("  Float value:  %g\n", bf16_to_float(input));
    LOG_PRINTF("  Sign:         %d (%s)\n", (int)sign, sign ? "negative" : "positive");
    LOG_PRINTF("  Exponent:     %d (0x%02x)\n", (int)exponent, (int)exponent);
    LOG_PRINTF("  Mantissa:     %d (0x%02x)\n", (int)mantissa, (int)mantissa);

    // Check special values
    if (BF16::is_zero(input) || BF16::is_denormal(input)) {
        LOG_PRINTF("\n  Special case: Zero or Denormal -> Return 1.0\n");
        LOG_PRINTF("  Result: 0x%04x (%g)\n", ONE_BF16, bf16_to_float(ONE_BF16));
        return ONE_BF16;
    }
    if (BF16::is_nan(input)) {
        uint16_t result = BF16::make_nan();
        LOG_PRINTF("\n  Special case: NaN -> Return NaN\n");
        LOG_PRINTF("  Result: 0x%04x\n", result);
        return result;
    }
    if (BF16::is_inf(input)) {
        uint16_t result = BF16::make_inf(sign);
        LOG_PRINTF("\n  Special case: Infinity -> Return %s\n", sign ? "0" : "+Inf");
        LOG_PRINTF("  Result: 0x%04x\n", result);
        return result;
    }

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 1: Mantissa with implicit 1\n");
    uint16_t mant_with_1 = (1 << 7) | mantissa;
    LOG_PRINTF("  mant_with_1 = (1 << 7) | mantissa\n");
    LOG_PRINTF("              = %d (0x%x)\n", (int)mant_with_1, mant_with_1);

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 2: Multiply by LN2_FLIP\n");
    LOG_PRINTF("  LN2_FLIP = %d (1/ln2 * 2^14)\n", LN2_FLIP);
    uint32_t mant_mul_ln2flip = ((uint32_t)mant_with_1 * LN2_FLIP) & 0x7FFFFF;
    LOG_PRINTF("  mant_mul_ln2flip = (mant_with_1 * LN2_FLIP) & 0x7FFFFF\n");
    LOG_PRINTF("                   = %u (0x%x)\n", mant_mul_ln2flip, mant_mul_ln2flip);

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 3: Shift by exponent (shm calculation)\n");
    LOG_PRINTF("  BIAS = %d\n", (int)BIAS);
    LOG_PRINTF("  exponent - BIAS = %d - %d = %d\n", (int)exponent, (int)BIAS, (int)exponent - (int)BIAS);

    uint32_t shm;
    if (exponent >= BIAS) {
        uint8_t shift_amount = exponent - BIAS;
        LOG_PRINTF("  Branch: exponent >= BIAS (left shift)\n");
        LOG_PRINTF("  shift_amount = %d\n", (int)shift_amount);
        shm = (shift_amount < 30) ? ((mant_mul_ln2flip << shift_amount) & 0x3FFFFFFF) : 0;
        LOG_PRINTF("  shm = (mant_mul_ln2flip << %d) & 0x3FFFFFFF\n", (int)shift_amount);
    } else {
        uint8_t shift_amount = BIAS - exponent;
        LOG_PRINTF("  Branch: exponent < BIAS (right shift)\n");
        LOG_PRINTF("  shift_amount = %d\n", (int)shift_amount);
        shm = (shift_amount > 30) ? 0 : ((mant_mul_ln2flip >> shift_amount) & 0x3FFFFFFF);
        LOG_PRINTF("  shm = (mant_mul_ln2flip >> %d) & 0x3FFFFFFF\n", (int)shift_amount);
    }
    LOG_PRINTF("  shm = %u (0x%x)\n", shm, shm);

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 4: Round to remove fraction\n");
    bool round_bit = (shm >> 13) & 1;
    LOG_PRINTF("  round_bit = (shm >> 13) & 1 = %d\n", round_bit);
    uint16_t shm_nofraction = round_bit ? ((shm >> 14) + 1) & 0xFFFF : (shm >> 14) & 0xFFFF;
    LOG_PRINTF("  shm_nofraction = %d (0x%x)\n", shm_nofraction, shm_nofraction);

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 5: Apply sign (two's complement if negative)\n");
    uint16_t shm_unsigned = shm_nofraction;
    if (sign) {
        shm_unsigned = ((~shm_unsigned + 1) & 0xFFFF);
        LOG_PRINTF("  Input is negative, applying two's complement:\n");
        LOG_PRINTF("  shm_unsigned = (~shm_nofraction + 1) & 0xFFFF\n");
    } else {
        LOG_PRINTF("  Input is positive, no change:\n");
        LOG_PRINTF("  shm_unsigned = shm_nofraction\n");
    }
    LOG_PRINTF("  shm_unsigned = %d (0x%x)\n", shm_unsigned, shm_unsigned);

    int64_t shm_signed = sign_extend(shm_unsigned, 16);
    LOG_PRINTF("  shm_signed (16-bit sign extended) = %lld (0x%llx)\n", (long long)shm_signed, (unsigned long long)shm_signed);

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 6: Extract nm (mantissa) and ne (exponent)\n");
    uint8_t nm = shm_signed & 0x7F;
    LOG_PRINTF("  nm = shm_signed & 0x7F = %d (0x%02x)\n", (int)nm, (int)nm);

    int16_t ne_temp = sign_extend((sign_extend((shm_signed >> 7), 9) + BIAS) & 0x1FF, 9);
    LOG_PRINTF("  ne_temp = sign_extend((sign_extend((shm_signed >> 7), 9) + BIAS) & 0x1FF, 9)\n");
    LOG_PRINTF("          = sign_extend((sign_extend(%lld, 9) + %d) & 0x1FF, 9)\n", (long long)(shm_signed >> 7), (int)BIAS);
    LOG_PRINTF("          = %d\n", ne_temp);

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 7: Check for overflow/underflow\n");
    if (ne_temp >= 255) {
        LOG_PRINTF("  Overflow: ne_temp >= 255\n");
        uint16_t result = sign ? BF16::make_zero(false) : BF16::make_inf(false);
        LOG_PRINTF("  Result: %s (0x%04x)\n", sign ? "0" : "+Inf", result);
        return result;
    }
    if (ne_temp <= 0) {
        LOG_PRINTF("  Underflow: ne_temp <= 0\n");
        uint16_t result = BF16::make_zero(false);
        LOG_PRINTF("  Result: 0 (0x%04x)\n", result);
        return result;
    }
    LOG_PRINTF("  No overflow/underflow, ne_temp in valid range [1, 254]\n");

    uint8_t ne = ne_temp & 0xFF;
    LOG_PRINTF("  ne = ne_temp & 0xFF = %d (0x%02x)\n", (int)ne, (int)ne);

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 8: Polynomial approximation\n");
    uint8_t frac_msb = (nm >> 6) & 0x1;
    LOG_PRINTF("  frac_msb = (nm >> 6) & 0x1 = %d (selects %s half)\n", (int)frac_msb, frac_msb ? "upper" : "lower");

    LOG_PRINTF("\n  Substep 8a: res_add_1\n");
    uint16_t gamma_selected = frac_msb ? GAMMA2 : GAMMA1;
    LOG_PRINTF("  gamma_selected = %s = %d\n", frac_msb ? "GAMMA2" : "GAMMA1", gamma_selected);
    uint16_t res_add_1 = (nm + gamma_selected) & 0x1FF;
    LOG_PRINTF("  res_add_1 = (nm + gamma_selected) & 0x1FF\n");
    LOG_PRINTF("            = (%d + %d) & 0x1FF\n", (int)nm, gamma_selected);
    LOG_PRINTF("            = %d (0x%x)\n", res_add_1, res_add_1);

    LOG_PRINTF("\n  Substep 8b: res_mul_1\n");
    uint8_t mant_mul = (nm << 1) & 0xFF;
    LOG_PRINTF("  mant_mul = (nm << 1) & 0xFF = %d (0x%02x)\n", (int)mant_mul, (int)mant_mul);

    uint16_t res_mul_1;
    if (frac_msb) {
        res_mul_1 = ((0xFF - mant_mul) * BETA) & 0x7FF;
        LOG_PRINTF("  res_mul_1 = ((0xFF - mant_mul) * BETA) & 0x7FF\n");
        LOG_PRINTF("            = ((0xFF - %d) * %d) & 0x7FF\n", (int)mant_mul, (int)BETA);
    } else {
        res_mul_1 = (mant_mul * ALPHA) & 0x7FF;
        LOG_PRINTF("  res_mul_1 = (mant_mul * ALPHA) & 0x7FF\n");
        LOG_PRINTF("            = (%d * %d) & 0x7FF\n", (int)mant_mul, (int)ALPHA);
    }
    LOG_PRINTF("            = %d (0x%x)\n", res_mul_1, res_mul_1);

    LOG_PRINTF("\n  Substep 8c: res_mul_2\n");
    uint8_t res_mul_2 = ((uint32_t)res_add_1 * res_mul_1 >> 12) & 0xFF;
    LOG_PRINTF("  res_mul_2 = (res_add_1 * res_mul_1 >> 12) & 0xFF\n");
    LOG_PRINTF("            = (%d * %d >> 12) & 0xFF\n", res_add_1, res_mul_1);
    LOG_PRINTF("            = %d (0x%02x)\n", (int)res_mul_2, (int)res_mul_2);

    LOG_PRINTF("\n  Substep 8d: result_mant\n");
    uint8_t result_mant;
    if (frac_msb) {
        result_mant = (0x7F - res_mul_2) & 0x7F;
        LOG_PRINTF("  result_mant = (0x7F - res_mul_2) & 0x7F\n");
        LOG_PRINTF("              = (0x7F - %d) & 0x7F\n", (int)res_mul_2);
    } else {
        result_mant = res_mul_2 & 0x7F;
        LOG_PRINTF("  result_mant = res_mul_2 & 0x7F\n");
        LOG_PRINTF("              = %d & 0x7F\n", (int)res_mul_2);
    }
    LOG_PRINTF("              = %d (0x%02x)\n", (int)result_mant, (int)result_mant);

    LOG_PRINTF("\n----------------------------------------\n");
    LOG_PRINTF("Step 9: Build final result\n");
    BFloat16 result;
    result.fields.sign = 0;
    result.fields.exponent = ne & 0xFF;
    result.fields.mantissa = result_mant;

    LOG_PRINTF("  result.sign     = 0\n");
    LOG_PRINTF("  result.exponent = %d (0x%02x)\n", (int)result.fields.exponent, (int)result.fields.exponent);
    LOG_PRINTF("  result.mantissa = %d (0x%02x)\n", (int)result.fields.mantissa, (int)result.fields.mantissa);
    LOG_PRINTF("  result.bits     = 0x%04x\n", result.bits);
    LOG_PRINTF("  result (float)  = %g\n", bf16_to_float(result.bits));

    return result.bits;
}

int main(int argc, char* argv[]) {
    uint16_t test_input = 0x8000;  // Default: -0.0

    // Parse command line argument if provided
    if (argc > 1) {
        // Support both hex (0x8000) and decimal input
        if (strncmp(argv[1], "0x", 2) == 0 || strncmp(argv[1], "0X", 2) == 0) {
            test_input = (uint16_t)strtoul(argv[1], nullptr, 16);
        } else {
            test_input = (uint16_t)strtoul(argv[1], nullptr, 0);
        }
    }

    // Initialize log
    log_init("log/single_expp.log");

    LOG_PRINTF("========================================\n");
    LOG_PRINTF("Single BF16 Expp Test\n");
    LOG_PRINTF("========================================\n");
    LOG_PRINTF("Testing input: 0x%04x\n", test_input);

    // Process with debug output
    uint16_t result = process_with_debug(test_input);

    // Compare with expected result
    float input_float = bf16_to_float(test_input);
    float expected_float = std::exp(input_float);
    float result_float = bf16_to_float(result);

    LOG_PRINTF("\n========================================\n");
    LOG_PRINTF("Final Comparison\n");
    LOG_PRINTF("========================================\n");
    LOG_PRINTF("  Input (BF16):     0x%04x\n", test_input);
    LOG_PRINTF("  Input (Float):    %g\n", input_float);
    LOG_PRINTF("  Result (BF16):    0x%04x\n", result);
    LOG_PRINTF("  Result (Float):   %g\n", result_float);
    LOG_PRINTF("  Expected (F32):   %g\n", expected_float);
    LOG_PRINTF("  Difference:       %g\n", (result_float - expected_float));

    if (!std::isnan(expected_float) && !std::isinf(expected_float) && expected_float != 0.0f) {
        double rel_error = std::abs((result_float - expected_float) / expected_float) * 100.0;
        LOG_PRINTF("  Relative Error:   %.6f%%\n", rel_error);
    }
    LOG_PRINTF("========================================\n");

    return 0;
}
