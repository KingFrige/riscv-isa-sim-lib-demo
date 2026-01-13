// +FHDR========================================================================
//  File Name:      Fp32PsumAcc.hpp
//  Description:    FP32 Partial Sum Accumulator Model
// -FHDR========================================================================
#ifndef FP32_PSUM_ACC_HPP
#define FP32_PSUM_ACC_HPP

#include <cstdint>
#include "config.h"

struct Fp32PsumAccModel {
    uint32_t o_Psum;

    Fp32PsumAccModel() : o_Psum(0) {}
    ~Fp32PsumAccModel() = default;

    void compute(uint8_t i_CfgFp8Fp8, uint8_t i_CfgBf16Norm,
                 uint8_t i_ProductExp, int64_t i_ProductMat,
                 uint8_t i_ScaleAct, uint8_t i_ScaleWgt, uint32_t i_ScaleTensor,
                 uint16_t i_PsumExp, int64_t i_PsumMat);
};

#endif // FP32_PSUM_ACC_HPP
