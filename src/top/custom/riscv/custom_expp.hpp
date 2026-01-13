// +FHDR========================================================================
//  File Name:      custom_expp.hpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BFloat16 e^x Approximation Model
// -FHDR========================================================================
#ifndef CUSTOM_EXPP_HPP
#define CUSTOM_EXPP_HPP

#include <cstdint>
#include "BF16.hpp"
#include "../util.h"

class BF16ExppUnit_Model {
public:
    BF16ExppUnit_Model() = default;
    ~BF16ExppUnit_Model() = default;

    uint16_t process(uint16_t input);

private:
    static constexpr uint8_t BIAS = 127;
    static constexpr uint16_t ONE_BF16 = 0x3F80;
    static constexpr uint16_t LN2_FLIP = 23637;  // (1/ln2) * 2^14

    //approximation coefficients
    static constexpr uint8_t  ALPHA = 4;     // 0.25
    static constexpr uint8_t  BETA = 7;      // 0.4375
    static constexpr uint16_t GAMMA1 = 363;  // 2.836
    static constexpr uint16_t GAMMA2 = 278;  // 2.168

    static constexpr uint16_t too_small_L = 0xc386;
    static constexpr uint16_t too_small_H = 0xff7f;
    static constexpr uint16_t too_large_L = 0x42b2;
    static constexpr uint16_t too_large_H = 0x7f7f;
};

#endif // CUSTOM_EXPP_HPP
