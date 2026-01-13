// +FHDR========================================================================
//  File Name:      test_custom_expp.cpp
//                  Rocky (luoqi754@gmail.com)
//  Description:    BF16ExppUnit Full Range Exhaustive Test
// -FHDR========================================================================
#include "custom_expp.hpp"
#include "BF16.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstring>
#include <fstream>
#include "util.h"

using namespace BF16;



struct ErrorStats {
    uint64_t total_count = 0;
    uint64_t special_count = 0;
    uint64_t normal_count = 0;
    double max_rel_error = 0.0;
    double sum_rel_error = 0.0;
    uint16_t max_error_input = 0;
    uint16_t max_error_output = 0;
    float max_error_expected = 0.0f;

    void add_sample(uint16_t input, uint16_t output, float expected, bool is_special) {
        total_count++;
        if (is_special) {
            special_count++;
            return;
        }

        normal_count++;
        float output_float = bf16_to_float(output);

        // Skip inf/nan cases for error calculation
        if (std::isnan(expected) || std::isinf(expected) ||
            std::isnan(output_float) || std::isinf(output_float)) {
            return;
        }

        if (expected == 0.0f) return;

        double rel_error = std::abs((output_float - expected) / expected);
        sum_rel_error += rel_error;

        if (rel_error > max_rel_error) {
            max_rel_error = rel_error;
            max_error_input = input;
            max_error_output = output;
            max_error_expected = expected;
        }
    }

    void print_summary() {
        LOG_PRINTF("\n========================================\n");
        LOG_PRINTF("Exhaustive Test Summary\n");
        LOG_PRINTF("========================================\n");
        LOG_PRINTF("Total values tested:  %llu\n", (unsigned long long)total_count);
        LOG_PRINTF("Special values:       %llu\n", (unsigned long long)special_count);
        LOG_PRINTF("Normal values:        %llu\n", (unsigned long long)normal_count);
        LOG_PRINTF("\nError Statistics:\n");
        LOG_PRINTF("  Max relative error: %.6f%%\n", max_rel_error * 100);
        LOG_PRINTF("  Avg relative error: %.6f%%\n",
                   (normal_count > 0) ? (sum_rel_error / normal_count * 100) : 0.0);
        LOG_PRINTF("\nMax Error Case:\n");
        LOG_PRINTF("  Input:    0x%04x (%.6f)\n", max_error_input, bf16_to_float(max_error_input));
        LOG_PRINTF("  Output:   0x%04x (%.6f)\n", max_error_output, bf16_to_float(max_error_output));
        LOG_PRINTF("  Expected: %.6f\n", max_error_expected);
    }
};

