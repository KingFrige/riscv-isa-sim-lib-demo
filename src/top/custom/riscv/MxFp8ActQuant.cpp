// +FHDR========================================================================
//  File Name:      MxFp8ActQuant.cpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BF16 to MxFP8 Quantization Implementation
// -FHDR========================================================================
#include "MxFp8ActQuant.hpp"
#include <algorithm>

void MxFp8ActQuant_Model::process(const uint16_t *i_Bf16Act) {
    if (!i_Bf16Act) return;

    const uint32_t DATA_SIZE = block_size;

    // DFF1: Find max exponent and calculate scaling factor
    std::vector<uint8_t> Bf16ActExp(DATA_SIZE);
    uint8_t Bf16ActExpMax = 0;

    for (uint32_t i = 0; i < DATA_SIZE; i++) {
        Bf16ActExp[i] = (i_Bf16Act[i] >> 7) & 0xFF;
        if (Bf16ActExp[i] > Bf16ActExpMax) {
            Bf16ActExpMax = Bf16ActExp[i];
        }
    }

    uint8_t MxFp8Scale   = (Bf16ActExpMax > 8) ? (Bf16ActExpMax - 8) : 7;
    uint8_t MxFp8ExpInit = (Bf16ActExpMax > 8) ? 15 : Bf16ActExpMax;

    // Align exponent and mantissa
    std::vector<uint8_t> MxFp8Sign(DATA_SIZE);
    std::vector<uint8_t> MxFp8Exp(DATA_SIZE);
    std::vector<uint8_t> MxFp8Mat(DATA_SIZE);
    std::vector<uint8_t> MxFp8MatShiftAmt(DATA_SIZE);
    std::vector<uint8_t> MxFp8ExpShift(DATA_SIZE);
    std::vector<uint8_t> MxFp8MatShift(DATA_SIZE);

    for (uint32_t i = 0; i < DATA_SIZE; i++) {
        MxFp8Sign[i] = (i_Bf16Act[i] >> 15) & 0x1;
        MxFp8Exp[i] = (i_Bf16Act[i] >> 7) & 0xFF;
        uint8_t mant_frac = i_Bf16Act[i] & 0x7F;  // 7-bit fractional mantissa
        // Add implicit leading 1 for normal numbers (exp != 0)
        MxFp8Mat[i] = (MxFp8Exp[i] == 0) ? mant_frac : (0x80 | mant_frac);
        MxFp8MatShiftAmt[i] = Bf16ActExpMax - MxFp8Exp[i];

        if (MxFp8ExpInit >= MxFp8MatShiftAmt[i]) {// Normal
            MxFp8ExpShift[i] = MxFp8ExpInit - MxFp8MatShiftAmt[i];
            if (MxFp8ExpShift[i] == 0) {
                MxFp8MatShift[i] = MxFp8Mat[i] >> 1;
            } else {
                MxFp8MatShift[i] = MxFp8Mat[i];
            }
        } else {// Denormal,Efp8<0
            MxFp8ExpShift[i] = 0;
            uint8_t shift_amt = MxFp8MatShiftAmt[i] - MxFp8ExpInit;
            shift_amt += 1;
            MxFp8MatShift[i] = (shift_amt < 8) ? (MxFp8Mat[i] >> shift_amt) : 0;
        }
        MxFp8MatShift[i] = MxFp8MatShift[i] & 0x7F;
    }

    o_MxFp8ActScale = MxFp8Scale;

    // DFF2: Round and output
    for (uint32_t i = 0; i < DATA_SIZE; i++) {
        uint8_t MxFp8ActMatGrd = (MxFp8MatShift[i] >> 3) & 0x1;
        uint8_t MxFp8ActMatSty = (MxFp8MatShift[i] & 0x07) != 0;
        uint8_t MxFp8ActMatRndEn = MxFp8ActMatGrd &&
                                    (((MxFp8MatShift[i] >> 4) & 0x1) || MxFp8ActMatSty);

        // Calculate rounded mantissa
        uint8_t temp_rnd = ((MxFp8MatShift[i] >> 4) & 0x07) + MxFp8ActMatRndEn;
        uint8_t temp_carry = (temp_rnd >> 3) & 0x1;

        uint32_t final_exp_candidate = MxFp8ExpShift[i] + temp_carry;
        uint8_t final_mant_candidate = temp_rnd & 0x7;

        bool would_be_nan = (final_exp_candidate >= 15) && (final_mant_candidate == 0x7);
        uint8_t MxFp8ActMatRnd = would_be_nan ? 0x06 : temp_rnd;


        uint8_t final_sign = MxFp8Sign[i];
        uint8_t final_exp;
        uint8_t final_mat;
        uint8_t mat_carry = (MxFp8ActMatRnd >> 3) & 0x1;

        if ((MxFp8ExpShift[i] + mat_carry) > 15) {
            // Overflow: saturate to max (exp=15, mant=6)
            final_exp = 0xF;
            final_mat = 0x6;
        } else {
            final_exp = (MxFp8ExpShift[i] + mat_carry) & 0xF;
            final_mat = MxFp8ActMatRnd & 0x7;
        }

        o_MxFp8Act[i] = (final_sign << 7) | (final_exp << 3) | final_mat;
    }
}
