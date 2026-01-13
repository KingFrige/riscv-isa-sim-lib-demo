#include "GemmTop.hpp"
#include "../util.h"
#include "MxFp8ActDeNorm.hpp"
#include "MxFp8Product.hpp"
#include "Fp32PsumAcc.hpp"

void GemmTopModel::compute(
    uint8_t       i_CfgFp8Fp8,
    uint8_t       i_CfgBf16Norm,
    uint32_t      i_ScaleTensor,
    const uint8_t i_MxFp8Act[BATCH_SIZE][ROW_SIZE],
    const uint8_t i_MxFp8Wgt[COL_SIZE * ROW_SIZE],
    uint8_t       i_ScaleAct[BATCH_SIZE],
    uint8_t       i_ScaleWgt_lsb[COL_SIZE],
    uint8_t       i_ScaleWgt_msb[COL_SIZE]) {

    static MxFp8ActDeNormModel act_denorm[BATCH_SIZE];
    for (int b = 0; b < BATCH_SIZE; ++b) {
        act_denorm[b].compute(i_MxFp8Act[b]);
    }

    static MxFp8ProductModel product[BATCH_SIZE][COL_SIZE];
    for (int b = 0; b < BATCH_SIZE; ++b) {
        for (int col = 0; col < COL_SIZE; ++col) {
            uint8_t col_wgt[ROW_SIZE];
            for (int idx = 0; idx < ROW_SIZE; ++idx) {
                col_wgt[idx] = i_MxFp8Wgt[idx * COL_SIZE + col];
            }

            product[b][col].compute(
                i_CfgFp8Fp8,
                act_denorm[b].o_Fp8ActSign,
                act_denorm[b].o_Fp8ActExp,
                act_denorm[b].o_Fp8ActMatPos1,
                act_denorm[b].o_Fp8ActMatNeg1,
                act_denorm[b].o_Fp8ActMatPos2,
                act_denorm[b].o_Fp8ActMatNeg2,
                act_denorm[b].o_Fp8ActMatPos3,
                act_denorm[b].o_Fp8ActMatNeg3,
                col_wgt
            );
        }
    }

    static Fp32PsumAccModel acc0[BATCH_SIZE][COL_SIZE];
    static Fp32PsumAccModel acc1[BATCH_SIZE][COL_SIZE];

    for (int b = 0; b < BATCH_SIZE; ++b) {
        for (int col = 0; col < COL_SIZE; ++col) {
            acc0[b][col].compute(
                i_CfgFp8Fp8,
                i_CfgBf16Norm,
                product[b][col].o_ProductExp,
                product[b][col].o_ProductMatLsb,
                i_ScaleAct[b],
                i_ScaleWgt_lsb[col],
                i_ScaleTensor,
                psum0_exp[b][col],
                psum0_mat[b][col]
            );

            if (!i_CfgFp8Fp8) {
                acc1[b][col].compute(
                    i_CfgFp8Fp8,
                    i_CfgBf16Norm,
                    product[b][col].o_ProductExp,
                    product[b][col].o_ProductMatMsb,
                    i_ScaleAct[b],
                    i_ScaleWgt_msb[col],
                    i_ScaleTensor,
                    psum1_exp[b][col],
                    psum1_mat[b][col]
                );
            }

            o_Psum0[b][col] = acc0[b][col].o_Psum;
            if (!i_CfgFp8Fp8) {
                o_Psum1[b][col] = acc1[b][col].o_Psum;
            }

            if (i_CfgBf16Norm) {
                psum0_exp[b][col] = 0;
                psum0_mat[b][col] = 0;
                if (!i_CfgFp8Fp8) {
                    psum1_exp[b][col] = 0;
                    psum1_mat[b][col] = 0;
                }
            } else {
                psum0_exp[b][col] = (acc0[b][col].o_Psum >> (MAT_WIDTH - 1)) & bitmask_u64(EXP_WIDTH + 1);
                psum0_mat[b][col] = sign_extend(acc0[b][col].o_Psum & bitmask_u64(MAT_WIDTH - 1), MAT_WIDTH - 1);
                if (!i_CfgFp8Fp8) {
                    psum1_exp[b][col] = (acc1[b][col].o_Psum >> (MAT_WIDTH - 1)) & bitmask_u64(EXP_WIDTH + 1);
                    psum1_mat[b][col] = sign_extend(acc1[b][col].o_Psum & bitmask_u64(MAT_WIDTH - 1), MAT_WIDTH - 1);
                }
            }
        }
    }
}