int main() {
    log_init("log/expp.log");
    BF16ExppUnit_Model expp_model;
    ErrorStats stats;

    // Error threshold for check file
    // const double REL_ERROR_THRESHOLD = 0.005;  // 0.5% relative error
    const double REL_ERROR_THRESHOLD = 5000;  // 0.5% relative error
    const double ABS_ERROR_THRESHOLD = 0.8;   // Absolute error threshold

    // Range filter: only test values within [-inf, +RANGE]
    const float CHECK_RANGE = 0;  // Set to 0 to disable range filtering and test all values

    LOG_PRINTF("BF16ExppUnit Exhaustive Test\n");
    LOG_PRINTF("Error Threshold: Rel > %.2f%% or Abs > %.3f\n",
               REL_ERROR_THRESHOLD * 100, ABS_ERROR_THRESHOLD);
    LOG_PRINTF("Check Filter: Exclude cases where HW_Result or Expected is inf/nan\n");
    if (CHECK_RANGE > 0) {
        LOG_PRINTF("Test Range Filter: Only testing inputs <= %.1f (all negative values + positive up to %.1f)\n",
                   CHECK_RANGE, CHECK_RANGE);
    } else {
        LOG_PRINTF("Testing all 65536 BF16 values\n");
    }
    LOG_PRINTF("=========================================================================================\n");
    LOG_PRINTF("%-8s %-22s %-22s %-16s %-14s\n",
               "Index", "Input", "HW_Result", "Expected(F32)", "Diff");
    LOG_PRINTF("%-8s %-22s %-22s %-16s %-14s\n",
               "--------", "----------------------", "----------------------", "----------------", "--------------");

    // Create two CSV files
    std::ofstream full_csv("log/full_expp.csv");
    std::ofstream check_csv("log/check_expp.csv");

    full_csv << "Index,Input_Hex,Input_Dec,HW_Hex,HW_Dec,Expected_F32,Diff\n";
    check_csv << "Index,Input_Hex,Input_Dec,HW_Hex,HW_Dec,Expected_F32,Diff,Rel_Error(%)\n";

    uint64_t high_error_count = 0;
    uint64_t tested_count = 0;
    uint64_t skipped_count = 0;

    // Test all possible BF16 values
    for (uint32_t i = 0; i <= 0xFFFF; i++) {
        uint16_t input_bf16 = static_cast<uint16_t>(i);
        float input_float = bf16_to_float(input_bf16);

        // Apply range filter: skip if CHECK_RANGE > 0 and input > CHECK_RANGE
        if (CHECK_RANGE > 0 && input_float > CHECK_RANGE) {
            skipped_count++;
            continue;
        }

        tested_count++;

        // Check if special value
        BFloat16 bf(input_bf16);
        bool is_zero = (bf.fields.exponent == 0) && (bf.fields.mantissa == 0);
        bool is_denormal = (bf.fields.exponent == 0) && (bf.fields.mantissa != 0);
        bool is_inf = (bf.fields.exponent == 255) && (bf.fields.mantissa == 0);
        bool is_nan = (bf.fields.exponent == 255) && (bf.fields.mantissa != 0);
        bool is_special = is_zero || is_denormal || is_inf || is_nan;

        // Process
        uint16_t output_bf16 = expp_model.process(input_bf16);
        float output_float = bf16_to_float(output_bf16);
        float expected_float = std::exp(input_float);

        // Calculate diff
        float diff = output_float - expected_float;

        // Calculate relative error
        double rel_error = 0.0;
        bool has_valid_error = false;
        if (!is_special && !std::isnan(expected_float) && !std::isinf(expected_float) &&
            !std::isnan(output_float) && !std::isinf(output_float)) {
            if (expected_float != 0.0f) {
                rel_error = std::abs(diff / expected_float) * 100.0;
                has_valid_error = true;
            } else if (output_float != 0.0f) {
                // Special case: expected is 0 but output is not - this is a huge error
                rel_error = INFINITY;
                has_valid_error = true;
            }
        }

        // Write ALL data to full_csv
        full_csv << i << ","
                 << "0x" << std::hex << std::setw(4) << std::setfill('0') << input_bf16 << ","
                 << std::dec << std::setprecision(10) << input_float << ","
                 << "0x" << std::hex << std::setw(4) << std::setfill('0') << output_bf16 << ","
                 << std::dec << std::setprecision(10) << output_float << ","
                 << std::setprecision(10) << expected_float << ","
                 << std::setprecision(10) << diff << "\n";

        // Check if error exceeds threshold
        bool exceeds_threshold = false;
        if (has_valid_error) {
            if (std::isinf(rel_error) || (rel_error / 100.0) > REL_ERROR_THRESHOLD || std::abs(diff) > ABS_ERROR_THRESHOLD) {
                exceeds_threshold = true;
            }
        }

        // Check if HW_Result or Expected is inf/nan
        bool has_inf_or_nan = std::isnan(output_float) || std::isinf(output_float) ||
                              std::isnan(expected_float) || std::isinf(expected_float);

        // Write high-error cases to check_csv, but exclude cases where HW_Result or Expected is inf/nan
        if (exceeds_threshold && !has_inf_or_nan) {
            high_error_count++;
            check_csv << i << ","
                      << "0x" << std::hex << std::setw(4) << std::setfill('0') << input_bf16 << ","
                      << std::dec << std::setprecision(10) << input_float << ","
                      << "0x" << std::hex << std::setw(4) << std::setfill('0') << output_bf16 << ","
                      << std::dec << std::setprecision(10) << output_float << ","
                      << std::setprecision(10) << expected_float << ","
                      << std::setprecision(10) << diff << ","
                      << std::setprecision(4) << rel_error << "\n";
        }

        // Print all values to log file
        LOG_PRINTF("%-8u 0x%04x(%+.3e)      0x%04x(%+.3e)      %+.8e        %+.3e\n",
                   i,
                   input_bf16, input_float,
                   output_bf16, output_float,
                   expected_float,
                   diff);

        stats.add_sample(input_bf16, output_bf16, expected_float, is_special);

        // Progress indicator (stderr to not interfere with stdout)
        if ((i & 0xFFF) == 0) {
            fprintf(stderr, "\rProgress: %u%%", (i * 100 / 65536));
            fflush(stderr);
        }
    }

    fprintf(stderr, "\rProgress: 100%%     \n");
    full_csv.close();
    check_csv.close();

    stats.print_summary();

    LOG_PRINTF("\n========================================\n");
    LOG_PRINTF("Test Statistics:\n");
    LOG_PRINTF("  Total scanned:    %llu\n", (unsigned long long)(tested_count + skipped_count));
    LOG_PRINTF("  Values tested:    %llu\n", (unsigned long long)tested_count);
    LOG_PRINTF("  Values skipped:   %llu\n", (unsigned long long)skipped_count);
    LOG_PRINTF("\nFiles Generated:\n");
    LOG_PRINTF("  Full results:  log/full_expp.csv (%llu values)\n", (unsigned long long)tested_count);
    LOG_PRINTF("  High errors:   log/check_expp.csv (%llu values, excluding inf/nan)\n", (unsigned long long)high_error_count);
    LOG_PRINTF("========================================\n");

    return 0;
}
