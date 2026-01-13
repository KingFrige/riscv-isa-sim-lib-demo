// +FHDR========================================================================
//  File Name:      MxFp8Product.cpp
//  Description:    MxFP8 Product Calculation Implementation
// -FHDR========================================================================
#include "MxFp8Product.hpp"
#include "../util.h"

void MxFp8ProductModel::compute(
    uint8_t i_CfgFp8Fp8,
    const uint8_t i_Fp8ActSign[ROW_SIZE],
    uint8_t i_Fp8ActExp,
    const int64_t i_Fp8ActMatPos1[ROW_SIZE],
    const int64_t i_Fp8ActMatNeg1[ROW_SIZE],
    const int64_t i_Fp8ActMatPos2[ROW_SIZE],
    const int64_t i_Fp8ActMatNeg2[ROW_SIZE],
    const int64_t i_Fp8ActMatPos3[ROW_SIZE],
    const int64_t i_Fp8ActMatNeg3[ROW_SIZE],
    const uint8_t i_MxFp8Wgt[ROW_SIZE]) {

    int64_t psum_msb[ROW_SIZE];
    int64_t psum_lsb[ROW_SIZE];

    if (i_CfgFp8Fp8) {
        uint8_t exp_max = 0;
        for (int i = 0; i < ROW_SIZE; ++i) {
            uint8_t exp_high = (i_MxFp8Wgt[i] >> 5) & 0x3;
            if (exp_high > exp_max) exp_max = exp_high;
        }

        for (int i = 0; i < ROW_SIZE; ++i) {
            uint8_t wgt = i_MxFp8Wgt[i];
            uint8_t act_sign = i_Fp8ActSign[i];

            uint8_t wgt_sign = (wgt >> 7) & 0x1;
            uint8_t wgt_exp = (wgt >> 3) & 0xF;
            uint8_t wgt_mat = wgt & 0x7;

            uint8_t exp_high = (wgt_exp >> 2) & 0x3;
            uint8_t exp_low = wgt_exp & 0x3;
            uint8_t prod_sign = wgt_sign ^ act_sign;

            uint8_t mat_msb, mat_lsb;
            if (wgt_exp == 0) {
                mat_msb = (wgt_mat >> 1) & 0x3;
                mat_lsb = (wgt_mat & 0x1) << 1;
            } else {
                mat_msb = 2 | ((wgt_mat >> 2) & 0x1);
                mat_lsb = wgt_mat & 0x3;
            }

            int64_t Fp4MatMulMsb, Fp4MatMulLsb;

            switch (mat_msb) {
                case 0: Fp4MatMulMsb = 0; break;
                case 1: Fp4MatMulMsb = prod_sign ? i_Fp8ActMatNeg1[i] : i_Fp8ActMatPos1[i]; break;
                case 2: Fp4MatMulMsb = prod_sign ? i_Fp8ActMatNeg2[i] : i_Fp8ActMatPos2[i]; break;
                case 3: Fp4MatMulMsb = prod_sign ? i_Fp8ActMatNeg3[i] : i_Fp8ActMatPos3[i]; break;
                default: Fp4MatMulMsb = 0; break;
            }

            switch (mat_lsb) {
                case 0: Fp4MatMulLsb = 0; break;
                case 1: Fp4MatMulLsb = prod_sign ? i_Fp8ActMatNeg1[i] : i_Fp8ActMatPos1[i]; break;
                case 2: Fp4MatMulLsb = prod_sign ? i_Fp8ActMatNeg2[i] : i_Fp8ActMatPos2[i]; break;
                case 3: Fp4MatMulLsb = prod_sign ? i_Fp8ActMatNeg3[i] : i_Fp8ActMatPos3[i]; break;
                default: Fp4MatMulLsb = 0; break;
            }

            int64_t Fp4MatShiftLsb, Fp4MatShiftMsb;
            Fp4MatShiftLsb = (sign_extend(Fp4MatMulLsb, 22) << exp_low) & bitmask_u64(25);
            Fp4MatShiftMsb = (sign_extend(Fp4MatMulMsb, 22) << exp_low) & bitmask_u64(25);

            int64_t Fp8MatShiftLsb, Fp8MatShiftMsb;
            uint8_t shift_bit = (exp_max - exp_high) * 4 +(25-MAT_WIDTH);
            Fp8MatShiftLsb = (sign_extend(Fp4MatShiftLsb, 25) >> shift_bit) & bitmask_u64(MAT_WIDTH);
            Fp8MatShiftMsb = (sign_extend(Fp4MatShiftMsb, 25) >> shift_bit) & bitmask_u64(MAT_WIDTH);

            psum_msb[i] = Fp8MatShiftMsb;
            psum_lsb[i] = Fp8MatShiftLsb;
        }

        int64_t sum_msb = IntAdderTree(psum_msb, MAT_WIDTH);
        int64_t sum_lsb = IntAdderTree(psum_lsb, MAT_WIDTH);

        int64_t Fp8ProductMat;
        Fp8ProductMat = sign_extend(sum_msb, MAT_WIDTH) + (sign_extend(sum_lsb, MAT_WIDTH)>>2);

        uint8_t Fp8ProductMatOvl;
        Fp8ProductMatOvl = (Fp8ProductMat>>(MAT_WIDTH)) != (Fp8ProductMat>>(MAT_WIDTH-1));
        o_ProductExp = Fp8ProductMatOvl + i_Fp8ActExp + (exp_max << 2) + 1;

        o_ProductMatMsb = 0;
        if(Fp8ProductMatOvl){
          o_ProductMatLsb = sign_extend(Fp8ProductMat >> 1, MAT_WIDTH);
        }
        else{
          o_ProductMatLsb = sign_extend(Fp8ProductMat & bitmask_u64(MAT_WIDTH), MAT_WIDTH);
        }

    } else {
        for (int i = 0; i < ROW_SIZE; ++i) {
            uint8_t wgt = i_MxFp8Wgt[i];
            uint8_t act_sign = i_Fp8ActSign[i];

            uint8_t wgt_msb_sign = (wgt >> 7) & 0x1;
            uint8_t wgt_lsb_sign = (wgt >> 3) & 0x1;
            uint8_t wgt_msb_exp = (wgt >> 5) & 0x3;
            uint8_t wgt_lsb_exp = (wgt >> 3) & 0x3;

            uint8_t mat_msb, mat_lsb;
            if(wgt_msb_exp == 0){
              mat_msb = ((wgt >> 4) & 0x1) << 1;
              mat_lsb = (wgt & 0x1) << 1;
            }
            else{
              mat_msb = 2 | ((wgt >> 4) & 0x1);
              mat_lsb = 2 | (wgt & 0x1);
            }

            uint8_t Fp4WgtSignMsb = wgt_msb_sign ^ act_sign;
            uint8_t Fp4WgtSignLsb = wgt_lsb_sign ^ act_sign;

            int64_t Fp4MatMulMsb, Fp4MatMulLsb;

            switch (mat_msb) {
                case 0: Fp4MatMulMsb = 0; break;
                case 1: Fp4MatMulMsb = Fp4WgtSignMsb ? i_Fp8ActMatNeg1[i] : i_Fp8ActMatPos1[i]; break;
                case 2: Fp4MatMulMsb = Fp4WgtSignMsb ? i_Fp8ActMatNeg2[i] : i_Fp8ActMatPos2[i]; break;
                case 3: Fp4MatMulMsb = Fp4WgtSignMsb ? i_Fp8ActMatNeg3[i] : i_Fp8ActMatPos3[i]; break;
                default: Fp4MatMulMsb = 0; break;
            }

            switch (mat_lsb) {
                case 0: Fp4MatMulLsb = 0; break;
                case 1: Fp4MatMulLsb = Fp4WgtSignLsb ? i_Fp8ActMatNeg1[i] : i_Fp8ActMatPos1[i]; break;
                case 2: Fp4MatMulLsb = Fp4WgtSignLsb ? i_Fp8ActMatNeg2[i] : i_Fp8ActMatPos2[i]; break;
                case 3: Fp4MatMulLsb = Fp4WgtSignLsb ? i_Fp8ActMatNeg3[i] : i_Fp8ActMatPos3[i]; break;
                default: Fp4MatMulLsb = 0; break;
            }

            int64_t Fp4MatShiftLsb, Fp4MatShiftMsb;
            Fp4MatShiftLsb = (sign_extend(Fp4MatMulLsb, 22) << wgt_lsb_exp) & bitmask_u64(25);
            Fp4MatShiftMsb = (sign_extend(Fp4MatMulMsb, 22) << wgt_msb_exp) & bitmask_u64(25);

            psum_msb[i] = Fp4MatShiftMsb >> (25-MAT_WIDTH);
            psum_lsb[i] = Fp4MatShiftLsb >> (25-MAT_WIDTH);
        }

        int64_t sum_msb = IntAdderTree(psum_msb, MAT_WIDTH);
        int64_t sum_lsb = IntAdderTree(psum_lsb, MAT_WIDTH);

        o_ProductMatMsb = sum_msb;
        o_ProductMatLsb = sum_lsb;
        o_ProductExp = i_Fp8ActExp;
    }
}
