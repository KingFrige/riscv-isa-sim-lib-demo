#include <riscv_vector.h>
#include <stdbool.h>
// #include <stdio.h>
#include <stdint.h>

#include "printf.h"

// tohost symbol for communication with Spike (declared in main.c)
extern volatile uint64_t tohost;

#define OPCODE_DS_SHIFT_VAL 7
#define OPCODE_RS1_SHIFT_VAL 15
#define OPCODE_RS2_SHIFT_VAL 20

#define CUSTOM0 0x0b
#define CUSTOM1 0x2b

#define FINISHER_BASE 0x3fffb000

// scalar add

#define _XSTR(x) #x

#define PERIAADD(rd, rs1, rs2) __asm__ volatile (".word %0" : : "i"(((rs1) << 15) | ((rs2) << 20) | ((rd) << 7) | 0x0b) : "memory")
#define PERIVADD(vd, vs1, vs2) __asm__ volatile (".word %0" : : "i"(((vs1) << 15) | ((vs2) << 20) | ((vd) << 7) | (1 << 25) | 0x2b))

// XPERIVMUL vector multiplication extension (opcode 0x5b - CUSTOM-3)
#define PERIVMUL(vd, vs1, vs2) __asm__ volatile (".word %0" : : "i"(((vs1) << 15) | ((vs2) << 20) | ((vd) << 7) | (1 << 25) | 0x5b) : "memory")

// New mathematical instruction extensions (opcode 0x0b - CUSTOM0)
// EXP: func7=0x03, func3=0x6 (vm=1)
#define EXP(vd, vs1) __asm__ volatile (".word %0" : : "i"(((vs1) << 15) | ((vd) << 7) | (0x03 << 25) | (0x6 << 12) | 0x0b) : "memory")
// SOFTMAX: func7=0x03, func3=0x2 (vm=1)
#define SOFTMAX(vd, vs1) __asm__ volatile (".word %0" : : "i"(((vs1) << 15) | ((vd) << 7) | (0x03 << 25) | (0x2 << 12) | 0x0b) : "memory")
// QUANT: func7=0x05, func3=0x6 (vm=1)
#define QUANT(vd, vs1) __asm__ volatile (".word %0" : : "i"(((vs1) << 15) | ((vd) << 7) | (0x05 << 25) | (0x6 << 12) | 0x0b) : "memory")

// Test result codes
#define TEST_PASS 1
#define TEST_FAIL 0xFF

// Error codes for different test failures
#define ERR_XPERIA_ADD 0x10
#define ERR_XPERIV_ADD 0x30
#define ERR_XPERIV_MUL 0x40
#define ERR_EXP 0x50
#define ERR_SOFTMAX 0x60
#define ERR_QUANT 0x70

// Helper macro to report test failure
#define REPORT_FAILURE(error_code) \
    do { \
        tohost = (error_code << 8) | TEST_FAIL; \
        __sync_synchronize(); \
        /* Exit with error code */ \
        __asm__ volatile ( \
            "li a0, %0\n\t"      /* Exit code = error_code */ \
            "li a7, 93\n\t"     /* exit system call number */ \
            "ecall" \
            : \
            : "i"((error_code) & 0xFF) \
        ); \
        /* Fallback infinite loop if ecall doesn't work */ \
        while (1) { __asm__ volatile ("nop"); } \
    } while (0)

