// +FHDR========================================================================
//  File Name:      BF16.cpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BFloat16 Arithmetic Operations Implementation
// -FHDR========================================================================
#include "BF16.hpp"
#include "custom_expp.hpp"
#include <cmath>
#include <cstring>
#include <vector>

namespace BF16 {

// LUT storage
static std::vector<uint16_t> recip_lut(128);
static std::vector<uint16_t> exp_lut(128);
static bool luts_initialized = false;

float bf16_to_float(uint16_t bf16) {
    uint32_t fp32 = ((uint32_t)bf16) << 16;
    float result;
    std::memcpy(&result, &fp32, sizeof(float));
    return result;
}
uint16_t float_to_bf16(float f) {
    if (std::isnan(f)) return make_nan();
    if (std::isinf(f)) return make_inf(f < 0.0f);
    if (f == 0.0f) return make_zero(std::signbit(f));

    uint32_t fp32;
    std::memcpy(&fp32, &f, sizeof(float));
    return (uint16_t)(fp32 >> 16);
}


void init_luts() {
    if (luts_initialized) return;

    // Reciprocal LUT: 1/(1 + i/128) for i in [0, 127]
    for (int i = 0; i < 128; i++) {
        float val = 1.0f / (1.0f + i / 128.0f);
        recip_lut[i] = float_to_bf16(val);
    }

    // Exp LUT: 2^(i/128) for i in [0, 127]
    for (int i = 0; i < 128; i++) {
        float val = std::pow(2.0f, i / 128.0f);
        exp_lut[i] = float_to_bf16(val);
    }

    luts_initialized = true;
}


uint16_t bf16_add(uint16_t a, uint16_t b, bool sub) {
    BFloat16 bf_a(a), bf_b(b);

    uint8_t s_a = bf_a.fields.sign;
    uint8_t e_a = bf_a.fields.exponent;
    uint8_t m_a = bf_a.fields.mantissa;
    uint8_t s_b = bf_b.fields.sign ^ (sub ? 1 : 0);
    uint8_t e_b = bf_b.fields.exponent;
    uint8_t m_b = bf_b.fields.mantissa;

    // Special cases: NaN
    bool is_a_nan = (e_a == 255 && m_a != 0);
    bool is_b_nan = (e_b == 255 && m_b != 0);
    if (is_a_nan || is_b_nan) return make_nan();

    // Special cases: Infinity
    bool is_a_inf = (e_a == 255 && m_a == 0);
    bool is_b_inf = (e_b == 255 && m_b == 0);
    if (is_a_inf && is_b_inf && s_a != s_b) return make_nan();
    if (is_a_inf) return make_inf(s_a);
    if (is_b_inf) return make_inf(s_b);

    // Special cases: Zero
    bool is_a_zero = (e_a == 0 && m_a == 0);
    bool is_b_zero = (e_b == 0 && m_b == 0);
    if (is_a_zero && is_b_zero) {
        return make_zero(s_a != s_b ? 0 : s_a);
    }
    if (is_a_zero) {
        // Need to use the modified s_b (which accounts for sub flag)
        BFloat16 bf_result;
        bf_result.fields.sign = s_b;
        bf_result.fields.exponent = e_b;
        bf_result.fields.mantissa = m_b;
        return bf_result.bits;
    }
    if (is_b_zero) return a;

    // Normal case: align significands
    uint16_t sig_a = (1 << 7) | m_a;
    uint16_t sig_b = (1 << 7) | m_b;

    // Swap to ensure |a| >= |b|
    if (e_a < e_b || (e_a == e_b && sig_a < sig_b)) {
        std::swap(s_a, s_b);
        std::swap(e_a, e_b);
        std::swap(sig_a, sig_b);
    }

    // Align significands with guard bits
    constexpr int GUARD_BITS = 16;
    uint32_t shift = e_a - e_b;
    uint32_t sig_a_ext = sig_a << GUARD_BITS;
    uint32_t sig_b_ext = (sig_b << GUARD_BITS) >> shift;

    // Perform addition/subtraction
    bool eff_sub = (s_a != s_b);
    uint32_t sum_sig = eff_sub ? (sig_a_ext - sig_b_ext) : (sig_a_ext + sig_b_ext);

    if (sum_sig == 0) return make_zero(0);

    uint8_t res_sign = s_a;
    int res_exp = e_a;

    // Normalize
    int msb_pos = 31 - __builtin_clz(sum_sig);
    constexpr int TARGET_MSB_POS = 7 + GUARD_BITS;
    int shift_amt = msb_pos - TARGET_MSB_POS;
    res_exp += shift_amt;

    uint32_t final_sig = (shift_amt < 0) ? (sum_sig << -shift_amt) : (sum_sig >> shift_amt);
    uint8_t final_mant = (final_sig >> GUARD_BITS) & 0x7F;

    // Handle overflow/underflow
    if (res_exp >= 255) return make_inf(res_sign);
    if (res_exp <= 0) return make_zero(res_sign);

    BFloat16 result;
    result.fields.sign = res_sign;
    result.fields.exponent = res_exp;
    result.fields.mantissa = final_mant;
    return result.bits;
}

uint16_t bf16_max(uint16_t a, uint16_t b) {
    BFloat16 bf_a(a), bf_b(b);

    uint8_t s_a = bf_a.fields.sign;
    uint8_t e_a = bf_a.fields.exponent;
    uint8_t m_a = bf_a.fields.mantissa;
    uint8_t s_b = bf_b.fields.sign;
    uint8_t e_b = bf_b.fields.exponent;
    uint8_t m_b = bf_b.fields.mantissa;

    // Handle NaN
    bool is_a_nan = (e_a == 255 && m_a != 0);
    bool is_b_nan = (e_b == 255 && m_b != 0);
    if (is_a_nan && is_b_nan) return make_nan();
    if (is_a_nan) return b;
    if (is_b_nan) return a;

    // Different signs: return positive one
    if (s_a != s_b) return s_a ? b : a;

    // Same sign: compare magnitudes
    uint16_t abs_a = (e_a << 7) | m_a;
    uint16_t abs_b = (e_b << 7) | m_b;
    bool a_abs_gt_b = (abs_a > abs_b);

    if (s_a) {  // Both negative
        return a_abs_gt_b ? b : a;
    } else {    // Both positive
        return a_abs_gt_b ? a : b;
    }
}

uint16_t bf16_mul(uint16_t a, uint16_t b) {
    BFloat16 bf_a(a), bf_b(b);

    uint8_t s_a = bf_a.fields.sign;
    uint8_t e_a = bf_a.fields.exponent;
    uint8_t m_a = bf_a.fields.mantissa;
    uint8_t s_b = bf_b.fields.sign;
    uint8_t e_b = bf_b.fields.exponent;
    uint8_t m_b = bf_b.fields.mantissa;

    uint8_t res_sign = s_a ^ s_b;

    // Special cases
    bool is_a_denormal = (e_a == 0 && m_a != 0);
    bool is_b_denormal = (e_b == 0 && m_b != 0);
    bool is_a_nan = (e_a == 255 && m_a != 0);
    bool is_b_nan = (e_b == 255 && m_b != 0);
    bool is_a_inf = (e_a == 255 && m_a == 0);
    bool is_b_inf = (e_b == 255 && m_b == 0);
    bool is_a_zero = (e_a == 0 && m_a == 0) || is_a_denormal;
    bool is_b_zero = (e_b == 0 && m_b == 0) || is_b_denormal;

    if (is_a_nan || is_b_nan) return make_nan();
    if ((is_a_zero && is_b_inf) || (is_a_inf && is_b_zero)) return make_nan();
    if (is_a_inf || is_b_inf) return make_inf(res_sign);
    if (is_a_zero || is_b_zero) return make_zero(res_sign);

    // Normal multiplication
    int exp_sum = e_a + e_b - BIAS;
    uint16_t sig_a = (1 << 7) | m_a;
    uint16_t sig_b = (1 << 7) | m_b;
    uint16_t mant_prod = sig_a * sig_b;

    // Normalize
    int norm_exp;
    uint8_t norm_mant;
    if ((mant_prod >> 15) & 1) {
        norm_exp = exp_sum + 1;
        norm_mant = (mant_prod >> 8) & 0x7F;
    } else {
        norm_exp = exp_sum;
        norm_mant = (mant_prod >> 7) & 0x7F;
    }

    // Handle overflow/underflow
    if (norm_exp >= 255) return make_inf(res_sign);
    if (norm_exp <= 0) return make_zero(res_sign);

    BFloat16 result;
    result.fields.sign = res_sign;
    result.fields.exponent = norm_exp;
    result.fields.mantissa = norm_mant;
    return result.bits;
}

uint16_t bf16_reciprocal(uint16_t x) {
    if (!luts_initialized) init_luts();

    BFloat16 bf_x(x);
    uint8_t s_x = bf_x.fields.sign;
    uint8_t e_x = bf_x.fields.exponent;
    uint8_t m_x = bf_x.fields.mantissa;

    // Special cases
    bool is_zero = (e_x == 0 && m_x == 0);
    bool is_denormal = (e_x == 0 && m_x != 0);
    bool is_inf = (e_x == 255 && m_x == 0);
    bool is_nan = (e_x == 255 && m_x != 0);

    if (is_zero || is_denormal) return make_inf(s_x);
    if (is_inf) return make_zero(s_x);
    if (is_nan) return make_nan();

    // Look up reciprocal
    uint16_t lut_val = recip_lut[m_x];
    BFloat16 bf_lut(lut_val);
    uint8_t e_lut = bf_lut.fields.exponent;
    uint8_t m_lut = bf_lut.fields.mantissa;

    int final_exp = e_lut + BIAS - e_x;

    if (final_exp >= 255) return make_inf(s_x);
    if (final_exp <= 0) return make_zero(s_x);

    BFloat16 result;
    result.fields.sign = s_x;
    result.fields.exponent = final_exp;
    result.fields.mantissa = m_lut;
    return result.bits;
}

uint16_t bf16_exp(uint16_t x) {
    if (!luts_initialized) init_luts();

    BFloat16 bf_x(x);
    uint8_t s_x = bf_x.fields.sign;
    uint8_t e_x = bf_x.fields.exponent;
    uint8_t m_x = bf_x.fields.mantissa;

    // Special cases
    bool is_zero = (e_x == 0 && m_x == 0);
    bool is_denormal = (e_x == 0 && m_x != 0);
    bool is_inf = (e_x == 255 && m_x == 0);
    bool is_nan = (e_x == 255 && m_x != 0);

    if (is_zero || is_denormal) return ONE;
    if (is_inf && s_x == 0) return make_inf(0);
    if (is_inf && s_x == 1) return make_zero(0);
    if (is_nan) return make_nan();

    // exp(x) = 2^(x * log2(e))
    uint16_t y = bf16_mul(x, LOG2E);
    BFloat16 bf_y(y);
    uint8_t s_y = bf_y.fields.sign;
    uint8_t e_y = bf_y.fields.exponent;
    uint8_t m_y = bf_y.fields.mantissa;

    // Compute floor(y)
    int e_y_unbiased = e_y - BIAS;
    uint16_t y_trunc = y;

    if (e_y_unbiased < 0) {
        y_trunc = make_zero(s_y);
    } else if (e_y_unbiased < 7) {
        uint8_t mask = ~((1 << (7 - e_y_unbiased)) - 1) & 0x7F;
        BFloat16 bf_trunc;
        bf_trunc.fields.sign = s_y;
        bf_trunc.fields.exponent = e_y;
        bf_trunc.fields.mantissa = m_y & mask;
        y_trunc = bf_trunc.bits;
    }

    uint16_t y_floor = y_trunc;
    if (s_y == 1 && y != y_trunc) {
        y_floor = bf16_add(y_trunc, ONE, true);  // y_trunc - 1
    }

    // F = y - floor(y), I = floor(y)
    uint16_t F = bf16_add(y, y_floor, true);
    int I = 0;

    BFloat16 bf_yf(y_floor);
    uint8_t s_yf = bf_yf.fields.sign;
    uint8_t e_yf = bf_yf.fields.exponent;
    uint8_t m_yf = bf_yf.fields.mantissa;

    if (e_yf != 0) {
        int e_yf_unbiased = e_yf - BIAS;
        uint16_t sig_yf = (1 << 7) | m_yf;
        if (e_yf_unbiased >= 7) {
            I = sig_yf << (e_yf_unbiased - 7);
        } else {
            I = sig_yf >> (7 - e_yf_unbiased);
        }
        if (s_yf) I = -I;
    }

    // Compute 2^F using LUT
    BFloat16 bf_f(F);
    uint8_t s_f = bf_f.fields.sign;
    uint8_t e_f = bf_f.fields.exponent;
    uint8_t m_f = bf_f.fields.mantissa;

    int e_f_unbiased = e_f - BIAS;
    uint16_t pow2_F = ONE;  // Default to 1.0

    if (s_f == 0 && !(e_f == 0 && m_f == 0)) {  // F > 0
        if (e_f_unbiased == -1) {
            // F in [0.5, 1.0)
            int lut_index = 64 + (m_f >> 1);
            pow2_F = exp_lut[lut_index];
        } else if (e_f_unbiased < -1) {
            // F in (0, 0.5)
            int shift = BIAS - e_f;
            uint16_t sig_f = (1 << 7) | m_f;
            int lut_index = (sig_f >> shift) & 0x7F;
            pow2_F = exp_lut[lut_index];
        } else {
            // F >= 1.0 (rare in exp logic)
            pow2_F = TWO;
        }
    }

    // Combine: result = 2^I * 2^F
    BFloat16 bf_p2f(pow2_F);
    uint8_t e_p2f = bf_p2f.fields.exponent;
    uint8_t m_p2f = bf_p2f.fields.mantissa;

    int final_exp = I + (e_p2f - BIAS) + BIAS;

    if (final_exp >= 255) return make_inf(0);
    if (final_exp <= 0) return make_zero(0);

    BFloat16 result;
    result.fields.sign = 0;
    result.fields.exponent = final_exp;
    result.fields.mantissa = m_p2f;
    return result.bits;
}

uint16_t bf16_expp(uint16_t x) {
    static BF16ExppUnit_Model expp_unit;
    return expp_unit.process(x);
}

}  // namespace BF16
