// +FHDR========================================================================
//  License:
//      
//  =============================================================================
//  File Name:      main_gemm.c
//                  Rocky (luoqi754@gmail.com)
//  Organization:
//  Description:
//      Performs (1,4096)×(4096,16) matrix multiplication using 16×16 tiles
// -FHDR========================================================================

#include <cstdio>
#include <cstdint>
#include <cstring>
#include "util.h"
#include "GemmTop.hpp"

void update_inputs(
    int iter,
    int num_iterations,
    uint8_t *i_MxFp8Act,
    uint8_t *i_MxFp8Wgt,
    uint8_t *i_ScaleWgt_lsb,
    int enable_increment
) {
    if (enable_increment) {
        // Increment i_MxFp8Act with carry (starting from index 0)
        if (iter > 0 && iter <= 62) {
            int carry = 1;
            for (int i = 0; i < ROW_SIZE && carry; ++i) {
                int sum = i_MxFp8Act[i] + carry;
                i_MxFp8Act[i] = sum & 0xFF;
                carry = sum >> 8;
            }
        } else if (iter > 62) {
            // Reset to all 0x3c after iter 62
            for (int i = 0; i < ROW_SIZE; ++i) {
                i_MxFp8Act[i] = 0x3c;
            }
        }

        // Increment i_MxFp8Wgt (first col only) with carry (starting from row 0)
        if (iter == num_iterations - 1) {
            // Last iteration: reset all to 0x42
            for (int i = 0; i < COL_SIZE * ROW_SIZE; ++i) {
                i_MxFp8Wgt[i] = 0x42;
            }
        } else if (iter > 0) {
            int carry = 1;
            for (int row = 0; row < ROW_SIZE && carry; ++row) {
                int idx = row * COL_SIZE;  // Index of element in column 0
                int sum = i_MxFp8Wgt[idx] + carry;
                i_MxFp8Wgt[idx] = sum & 0xFF;
                carry = sum >> 8;
            }
        }

        // Update i_ScaleWgt_lsb[0] pattern: 7b 7b 7c 7c 7d 7d ... 81 81, then all 7b
        if (iter < 14) {
            i_ScaleWgt_lsb[0] = 0x7b + (iter / 2);
        } else {
            i_ScaleWgt_lsb[0] = 0x7b;
        }
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // Initialize log file
    log_init("./log/gemm_output.log");

    uint8_t i_CfgFp8Fp8 = 1;  // FP8×FP8 mode

    uint8_t i_MxFp8Act[BATCH_SIZE][ROW_SIZE];
    uint8_t i_MxFp8Wgt[COL_SIZE * ROW_SIZE];

    for (int b = 0; b < BATCH_SIZE; ++b) {
        for (int i = 0; i < ROW_SIZE; ++i) {
            i_MxFp8Act[b][i] = 0x3c;
        }
    }
    for (int i = 0; i < COL_SIZE * ROW_SIZE; ++i) {
        i_MxFp8Wgt[i] = 0x42;
    }

    LOG_PRINTF("Mode: %s\n", i_CfgFp8Fp8 ? "FP8xFP8" : "FP8xFP4");
    LOG_PRINTF("Tile size: %dx%d\n", ROW_SIZE, COL_SIZE);
    LOG_PRINTF("Number of iterations: %d (4096/16)\n\n", 4096/16);

    TERMINAL_PRINTF("Mode: %s\n", i_CfgFp8Fp8 ? "FP8xFP8" : "FP8xFP4");
    TERMINAL_PRINTF("Tile size: %dx%d\n", ROW_SIZE, COL_SIZE);
    TERMINAL_PRINTF("Number of iterations: %d (4096/16)\n\n", 4096/16);

    uint8_t i_ScaleAct[BATCH_SIZE];
    uint8_t i_ScaleWgt_lsb[COL_SIZE];
    uint8_t i_ScaleWgt_msb[COL_SIZE];
    uint32_t i_ScaleTensor = 0x3F800000;  // 1.0 in IEEE 754 format
    for (int b = 0; b < BATCH_SIZE; ++b) {
        i_ScaleAct[b] = 0x7d;
    }
    for (int i = 0; i < COL_SIZE; ++i) {
        i_ScaleWgt_lsb[i] = 0x7b;
        i_ScaleWgt_msb[i] = 0x7b;
    }

    GemmTopModel gemm;

    // Matrix multiplication: 4096×4096
    // Each iteration processes: (1×16) × (16×16) = (1×16) output
    // We need 4096/16 = 256 iterations to accumulate the full dot product
    int num_iterations = 4096 / 16;

    // Set to 1 to enable input increment pattern, 0 to keep inputs constant
    int enable_increment = 0;

    for (int iter = 0; iter < num_iterations; ++iter) {
        uint8_t i_CfgBf16Norm = (iter == num_iterations - 1) ? 1 : 0;

        // Update inputs based on increment mode
        update_inputs(iter, num_iterations, i_MxFp8Act[0], i_MxFp8Wgt, i_ScaleWgt_lsb, enable_increment);

        gemm.compute(
            i_CfgFp8Fp8,
            i_CfgBf16Norm,
            i_ScaleTensor,
            i_MxFp8Act,
            i_MxFp8Wgt,
            i_ScaleAct,
            i_ScaleWgt_lsb,
            i_ScaleWgt_msb
        );
    }

    // Print results
    LOG_PRINTF("========================================\n");
    LOG_PRINTF("Final Output Results (o_Psum0)\n");
    LOG_PRINTF("========================================\n");
    for (int batch = 0; batch < BATCH_SIZE; ++batch) {
        LOG_PRINTF("Batch %d:\n", batch);
        for (int col = 0; col < COL_SIZE; ++col) {
            LOG_PRINTF("o_Psum0[%d][%2d] = 0x%08x", batch, col, gemm.o_Psum0[batch][col]);

            // Decode BF16 value
            uint16_t bf16 = gemm.o_Psum0[batch][col] & 0xFFFF;
            uint8_t sign = (bf16 >> 15) & 0x1;
            uint8_t exp = (bf16 >> 7) & 0xFF;
            uint8_t mat = bf16 & 0x7F;

            LOG_PRINTF("  (BF16: sign=%d, exp=0x%02x, mat=0x%02x)", sign, exp, mat);

            // Convert to approximate float value for reference
            if (exp == 0xFF) {
                LOG_PRINTF(" [Inf/NaN]");
            } else if (exp == 0) {
                LOG_PRINTF(" [Denorm/Zero]");
            } else {
                // Approximate float value
                float f_val = (sign ? -1.0f : 1.0f) *
                             (1.0f + (float)mat / 128.0f) *
                             (1 << (exp - 127));
                LOG_PRINTF(" ≈ %.6e", f_val);
            }
            LOG_PRINTF("\n");
        }
        LOG_PRINTF("\n");
    }

    LOG_PRINTF("\n");
    LOG_PRINTF("========================================\n\n");
    LOG_PRINTF("Final Output Results (o_Psum1)\n");
    LOG_PRINTF("========================================\n");
    for (int batch = 0; batch < BATCH_SIZE; ++batch) {
        LOG_PRINTF("Batch %d:\n", batch);
        for (int col = 0; col < COL_SIZE; ++col) {
            LOG_PRINTF("o_Psum1[%d][%2d] = 0x%08x", batch, col, gemm.o_Psum1[batch][col]);

            // Decode BF16 value
            uint16_t bf16 = gemm.o_Psum1[batch][col] & 0xFFFF;
            uint8_t sign = (bf16 >> 15) & 0x1;
            uint8_t exp = (bf16 >> 7) & 0xFF;
            uint8_t mat = bf16 & 0x7F;

            LOG_PRINTF("  (BF16: sign=%d, exp=0x%02x, mat=0x%02x)", sign, exp, mat);

            // Convert to approximate float value for reference
            if (exp == 0xFF) {
                LOG_PRINTF(" [Inf/NaN]");
            } else if (exp == 0) {
                LOG_PRINTF(" [Denorm/Zero]");
            } else {
                // Approximate float value
                float f_val = (sign ? -1.0f : 1.0f) *
                             (1.0f + (float)mat / 128.0f) *
                             (1 << (exp - 127));
                LOG_PRINTF(" ≈ %.6e", f_val);
            }
            LOG_PRINTF("\n");
        }
        LOG_PRINTF("\n");
    }

    // Close log file
    log_close();

    TERMINAL_PRINTF("\nResults have been saved to log/gemm_output.log\n");

    return 0;
}
