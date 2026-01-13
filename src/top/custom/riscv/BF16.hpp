// +FHDR========================================================================
//  File Name:      BF16.hpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BFloat16 Common Utilities
// -FHDR========================================================================
#ifndef BF16_HPP
#define BF16_HPP

#include <cstdint>

// BFloat16 union: 1 sign + 8 exponent + 7 mantissa
union BFloat16 {
    uint16_t bits;
    struct {
        uint16_t mantissa : 7;
        uint16_t exponent : 8;
        uint16_t sign : 1;
    } fields;

    BFloat16() : bits(0) {}
    BFloat16(uint16_t val) : bits(val) {}
};

// BFloat16 Helper Functions
namespace BF16 {
    inline bool is_zero(uint16_t val) {
        BFloat16 bf(val);
        return (bf.fields.exponent == 0) && (bf.fields.mantissa == 0);
    }

    inline bool is_denormal(uint16_t val) {
        BFloat16 bf(val);
        return (bf.fields.exponent == 0) && (bf.fields.mantissa != 0);
    }

    inline bool is_inf(uint16_t val) {
        BFloat16 bf(val);
        return (bf.fields.exponent == 255) && (bf.fields.mantissa == 0);
    }

    inline bool is_nan(uint16_t val) {
        BFloat16 bf(val);
        return (bf.fields.exponent == 255) && (bf.fields.mantissa != 0);
    }

    inline uint16_t make_nan() {
        BFloat16 result;
        result.fields.sign = 0;
        result.fields.exponent = 255;
        result.fields.mantissa = 0x40;
        return result.bits;
    }

    inline uint16_t make_inf(bool sign) {
        BFloat16 result;
        result.fields.sign = sign ? 1 : 0;
        result.fields.exponent = 255;
        result.fields.mantissa = 0;
        return result.bits;
    }

    inline uint16_t make_zero(bool sign) {
        BFloat16 result;
        result.fields.sign = sign ? 1 : 0;
        result.fields.exponent = 0;
        result.fields.mantissa = 0;
        return result.bits;
    }

    constexpr uint16_t ONE = 0x3F80;         // 1.0
    constexpr uint16_t TWO = 0x4000;         // 2.0
    constexpr uint16_t LOG2E = 0x3FB9;       // log2(e)
    constexpr uint16_t NEG_INF = 0xFF80;     // -Infinity
    constexpr int BIAS = 127;

    uint16_t bf16_add(uint16_t a, uint16_t b, bool sub = false);
    uint16_t bf16_max(uint16_t a, uint16_t b);
    uint16_t bf16_mul(uint16_t a, uint16_t b);
    uint16_t bf16_reciprocal(uint16_t x);
    uint16_t bf16_exp(uint16_t x);
    uint16_t bf16_expp(uint16_t x);

    float bf16_to_float(uint16_t bf16);
    uint16_t float_to_bf16(float f);

    void init_luts();  // Initialize LUTs for reciprocal and exp
}

#endif // BF16_HPP
