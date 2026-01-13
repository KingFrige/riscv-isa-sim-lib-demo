// +FHDR========================================================================
//  File Name:      MxFp8ActDeNorm.hpp
//  Description:    MxFP8 Activation Denormalization Model
// -FHDR========================================================================
#ifndef MXFP8_ACT_DENORM_HPP
#define MXFP8_ACT_DENORM_HPP

#include <cstdint>
#include <cstring>
#include "config.h"

struct MxFp8ActDeNormModel {
    uint8_t o_Fp8ActSign[ROW_SIZE];
    uint8_t o_Fp8ActExp;
    int64_t o_Fp8ActMatPos1[ROW_SIZE];
    int64_t o_Fp8ActMatNeg1[ROW_SIZE];
    int64_t o_Fp8ActMatPos2[ROW_SIZE];
    int64_t o_Fp8ActMatNeg2[ROW_SIZE];
    int64_t o_Fp8ActMatPos3[ROW_SIZE];
    int64_t o_Fp8ActMatNeg3[ROW_SIZE];

    MxFp8ActDeNormModel() : o_Fp8ActExp(0) {
        std::memset(o_Fp8ActSign, 0, sizeof(o_Fp8ActSign));
        std::memset(o_Fp8ActMatPos1, 0, sizeof(o_Fp8ActMatPos1));
        std::memset(o_Fp8ActMatNeg1, 0, sizeof(o_Fp8ActMatNeg1));
        std::memset(o_Fp8ActMatPos2, 0, sizeof(o_Fp8ActMatPos2));
        std::memset(o_Fp8ActMatNeg2, 0, sizeof(o_Fp8ActMatNeg2));
        std::memset(o_Fp8ActMatPos3, 0, sizeof(o_Fp8ActMatPos3));
        std::memset(o_Fp8ActMatNeg3, 0, sizeof(o_Fp8ActMatNeg3));
    }
    ~MxFp8ActDeNormModel() = default;

    void compute(const uint8_t i_MxFp8Act[ROW_SIZE]);
};

#endif // MXFP8_ACT_DENORM_HPP
