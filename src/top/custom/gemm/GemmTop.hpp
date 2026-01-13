#ifndef GEMM_TOP_HPP
#define GEMM_TOP_HPP

#include <cstdint>
#include <cstring>
#include "config.h"

struct GemmTopModel {
    uint32_t o_Psum0[BATCH_SIZE][COL_SIZE];
    uint32_t o_Psum1[BATCH_SIZE][COL_SIZE];

    uint16_t psum0_exp[BATCH_SIZE][COL_SIZE];
    int64_t  psum0_mat[BATCH_SIZE][COL_SIZE];
    uint16_t psum1_exp[BATCH_SIZE][COL_SIZE];
    int64_t  psum1_mat[BATCH_SIZE][COL_SIZE];

    GemmTopModel() {
        std::memset(o_Psum0, 0, sizeof(o_Psum0));
        std::memset(o_Psum1, 0, sizeof(o_Psum1));
        std::memset(psum0_exp, 0, sizeof(psum0_exp));
        std::memset(psum0_mat, 0, sizeof(psum0_mat));
        std::memset(psum1_exp, 0, sizeof(psum1_exp));
        std::memset(psum1_mat, 0, sizeof(psum1_mat));
    }

    ~GemmTopModel() = default;

    void compute(
        uint8_t       i_CfgFp8Fp8,
        uint8_t       i_CfgBf16Norm,
        uint32_t      i_ScaleTensor,
        const uint8_t i_MxFp8Act[BATCH_SIZE][ROW_SIZE],
        const uint8_t i_MxFp8Wgt[COL_SIZE * ROW_SIZE],
        uint8_t       i_ScaleAct[BATCH_SIZE],
        uint8_t       i_ScaleWgt_lsb[COL_SIZE],
        uint8_t       i_ScaleWgt_msb[COL_SIZE]);
};

#endif // GEMM_TOP_HPP