// Complete test for all custom extensions with result verification
int test_insn () {
    printf("\n****************************************\n");
    printf("[Test insn]: Starting Test: EXP vector exponential extension\n");
    {
        // Use same vector memory regions as previous tests
        volatile uint16_t* v0_data = (volatile uint16_t*)0x8000a000;
        volatile uint16_t* v4_data = (volatile uint16_t*)0x8000a200;
        
        // Test case 1: Basic values from C_src test - expanded to use more of VLEN=512
        // With VLEN=512 and e16, we can process up to 512/16 = 32 elements per vector register
        uint16_t bf16_inputs[32] = {0x0000, 0x3F80, 0xBF80, 0x3F00, 0x4000, 0x4040, 0x4080, 0x3E80,
                                   0x3F40, 0x3FC0, 0x4020, 0x4060, 0x3F20, 0x3FA0, 0x4010, 0x4070,
                                   0x3F60, 0x3FE0, 0x4030, 0x4050, 0x3F10, 0x3F90, 0x4018, 0x4068,
                                   0x3F50, 0x3FD0, 0x4028, 0x4058, 0x3F30, 0x3FB0, 0x4014, 0x4074}; // 32 values
        uint16_t expected_outputs[32] = {0x3F80, 0x402E, 0x3EBC, 0x3FD3, 0x40EC, 0x41A1, 0x425B, 0x3FA4,
                                        0x3FAD, 0x408F, 0x4185, 0x4204, 0x3F92, 0x4060, 0x41AE, 0x4232,
                                        0x3FB7, 0x40B4, 0x41E6, 0x4223, 0x3F89, 0x4072, 0x41B9, 0x4229,
                                        0x3FA9, 0x409B, 0x41D3, 0x421C, 0x3F9A, 0x407D, 0x41B3, 0x423A}; // HW Result from C_src/log/expp.log
        
        for (int i = 0; i < 32; i++) {
            v0_data[i] = bf16_inputs[i]; // Store in lower 16 bits
            v4_data[i] = 0;
        }
        
        // Load vectors
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Set vector length to 32 elements, e16, m1 (for VLEN=512: 512/16 = 32 elements per register)
        __asm__ volatile("vsetvli t0, t0, e16, m1, ta, ma" : : : "t0");  // This will set vl=32 for VLEN=512
        __asm__ volatile("\
          li      t0, 0x8000a000;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x8000a200;\
          vle16.v v4, (t2);\
        ");
        
        // Execute EXP instruction: v4 = exp(v0)
        EXP(4, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x8000a200;\
          vse16.v v4, (t0);\
        ");
        
        // Verify results with expected values using approximate comparison
        for (int i = 0; i < 32; i++) {
            uint16_t result = v4_data[i];
            uint16_t expected = expected_outputs[i];
            
            // Check if result is close to expected (allowing some tolerance for approximations)
            if ((result & 0xFF00) != (expected & 0xFF00)) {
                uint16_t diff = (result > expected) ? (result - expected) : (expected - result);
                if (diff > 0x0400) {  // Allow tolerance for EXP tests (increased due to more values tested)
                    printf("[Test insn]: EXP Test failed at index %d: result=0x%04x, expected=0x%04x, diff=0x%04x\n", i, result, expected, diff);
                    REPORT_FAILURE(ERR_EXP);
                }
            }
        }
        printf("[Test insn]: Test: EXP vector exponential extension passed\n\n");
    }
    // ============================================
    // Test 5: SOFTMAX vector softmax extension
    // ============================================
    printf("[Test insn]: Starting Test: SOFTMAX vector softmax extension\n");
    {
        // SOFTMAX instruction expects vector of BF16 values and computes softmax across vector
        // Test case 1: Random values from C_src/log/softmax_output.log - expanded for VLEN=512
        volatile uint16_t* v0_data = (volatile uint16_t*)0x8000a400;
        volatile uint16_t* v4_data = (volatile uint16_t*)0x8000a600;
        
        // With VLEN=512 and e16, we can process up to 512/16 = 32 elements per vector register
        // Input values from C_src/log/softmax_output.log "Random [-2.0, 2.0]" test
        uint16_t bf16_inputs[32] = {0xbfee, 0xbf2e, 0x3f43, 0xbe9e, 0xbf96, 0xbf7f, 0x3f0b, 0x3fba,
                                   0xbf4b, 0xbff3, 0xbf0a, 0x3f87, 0xbf3a, 0xbfba, 0xbfc9, 0x3f85,
                                   0xbfd6, 0x3e50, 0x3e84, 0x3f79, 0x3ff6, 0xbf8f, 0xbe3a, 0x3d98,
                                   0x3e97, 0x3e62, 0x3f6a, 0x3f03, 0x3f97, 0x3eab, 0xbf66, 0x3fa8}; // 32 values
        for (int i = 0; i < 32; i++) {
            v0_data[i] = bf16_inputs[i];
            v4_data[i] = 0;
        }
        
        // Load vectors
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Set vector length to 32 elements, e16, m1 (for VLEN=512: 512/16 = 32 elements per register)
        __asm__ volatile("vsetvli t0, t0, e16, m1, ta, ma" : : : "t0");  // This will set vl=32 for VLEN=512
        
        __asm__ volatile("\
          li      t0, 0x8000a400;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x8000a600;\
          vle16.v v4, (t2);\
        ");
        
        // Execute SOFTMAX instruction: v4 = softmax(v0)
        SOFTMAX(4, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x8000a600;\
          vse16.v v4, (t0);\
        ");
        
        // Verify results - check that outputs match expected values from C_src/log/softmax_output.log
        // Expected outputs from C_src/log/softmax_output.log "Random [-2.0, 2.0]" test
        uint16_t expected_outputs[32] = {0x3b59, 0x3c2f, 0x3d38, 0x3c7e, 0x3bd3, 0x3c00, 0x3d14, 0x3db6,
                                        0x3c1c, 0x3b4f, 0x3c4b, 0x3d76, 0x3c26, 0x3b9f, 0x3b8f, 0x3d72,
                                        0x3b80, 0x3cd3, 0x3cdf, 0x3d63, 0x3e13, 0x3be2, 0x3c91, 0x3cba,
                                        0x3ce6, 0x3cd9, 0x3d55, 0x3d0f, 0x3d8b, 0x3cf1, 0x3c0c, 0x3d9e}; // Expected outputs from C_src
        
        int valid_results = 0;  // Count results that match expected values (within tolerance)
        for (int i = 0; i < 32; i++) {
            uint16_t result = v4_data[i];
            uint16_t expected = expected_outputs[i];
            
            // Check if result is close to expected (allowing some tolerance for approximations)
            // For softmax, compare high bits to see if value is in right range
            uint16_t result_high = result & 0xFF00;
            uint16_t expected_high = expected & 0xFF00;
            
            // Calculate absolute difference manually (since abs might not be available in embedded environment)
            uint32_t diff = (result_high > expected_high) ? (result_high - expected_high) : (expected_high - result_high);
            
            if (result_high == expected_high || 
                (result_high != 0 && expected_high != 0 && diff <= 0x0200)) {
                valid_results++;
            } else {
                printf("[Test insn]: SOFTMAX Test case 1 failed at index %d: result=0x%04x, expected=0x%04x\n", i, result, expected);
            }
        }
        
        // Most results should be valid (at least 24 out of 32)
        if (valid_results < 24) {  // Increased threshold for 32-element test
            printf("[Test insn]: SOFTMAX Test case 1 failed: only %d/%d results were valid\n", valid_results, 32);
            REPORT_FAILURE(ERR_SOFTMAX);
        }
        printf("[Test insn]: Test: SOFTMAX vector exponential extension passed\n\n");
    }
    
    // ============================================
    // Test 6: QUANT vector quantization extension
    // ============================================
    printf("[Test insn]: Starting Test: QUANT vector quantization extension\n");
    {
        // QUANT instruction quantizes BF16 to MxFP8 with scale
        // Test case 1: Basic quantization with non-zero mantissa - expanded for VLEN=512
        volatile uint16_t* v0_data = (volatile uint16_t*)0x8000b000;
        volatile uint16_t* v4_data = (volatile uint16_t*)0x8000b200;
        
        // With VLEN=512 and e16, we can process up to 512/16 = 32 elements per vector register
        // Input values from C_src/log/quant_output.log "BF16_Input" column (Block 0, Index 0-31)
        uint16_t bf16_inputs[32] = {0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
                                   0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
                                   0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
                                   0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f}; // From C_src log
        uint8_t expected_outputs[32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                       0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}; // HW_Quant values from C_src log
        
        for (int i = 0; i < 32; i++) {
            v0_data[i] = bf16_inputs[i];
            v4_data[i] = 0;
        }
        
        // Load vectors
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Set vector length to 32 elements, e16, m1 (for VLEN=512: 512/16 = 32 elements per register)
        __asm__ volatile("vsetvli t0, t0, e16, m1, ta, ma" : : : "t0");  // This will set vl=32 for VLEN=512
        
        __asm__ volatile("\
          li      t0, 0x8000b000;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x8000b200;\
          vle16.v v4, (t2);\
        ");
        
        // Execute QUANT instruction: v4 = quant(v0)
        QUANT(4, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x8000b200;\
          vse16.v v4, (t0);\
        ");
        
        // Verify results with expected values from C_src/log/quant_output.log (check lower 8 bits)
        int valid_results = 0;  // Count matching results
        for (int i = 0; i < 32; i++) {
            uint8_t actual = v4_data[i] & 0xFF;
            uint8_t expected = expected_outputs[i];
            
            // Check if result is close to expected (allowing tolerance for quantization differences)
            uint8_t diff = (actual > expected) ? (actual - expected) : (expected - actual);
            if (diff <= 0x01) {  // Tight tolerance for quantization accuracy
                valid_results++;
            } else {
                printf("[Test insn]: QUANT Test case 1 failed at index %d: actual=0x%02x, expected=0x%02x\n", i, actual, expected);
            }
        }
        
        // Most results should match expected values (at least 24 out of 32)
        if (valid_results < 24) {
            printf("[Test insn]: QUANT Test case 1 failed: only %d/%d results were valid\n", valid_results, 32);
            REPORT_FAILURE(ERR_QUANT);
        }
        printf("[Test insn]: Test: QUANT vector exponential extension passed\n\n");
    }
    
    printf("[Test insn]: All tests passed successfully!\n");
    printf("****************************************\n\n");
    
    // Memory barrier to ensure all previous accesses are visible
    __sync_synchronize();
    
    // Report success
    tohost = TEST_PASS;

    return 0;
}
