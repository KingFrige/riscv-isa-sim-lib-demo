// +FHDR========================================================================
//  File Name:      MxFp8ActQuant.hpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BF16 to MxFP8 Quantization Model
// -FHDR========================================================================
#ifndef MXFP8_ACT_QUANT_HPP
#define MXFP8_ACT_QUANT_HPP

#include <cstdint>
#include <vector>

struct MxFp8ActQuant_Model {
    uint32_t block_size;
    std::vector<uint8_t> o_MxFp8Act;
    uint8_t o_MxFp8ActScale;

    MxFp8ActQuant_Model(uint32_t block_size)
        : block_size(block_size),
          o_MxFp8Act(block_size, 0),
          o_MxFp8ActScale(0) {}

    ~MxFp8ActQuant_Model() = default;

    void process(const uint16_t *i_Bf16Act);
};

#endif // MXFP8_ACT_QUANT_HPP
