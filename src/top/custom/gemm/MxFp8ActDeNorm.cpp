// +FHDR========================================================================
//  File Name:      MxFp8ActDeNorm.cpp
//  Description:    MxFP8 Activation Denormalization Implementation
// -FHDR========================================================================
#include "MxFp8ActDeNorm.hpp"
#include "../util.h"

void MxFp8ActDeNormModel::compute(const uint8_t i_MxFp8Act[ROW_SIZE]) {
    uint8_t exp_max = 0;
    for (int i = 0; i < ROW_SIZE; ++i) {
        uint8_t exp = (i_MxFp8Act[i] >> 3) & 0xF;
        if (exp > exp_max) {
            exp_max = exp;
        }
    }
    o_Fp8ActExp = exp_max;

    for (int i = 0; i < ROW_SIZE; ++i) {
        uint8_t fp8 = i_MxFp8Act[i];
        uint8_t sign = (fp8 >> 7) & 0x1;
        uint8_t exp = (fp8 >> 3) & 0xF;
        uint8_t mat_raw = fp8 & 0x7;

        uint8_t mat = (exp != 0) ? ((1 << 3) | mat_raw) : (mat_raw << 1);
        uint8_t shift_amt = exp_max - exp;
        uint32_t mat_shifted = ((uint32_t)mat << 15) >> shift_amt;

        uint32_t mat_mul1 = mat_shifted;
        uint32_t mat_mul2 = mat_shifted * 2;
        uint32_t mat_mul3 = mat_shifted * 3;

        uint32_t mat_pos1 = mat_mul1 & 0x3FFFFF;
        uint32_t mat_pos2 = mat_mul2 & 0x3FFFFF;
        uint32_t mat_pos3 = mat_mul3 & 0x3FFFFF;

        uint32_t mat_neg1 = (~mat_mul1 + 1) & 0x3FFFFF;
        uint32_t mat_neg2 = (~mat_mul2 + 1) & 0x3FFFFF;
        uint32_t mat_neg3 = (~mat_mul3 + 1) & 0x3FFFFF;

        o_Fp8ActSign[i] = sign;
        o_Fp8ActMatPos1[i] = sign_extend(mat_pos1, 22);
        o_Fp8ActMatNeg1[i] = sign_extend(mat_neg1, 22);
        o_Fp8ActMatPos2[i] = sign_extend(mat_pos2, 22);
        o_Fp8ActMatNeg2[i] = sign_extend(mat_neg2, 22);
        o_Fp8ActMatPos3[i] = sign_extend(mat_pos3, 22);
        o_Fp8ActMatNeg3[i] = sign_extend(mat_neg3, 22);
    }
}
