// +FHDR========================================================================
//  File Name:      Fp32PsumAcc.cpp
//  Description:    FP32 Partial Sum Accumulator Implementation
// -FHDR========================================================================
#include "Fp32PsumAcc.hpp"
#include "../util.h"

static void fp_add(uint16_t exp1, int64_t mat1,
                   uint16_t exp2, int64_t mat2,
                   uint16_t *result_exp, int64_t *result_mat){
    if (exp1 > exp2) {
        mat2 >>= (exp1 - exp2);
        *result_exp = exp1;
    } else {
        mat1 >>= (exp2 - exp1);
        *result_exp = exp2;
    }

    int64_t sum = mat1 + mat2;
    int sign_bit = (sum >> (MAT_WIDTH + 5)) & 1;
    int msb = (sum >> (MAT_WIDTH + 4)) & 1;
    int overflow = (sign_bit != msb);

    if (overflow) {
        *result_mat = (sum >> 7);
        (*result_exp)++;
    } else {
        *result_mat = sign_extend( (sum >> 6) & bitmask_u64(MAT_WIDTH - 1),  MAT_WIDTH-1);
    }
}

static uint8_t round_mantissa_to_7bits(uint64_t mantissa, uint8_t is_normal){
    uint8_t guard, sticky, round_enable;
    uint8_t mat_7bit;

    if (is_normal) {
        guard = (mantissa >> (MAT_WIDTH - 10)) & 1;
        sticky = (mantissa & bitmask_u64(MAT_WIDTH - 10)) != 0;
        round_enable = guard && (sticky || ((mantissa >> (MAT_WIDTH - 9)) & 1));
        mat_7bit = (mantissa >> (MAT_WIDTH - 9)) & 0x7F;
    } else {
        guard = (mantissa >> (MAT_WIDTH - 9)) & 1;
        sticky = (mantissa & bitmask_u64(MAT_WIDTH - 9)) != 0;
        round_enable = guard && (sticky || ((mantissa >> (MAT_WIDTH - 8)) & 1));
        mat_7bit = (mantissa >> (MAT_WIDTH - 8)) & 0x7F;
    }

    return mat_7bit + round_enable;
}

static uint16_t to_bf16(uint16_t fp32_exp, int64_t fp32_mat, uint8_t mode_fp8fp8){
    uint8_t sign = (fp32_mat >> (MAT_WIDTH - 1)) & 1;
    uint64_t abs_mat;
    if (sign) {
        uint64_t mat_bits = fp32_mat & bitmask_u64(MAT_WIDTH - 1);
        abs_mat = (~mat_bits + 1) & bitmask_u64(MAT_WIDTH - 1);
    } else {
        abs_mat = fp32_mat & bitmask_u64(MAT_WIDTH - 1);
    }

    if (abs_mat == 0) {
        return (sign << 15);
    }

    int leading_zeros = 0;
    for (int i = MAT_WIDTH - 2; i >= 0; i--) {
        if (abs_mat & (1ULL << i)) break;
        leading_zeros++;
    }
    uint64_t mat_shifted = abs_mat << leading_zeros;

    int32_t max_threshold = mode_fp8fp8 ? 388 : 391;
    int32_t min_threshold = mode_fp8fp8 ? 134 : 137;
    int32_t adj_exp = (int32_t)fp32_exp + EXT_WIDTH - leading_zeros;

    uint8_t bf16_exp_bias;
    uint64_t bf16_mat_bias;
    uint8_t is_normal;

    if (adj_exp > max_threshold) {
        bf16_exp_bias = 0xFF;
        bf16_mat_bias = 0;
        is_normal = 0;
    } else if (adj_exp <= min_threshold) {
        bf16_exp_bias = 0;
        int shift = min_threshold + leading_zeros - EXT_WIDTH - fp32_exp;
        bf16_mat_bias = mat_shifted >> shift;
        is_normal = 0;
    } else {
        bf16_exp_bias = adj_exp - min_threshold;
        bf16_mat_bias = mat_shifted;
        is_normal = 1;
    }

    uint8_t mat_rounded = round_mantissa_to_7bits(bf16_mat_bias, is_normal);
    uint8_t bf16_exp;
    uint8_t bf16_mat;

    if ((int32_t)bf16_exp_bias + ((mat_rounded >> 7) & 1) > 254) {
        bf16_exp = 0xFF;
        bf16_mat = 0;
    } else {
        bf16_exp = (bf16_exp_bias & 0xFF) + ((mat_rounded >> 7) & 1);
        if (mat_rounded & 0x80) {
            bf16_mat = is_normal ? ((mat_rounded >> 1) & 0x3F) : (0x40 | ((mat_rounded >> 1) & 0x3F));
        } else {
            bf16_mat = mat_rounded & 0x7F;
        }
    }

    return (sign << 15) | (bf16_exp << 7) | bf16_mat;
}

