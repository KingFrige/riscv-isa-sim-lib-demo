// +FHDR========================================================================
//  File Name:      main_softmax.cpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    SoftmaxCore Golden Model Test
// -FHDR========================================================================
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <cstring>
#include "BF16.hpp"
#include "SoftmaxCore.hpp"
#include "util.h"

using namespace BF16;

// ============================================================================
// Test Parameters (modify as needed)
// ============================================================================
#define VEC_WIDTH   32      // Elements per vector
#define RANDOM_MIN  -2.0f  // Random value minimum
#define RANDOM_MAX   2.0f  // Random value maximum

void test_softmax_case(const char* test_name, const std::vector<std::vector<uint16_t>>& input_row, uint32_t vec_width) {
    LOG_PRINTF("\n========================================\n");
    LOG_PRINTF("Test: %s\n", test_name);
    LOG_PRINTF("========================================\n");
    LOG_PRINTF("VEC_WIDTH=%u, NUM_VECTORS=%zu, TOTAL_LEN=%zu\n",
               vec_width, input_row.size(), input_row.size() * vec_width);

    // Process
    SoftmaxCore_Model model(vec_width);
    SoftmaxResult result = model.process(input_row);

    // Print key results
    LOG_PRINTF("\nKey Results:\n");
    LOG_PRINTF("  x_max:   0x%04x (%.6f)\n", result.x_max, bf16_to_float(result.x_max));
    LOG_PRINTF("  s_inv:   0x%04x (%.6f)\n", result.s_inv, bf16_to_float(result.s_inv));

    // Print input/output table
    LOG_PRINTF("\nSoftmax Input/Output Table:\n");
    LOG_PRINTF("-------------------------------------------------------\n");
    LOG_PRINTF("Index |  Input_BF16  |  Input_Float  | Output_BF16  | Output_Float\n");
    LOG_PRINTF("------|--------------|---------------|--------------|-------------\n");

    size_t idx = 0;
    for (size_t v = 0; v < input_row.size(); v++) {
        for (size_t i = 0; i < input_row[v].size(); i++, idx++) {
            uint16_t in_val = input_row[v][i];
            uint16_t out_val = result.final_output[v][i];
            float in_float = bf16_to_float(in_val);
            float out_float = bf16_to_float(out_val);

            LOG_PRINTF("%5zu |   0x%04x    |  %+.6e  |   0x%04x    | %.8f\n",
                       idx, in_val, in_float, out_val, out_float);
        }
    }

    // Verify sum ≈ 1.0
    float sum = 0.0f;
    for (const auto& vec : result.final_output) {
        for (uint16_t val : vec) {
            sum += bf16_to_float(val);
        }
    }
    LOG_PRINTF("-------------------------------------------------------\n");
    LOG_PRINTF("Sum check: %.8f (should be ~1.0)\n", sum);
}

int main(int argc, char* argv[]) {
    int ROW_LEN = 512;  // Default value

    // Parse command line arguments
    if (argc > 1) {
        ROW_LEN = atoi(argv[1]);
        if (ROW_LEN <= 0 || ROW_LEN % 32 != 0) {
            TERMINAL_PRINTF("Error: ROW_LEN must be a positive multiple of 32\n");
            return 1;
        }
    }

    srand(42);  // Fixed seed for reproducibility

    // Generate output file name based on ROW_LEN: softmax_output_mN.log where N = ROW_LEN >> 5
    int m_value = ROW_LEN >> 5;  // This is ROW_LEN / 32
    char log_filename[256];
    snprintf(log_filename, sizeof(log_filename), "./log/softmax_output_m%d.log", m_value);
    log_init(log_filename);

    TERMINAL_PRINTF("VEC_WIDTH=%d, ROW_LEN=%d\n\n", VEC_WIDTH, ROW_LEN);

    const uint32_t NUM_VECTORS = ROW_LEN / VEC_WIDTH;

    // Test 1: Sequential ascending values
    {
        std::vector<std::vector<uint16_t>> input;
        uint16_t base_bf16 = 0xbf2a;
        for (uint32_t v = 0; v < NUM_VECTORS; v++) {
            std::vector<uint16_t> vec;
            for (uint32_t i = 0; i < VEC_WIDTH; i++) {
                uint16_t bf16_val = base_bf16 + (v * VEC_WIDTH + i);
                vec.push_back(bf16_val);
            }
            input.push_back(vec);
        }
        test_softmax_case("Sequential Ascending", input, VEC_WIDTH);
    }

    // Test 2: Random values in configurable range
    {
        std::vector<std::vector<uint16_t>> input;
        for (uint32_t v = 0; v < NUM_VECTORS; v++) {
            std::vector<uint16_t> vec;
            for (uint32_t i = 0; i < VEC_WIDTH; i++) {
                float val = ((float)rand() / RAND_MAX) * (RANDOM_MAX - RANDOM_MIN) + RANDOM_MIN;
                vec.push_back(float_to_bf16(val));
            }
            input.push_back(vec);
        }
        char test_name[64];
        snprintf(test_name, sizeof(test_name), "Random [%.1f, %.1f]", RANDOM_MIN, RANDOM_MAX);
        test_softmax_case(test_name, input, VEC_WIDTH);
    }

    log_close();
    TERMINAL_PRINTF("\nResults saved to %s\n", log_filename);
    return 0;
}
