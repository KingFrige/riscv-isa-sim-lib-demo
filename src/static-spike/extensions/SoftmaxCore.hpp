// +FHDR========================================================================
//  File Name:      SoftmaxCore.hpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    SoftmaxCore Golden Model
// -FHDR========================================================================
#ifndef SOFTMAX_CORE_HPP
#define SOFTMAX_CORE_HPP

#include <cstdint>
#include <vector>

struct SoftmaxIntermediateResults {
    std::vector<uint16_t> frontend_max_tree_out;//max_of_vec
    std::vector<uint16_t> frontend_max_final_out;//max among vectors
    std::vector<std::vector<uint16_t>> expsum_sub_out;// val - x_max
    std::vector<std::vector<uint16_t>> expsum_exp_out;// exp(val - x_max)
};

struct SoftmaxResult {
    std::vector<std::vector<uint16_t>> final_output;
    SoftmaxIntermediateResults intermediate_results;
    uint16_t x_max;
    uint16_t s_inv;
};

struct SoftmaxCore_Model {
    uint32_t vec_width;

    SoftmaxCore_Model(uint32_t vec_width) : vec_width(vec_width) {}
    ~SoftmaxCore_Model() = default;

    SoftmaxResult process(const std::vector<std::vector<uint16_t>>& input_row);

private:
    template<typename Op>
    uint16_t reduction_tree_op(const std::vector<uint16_t>& vector, Op op);
};

#endif // SOFTMAX_CORE_HPP