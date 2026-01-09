// +FHDR========================================================================
//  File Name:      MxFp8ActQuant.hpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BF16 to MxFP8 Quantization Model
// -FHDR========================================================================
#ifndef MXFP8ACTQUANT_HPP
#define MXFP8ACTQUANT_HPP

#include <cstdint>
#include <vector>

class MxFp8ActQuant_Model {
public:
    uint32_t block_size;
    uint8_t o_MxFp8ActScale;
    std::vector<uint8_t> o_MxFp8Act;

    MxFp8ActQuant_Model(uint32_t size) : block_size(size) {
        o_MxFp8Act.resize(block_size);
    }
    ~MxFp8ActQuant_Model() = default;

    void process(const uint16_t *i_Bf16Act);
};

#endif // MXFP8ACTQUANT_HPP