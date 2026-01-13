// +FHDR========================================================================
//  File Name:      SoftmaxCore.cpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    SoftmaxCore Golden Model Implementation
// -FHDR========================================================================
#include "SoftmaxCore.hpp"
#include "BF16.hpp"
#include <algorithm>

template<typename Op>
uint16_t SoftmaxCore_Model::reduction_tree_op(const std::vector<uint16_t>& vector, Op op) {
    std::vector<uint16_t> nodes = vector;

    while (nodes.size() > 1) {
        std::vector<uint16_t> next_level_nodes;
        for (size_t i = 0; i < nodes.size(); i += 2) {
            if (i + 1 < nodes.size()) {
                uint16_t res_val = op(nodes[i], nodes[i + 1]);
                next_level_nodes.push_back(res_val);
            } else {
                next_level_nodes.push_back(nodes[i]);
            }
        }
        nodes = next_level_nodes;
    }

    return nodes[0];
}


SoftmaxResult SoftmaxCore_Model::process(const std::vector<std::vector<uint16_t>>& input_row) {
    SoftmaxResult result;

    if (input_row.empty() || input_row[0].empty()) { return result;}

    BF16::init_luts();

    // --- Frontend Pass: Find x_max ---
    uint16_t neg_inf = BF16::NEG_INF;
    uint16_t running_max = neg_inf;

    for (const auto& vec : input_row) {
        // 1. Simulate frontend.max_tree (max within vector)
        uint16_t max_of_vec = reduction_tree_op(vec, BF16::bf16_max);
        result.intermediate_results.frontend_max_tree_out.push_back(max_of_vec);

        // 2. Simulate frontend.max_final_comp (max among vectors)
        running_max = BF16::bf16_max(running_max, max_of_vec);
        result.intermediate_results.frontend_max_final_out.push_back(running_max);
    }

    uint16_t x_max = running_max;

    // --- Backend Pass ---
    // Stage 1 & 2: (x_i - x_max) and exp(x_i - x_max)
    std::vector<std::vector<uint16_t>> exp_results_vectors;

    for (const auto& vec : input_row) {
        std::vector<uint16_t> sub_vec;
        std::vector<uint16_t> exp_vec;

        for (uint16_t val : vec) {
            // 3. Simulate backend.exp_sum_stage.sub_units
            uint16_t sub_res = BF16::bf16_add(val, x_max, true);  // val - x_max
            sub_vec.push_back(sub_res);

            // 4. Simulate backend.exp_sum_stage.exp_units
            uint16_t exp_res = BF16::bf16_exp(sub_res);
            //uint16_t exp_res = BF16::bf16_expp(sub_res);
            exp_vec.push_back(exp_res);
        }

        result.intermediate_results.expsum_sub_out.push_back(sub_vec);
        result.intermediate_results.expsum_exp_out.push_back(exp_vec);
        exp_results_vectors.push_back(exp_vec);
    }

    // --- Stage 3: Sum(exp_results) ---
    uint16_t sum_exp = BF16::make_zero(0);

    for (const auto& exp_vec : exp_results_vectors) {
        uint16_t partial_sum = reduction_tree_op(exp_vec, [](uint16_t a, uint16_t b) {
            return BF16::bf16_add(a, b);
        });
        sum_exp = BF16::bf16_add(sum_exp, partial_sum);
    }

    // --- Stage 4: Reciprocal of sum_exp ---
    uint16_t s_inv = BF16::bf16_reciprocal(sum_exp);

    // --- Stage 5: Multiply ---
    std::vector<uint16_t> final_results_flat;
    for (const auto& vec : exp_results_vectors) {
        for (uint16_t exp_val : vec) {
            uint16_t final_val = BF16::bf16_mul(exp_val, s_inv);
            final_results_flat.push_back(final_val);
        }
    }

    // Reshape flat results back to vectors
    for (size_t i = 0; i < final_results_flat.size(); i += vec_width) {
        std::vector<uint16_t> output_vec(
            final_results_flat.begin() + i,
            final_results_flat.begin() + std::min(i + vec_width, final_results_flat.size())
        );
        result.final_output.push_back(output_vec);
    }

    result.x_max = x_max;
    result.s_inv = s_inv;

    return result;
}
