// +FHDR========================================================================
//  File Name:      MxFp8Product.hpp
//  Description:    MxFP8 Product Calculation Model
// -FHDR========================================================================
#ifndef MXFP8_PRODUCT_HPP
#define MXFP8_PRODUCT_HPP

#include <cstdint>
#include <cstring>
#include "config.h"

struct MxFp8ProductModel {
    uint8_t o_ProductExp;
    int64_t o_ProductMatMsb;
    int64_t o_ProductMatLsb;

    MxFp8ProductModel() : o_ProductExp(0), o_ProductMatMsb(0), o_ProductMatLsb(0) {}
    ~MxFp8ProductModel() = default;

    void compute(uint8_t i_CfgFp8Fp8,
                 const uint8_t i_Fp8ActSign[ROW_SIZE],
                 uint8_t i_Fp8ActExp,
                 const int64_t i_Fp8ActMatPos1[ROW_SIZE],
                 const int64_t i_Fp8ActMatNeg1[ROW_SIZE],
                 const int64_t i_Fp8ActMatPos2[ROW_SIZE],
                 const int64_t i_Fp8ActMatNeg2[ROW_SIZE],
                 const int64_t i_Fp8ActMatPos3[ROW_SIZE],
                 const int64_t i_Fp8ActMatNeg3[ROW_SIZE],
                 const uint8_t i_MxFp8Wgt[ROW_SIZE]);
};

#endif // MXFP8_PRODUCT_HPP