void Fp32PsumAccModel::compute(
    uint8_t i_CfgFp8Fp8, uint8_t i_CfgBf16Norm,
    uint8_t i_ProductExp, int64_t i_ProductMat,
    uint8_t i_ScaleAct, uint8_t i_ScaleWgt, uint32_t i_ScaleTensor,
    uint16_t i_PsumExp, int64_t i_PsumMat){

    uint8_t scale_wgt_exp;
    uint8_t scale_wgt_mat;
    if(i_CfgFp8Fp8){
      scale_wgt_exp = i_ScaleWgt;
      scale_wgt_mat = 0x8;
    } else {
      scale_wgt_exp = (i_ScaleWgt >> 3) & 0xF;
      scale_wgt_mat = scale_wgt_exp != 0 ? (1<<3 | (i_ScaleWgt & 0x7)) : (i_ScaleWgt & 0x7) << 1;
    }

    uint16_t product_exp  = i_ProductExp + i_ScaleAct + scale_wgt_exp;
    int64_t  product_mat  = sign_extend(i_ProductMat,MAT_WIDTH) * sign_extend(scale_wgt_mat,5);

    uint16_t acc_exp;
    int64_t  acc_mat;

    if (i_PsumExp == 0 && i_PsumMat == 0) {
        acc_exp = product_exp;
        acc_mat = sign_extend( (product_mat>>6) & bitmask_u64(MAT_WIDTH-1), MAT_WIDTH-1);
    } else {
        fp_add(product_exp, product_mat, i_PsumExp, sign_extend(i_PsumMat<<6,MAT_WIDTH+5), &acc_exp, &acc_mat);
    }

    uint8_t TensorScaleExp;
    uint32_t TensorScaleMat;
    if(i_CfgFp8Fp8){
      TensorScaleExp = 0;
      TensorScaleMat = 0x800000;
    } else {
      TensorScaleExp = (i_ScaleTensor >> 23) & 0xFF;
      TensorScaleMat = TensorScaleExp != 0 ? (1<<23 | (i_ScaleTensor & 0x7FFFFF)) : (i_ScaleTensor & 0x7FFFFF) << 1;
    }

    uint16_t PartialScaleExp;
    int64_t PartialScaleMat;
    PartialScaleExp = acc_exp + TensorScaleExp;
    PartialScaleMat = acc_mat * sign_extend(TensorScaleMat,25);
    PartialScaleMat = sign_extend( (PartialScaleMat >> 24) & bitmask_u64(MAT_WIDTH), MAT_WIDTH);

    if (i_CfgBf16Norm) {
        uint16_t bf16 = to_bf16(PartialScaleExp, PartialScaleMat, i_CfgFp8Fp8);
        o_Psum = 0xFFFF0000 | bf16;
    } else {
        uint32_t exp_bits = (uint32_t)(acc_exp & bitmask_u64(EXP_WIDTH + 1));
        uint32_t mat_bits = (uint32_t)(acc_mat & bitmask_u64(MAT_WIDTH - 1));
        o_Psum = (exp_bits << (MAT_WIDTH - 1)) | mat_bits;
    }
}
