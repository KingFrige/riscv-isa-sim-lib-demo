// +FHDR========================================================================
//  File Name:      custom_expp.cpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BFloat16 e^x Approximation Implementation
// -FHDR========================================================================
#include "custom_expp.hpp"
#include <cstdint>

uint16_t BF16ExppUnit_Model::process(uint16_t input) {
    BFloat16 in(input);
    bool sign = in.fields.sign;
    uint8_t exponent = in.fields.exponent;
    uint8_t mantissa = in.fields.mantissa;

    // Handle special values
    if (BF16::is_zero(input) || BF16::is_denormal(input)) {
        return ONE_BF16;
    }
    if (BF16::is_nan(input)) {
        return BF16::make_nan();
    }
    if (BF16::is_inf(input)) {
        return sign? BF16::make_zero(false) : BF16::make_inf(false);
    }

    if (input >= too_large_L && input <= too_large_H) {
      return BF16::make_inf(false);
    } else if (input >= too_small_L && input <= too_small_H) {
      return BF16::make_zero(false);
    }

    uint16_t mant_with_1 = (1 << 7) | mantissa;
    // Multiply by ln2_flip: (1/ln2) * 2^14
    uint32_t mant_mul_ln2flip = ((uint32_t)mant_with_1 * LN2_FLIP) & 0x7FFFFF;//23bit

    uint32_t shm;
    if (exponent >= BIAS) {
        uint8_t shift_amount = exponent - BIAS;
        shm = (shift_amount < 30) ? ((mant_mul_ln2flip << shift_amount) & 0x3FFFFFFF) : 0;
    } else {
        uint8_t shift_amount = BIAS - exponent;
        shm = (shift_amount < 23) ? ((mant_mul_ln2flip >> shift_amount) & 0x3FFFFFFF) : 0;
    }


    uint16_t shm_nofraction = ((shm >> 13) & 1) ? ((shm >> 14) + 1) & 0xFFFF : (shm >> 14) & 0xFFFF;//16bit


    uint16_t shm_unsigned = shm_nofraction;
    if (sign) {
        shm_unsigned = ((~shm_unsigned + 1) & 0xFFFF);
    }

    int64_t shm_signed = sign_extend(shm_unsigned,16);

    uint8_t nm = shm_signed & 0x7F;
    // int32_t ne_temp = (shm_signed >> 7) + BIAS;
    int16_t ne_temp = sign_extend( (sign_extend((shm_signed >> 7),9) + BIAS) & 0x1FF,9);

    // Overflow: e^(+big) = +inf, e^(-big) = 0
    if (ne_temp >= 255) {
        return sign ? BF16::make_zero(false) : BF16::make_inf(false);
    }
    if (ne_temp <= 0) {
        return BF16::make_zero(false);
    }

    // Now safe to cast to uint8_t
    uint8_t ne = ne_temp & 0xFF;

    // Polynomial approximation
    uint8_t frac_msb = (nm >> 6) & 0x1;
    uint16_t res_add_1 = (nm + (frac_msb ? GAMMA2 : GAMMA1)) & 0x1FF;
    uint8_t mant_mul = (nm << 1) & 0xFF;
    uint16_t res_mul_1 = (frac_msb ? ((0xFF - mant_mul) * BETA) & 0x7FF : (mant_mul * ALPHA)) & 0x7FF;
    uint8_t res_mul_2 = ((uint32_t)res_add_1 * res_mul_1 >> 12) & 0xFF;
    uint8_t result_mant = (frac_msb ? (0x7F - res_mul_2) : res_mul_2) & 0x7F;

    // Build result
    BFloat16 result;
    result.fields.sign = 0;
    result.fields.exponent = ne & 0xFF;
    result.fields.mantissa = result_mant;

    return result.bits;
}
