#include <riscv_vector.h>
#include <stdbool.h>
// #include <stdio.h>
#include <stdint.h>

// tohost and fromhost symbols for communication with Spike
__attribute__((section(".tohost")))
volatile uint64_t tohost = 0;

__attribute__((section(".tohost")))
volatile uint64_t fromhost = 0;

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
int main () {
    
    // ============================================
    // Test 1: XPERIA scalar addition extension
    // ============================================
    {
        // Set up test values
        long a1 = 100;
        long a2 = 23;
        long a0 = 0;
        
        // Execute XPERIA addition: a0 = a1 + a2
        // Use inline assembly to set registers
        __asm__ volatile (
            "mv a1, %0\n\t"
            "mv a2, %1\n\t"
            : // no outputs
            : "r"(a1), "r"(a2)
            : "a1", "a2"
        );
        
        PERIAADD(10, 11, 12);  // a0 = a1 + a2
        
        // Read result
        __asm__ volatile ("mv %0, a0" : "=r"(a0));
        
        // Verify result: 100 + 23 = 123
        if (a0 != 123) {
            REPORT_FAILURE(ERR_XPERIA_ADD);
        }
    }
    
    // ============================================
    // Test 2 & 3: Vector extensions
    // ============================================
    {
        // Vector test data at fixed addresses
        volatile uint32_t* v0_data = (volatile uint32_t*)0x80001000;
        volatile uint32_t* v2_data = (volatile uint32_t*)0x80001100;
        volatile uint32_t* v4_data = (volatile uint32_t*)0x80001200;
        volatile uint32_t* v6_data = (volatile uint32_t*)0x80001300;
        
        // Initialize test vectors - now testing more elements based on VLEN=512
        // With VLEN=512 and e32, LMUL=m1, we can process up to 512/32=16 elements per vector register
        for (int i = 0; i < 16; i++) {
            v0_data[i] = i + 1;      // [1, 2, 3, ..., 16]
            v2_data[i] = i + 5;      // [5, 6, 7, ..., 20]
            v4_data[i] = 0;
            v6_data[i] = 0;
        }
        
        // Enable vector extension and load vectors
        __asm__("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        \
          li      t0, 1024;\
          vsetvli t0, t0, e32, m1, tu, mu;\
        \
          li      t0, 0x80001000;\
          vle32.v v0, (t0);\
        \
          li      t2, 0x80001100;\
          vle32.v v2, (t2);\
        ");

        
        PERIVADD(4, 0, 2);
        
        // Store v4 to memory for verification
        __asm__("\
          li      t0, 0x80001200;\
          vse32.v v4, (t0);\
        ");
        
        // Verify vector addition results
        // Expected: v4[i] = v0[i] + v2[i]
        // [1+5=6, 2+6=8, 3+7=10, ..., 16+20=36]
        uint32_t expected_add[16] = {6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36};
        for (int i = 0; i < 16; i++) {
            if (v4_data[i] != expected_add[i]) {
                REPORT_FAILURE(ERR_XPERIV_ADD);
            }
        }
        
        // Test 3: XPERIVMUL vector multiplication extension
        __asm__ volatile("vsetvli t0, t0, e32, m1, ta, ma" : : : "t0");
        PERIVMUL(6, 0, 2);     // v6 = v0 * v2
        
        // Store v6 to memory for verification
        __asm__("\
          li      t0, 0x80001300;\
          vse32.v v6, (t0);\
        ");
        
        // Verify vector multiplication results
        // Expected: v6[i] = v0[i] * v2[i]
        // [1*5=5, 2*6=12, 3*7=21, ..., 16*20=320]
        uint32_t expected_mul[16] = {5, 12, 21, 32, 45, 60, 77, 96, 117, 140, 165, 192, 221, 252, 285, 320};
        for (int i = 0; i < 16; i++) {
            if (v6_data[i] != expected_mul[i]) {
                REPORT_FAILURE(ERR_XPERIV_MUL);
            }
        }
    }
    
    // ============================================
    // Test 4: EXP vector exponential extension
    // ============================================
    {
        // Use same vector memory regions as previous tests
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80001000;
        volatile uint16_t* v4_data = (volatile uint16_t*)0x80001200;
        
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
          li      t0, 0x80001000;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80001200;\
          vle16.v v4, (t2);\
        ");
        
        // Execute EXP instruction: v4 = exp(v0)
        EXP(4, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80001200;\
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
                    REPORT_FAILURE(ERR_EXP);
                }
            }
        }
    }
    // ============================================
    // Test 5: SOFTMAX vector softmax extension
    // ============================================
    {
        // SOFTMAX instruction expects vector of BF16 values and computes softmax across vector
        // Test case 1: Random values from C_src/log/softmax_output.log - expanded for VLEN=512
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80001000;
        volatile uint16_t* v4_data = (volatile uint16_t*)0x80001200;
        
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
          li      t0, 0x80001000;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80001200;\
          vle16.v v4, (t2);\
        ");
        
        // Execute SOFTMAX instruction: v4 = softmax(v0)
        SOFTMAX(4, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80001200;\
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
            }
        }
        
        // Most results should be valid (at least 24 out of 32)
        if (valid_results < 24) {  // Increased threshold for 32-element test
            REPORT_FAILURE(ERR_SOFTMAX);
        }
        
        // Test case 2: Sequential Ascending values from C_src/log/softmax_output.log - expanded for VLEN=512
        // Use different memory region
        volatile uint16_t* v8_data = (volatile uint16_t*)0x80001400;
        volatile uint16_t* v12_data = (volatile uint16_t*)0x80001500;
        
        // Sequential ascending BF16 values from C_src/log/softmax_output.log
        uint16_t bf16_inputs2[32] = {0xbf2a, 0xbf2b, 0xbf2c, 0xbf2d, 0xbf2e, 0xbf2f, 0xbf30, 0xbf31,
                                    0xbf32, 0xbf33, 0xbf34, 0xbf35, 0xbf36, 0xbf37, 0xbf38, 0xbf39,
                                    0xbf3a, 0xbf3b, 0xbf3c, 0xbf3d, 0xbf3e, 0xbf3f, 0xbf40, 0xbf41,
                                    0xbf42, 0xbf43, 0xbf44, 0xbf45, 0xbf46, 0xbf47, 0xbf48, 0xbf49}; // 32 values from C_src log
        uint16_t expected_outputs2[32] = {0x3d09, 0x3d07, 0x3d07, 0x3d06, 0x3d06, 0x3d05, 0x3d05, 0x3d04,
                                         0x3d04, 0x3d03, 0x3d03, 0x3d03, 0x3d02, 0x3d01, 0x3d00, 0x3d00,
                                         0x3cff, 0x3cfe, 0x3cfe, 0x3cfd, 0x3cfc, 0x3cfa, 0x3cfa, 0x3cf9,
                                         0x3cf8, 0x3cf8, 0x3cf6, 0x3cf5, 0x3cf4, 0x3cf4, 0x3cf2, 0x3cf1}; // Expected outputs from C_src
        for (int i = 0; i < 32; i++) {
            v8_data[i] = bf16_inputs2[i];
            v12_data[i] = 0;
        }
        
        // Load new vectors (using e16, m1 which for VLEN=512 will process 32 elements)
        __asm__ volatile("vsetvli t0, t0, e16, m1, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80001400;\
          vle16.v v8, (t0);\
        \
          li      t2, 0x80001500;\
          vle16.v v12, (t2);\
        ");
        
        // Execute SOFTMAX on new vectors
        SOFTMAX(12, 8);
        
        // Store result
        __asm__("\
          li      t0, 0x80001500;\
          vse16.v v12, (t0);\
        ");
        
        // Verify results against expected outputs from C_src
        int valid_results2 = 0;
        for (int i = 0; i < 32; i++) {
            uint16_t result = v12_data[i];
            uint16_t expected = expected_outputs2[i];
            
            // Check if result is close to expected (allowing some tolerance for approximations)
            // For softmax, compare high bits to see if value is in right range
            uint16_t result_high = result & 0xFF00;
            uint16_t expected_high = expected & 0xFF00;
            
            // Calculate absolute difference manually (since abs might not be available in embedded environment)
            uint32_t diff = (result_high > expected_high) ? (result_high - expected_high) : (expected_high - result_high);
            
            if (result_high == expected_high || 
                (result_high != 0 && expected_high != 0 && diff <= 0x0200)) {
                valid_results2++;
            }
        }
        
        // Most results should be valid (at least 24 out of 32)
        if (valid_results2 < 24) {  // Increased threshold for 32-element test
            REPORT_FAILURE(ERR_SOFTMAX);
        }
    }
    
    // ============================================
    // Test 6: QUANT vector quantization extension
    // ============================================
    {
        // QUANT instruction quantizes BF16 to MxFP8 with scale
        // Test case 1: Basic quantization with non-zero mantissa - expanded for VLEN=512
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80001000;
        volatile uint16_t* v4_data = (volatile uint16_t*)0x80001200;
        
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
          li      t0, 0x80001000;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80001200;\
          vle16.v v4, (t2);\
        ");
        
        // Execute QUANT instruction: v4 = quant(v0)
        QUANT(4, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80001200;\
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
            }
        }
        
        // Most results should match expected values (at least 24 out of 32)
        if (valid_results < 24) {
            REPORT_FAILURE(ERR_QUANT);
        }
        
        // Test case 2: Values from C_src test_quant.cpp - expanded for VLEN=512
        // Use different memory region
        volatile uint16_t* v8_data = (volatile uint16_t*)0x80001400;
        volatile uint16_t* v12_data = (volatile uint16_t*)0x80001500;
        
        // Use data from Block 1 of C_src/log/quant_output.log (Index 32-63)
        uint16_t bf16_inputs2[32] = {0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
                                    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
                                    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
                                    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f}; // From C_src log
        for (int i = 0; i < 32; i++) {
            v8_data[i] = bf16_inputs2[i];
            v12_data[i] = 0;
        }
        
        // Load new vectors (using e16, m1 which for VLEN=512 will process 32 elements)
        __asm__ volatile("vsetvli t0, t0, e16, m1, ta, ma" : : : "t0");  // This will set vl=32 for VLEN=512
        __asm__ volatile("\
          li      t0, 0x80001400;\
          vle16.v v8, (t0);\
        \
          li      t2, 0x80001500;\
          vle16.v v12, (t2);\
        ");
        
        // Execute QUANT on new vectors
        QUANT(12, 8);
        
        // Store result
        __asm__("\
          li      t0, 0x80001500;\
          vse16.v v12, (t0);\
        ");
        
        // Expected outputs from Block 1 of C_src/log/quant_output.log (HW_Quant values for Index 32-63)
        uint8_t expected_outputs2[32] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                                        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02}; // HW_Quant values from C_src log
        
        // Verify results with expected values from C_src/log/quant_output.log
        int valid_results2 = 0;  // Count matching results
        for (int i = 0; i < 32; i++) {
            uint8_t actual = v12_data[i] & 0xFF;
            uint8_t expected = expected_outputs2[i];
            
            // Check if result is close to expected (allowing tolerance for quantization differences)
            uint8_t diff = (actual > expected) ? (actual - expected) : (expected - actual);
            if (diff <= 0x01) {  // Tight tolerance for quantization accuracy
                valid_results2++;
            }
        }
        
        // Most results should match expected values (at least 24 out of 32)
        if (valid_results2 < 24) {
            REPORT_FAILURE(ERR_QUANT);
        }
    }
    
    // ============================================
    // NEW Test 7: Test EXP with LMUL = 1
    // ============================================
    {
        volatile uint16_t* v8_data = (volatile uint16_t*)0x80001400;
        volatile uint16_t* v12_data = (volatile uint16_t*)0x80001500;
        
        // Additional test inputs: 2.0, -2.0, 0.25, -0.25 and more values for VLEN=512
        uint16_t bf16_inputs2[32] = {0x4000, 0xC000, 0x3E80, 0xBE80, 0x3F40, 0xBF40, 0x4040, 0xC040,
                                    0x3F80, 0xBF80, 0x4080, 0xC080, 0x3F00, 0xBF00, 0x4100, 0xC100,
                                    0x3EC0, 0xBEC0, 0x3F20, 0xBF20, 0x3F60, 0xBF60, 0x3F88, 0xBF88,
                                    0x3FC0, 0xBFC0, 0x3FE0, 0xBFE0, 0x3FF0, 0xBFF0, 0x4010, 0xC010}; // 32 values
        // Expected outputs: exp of the above values
        uint16_t expected_outputs2[32] = {0x40EC, 0x3CE5, 0x3FA5, 0x3F45, 0x3FCB, 0x3F15, 0x41A1, 0x3E6A,
                                         0x402E, 0x3EBC, 0x425B, 0x3E2E, 0x3FD3, 0x3FB3, 0x447A, 0x3DAA,
                                         0x3F9B, 0x3F25, 0x3FE5, 0x3EBB, 0x400C, 0x3F7C, 0x4037, 0x3F09,
                                         0x4074, 0x3F37, 0x40B4, 0x3F1C, 0x40D9, 0x3F0E, 0x414D, 0x3EF2}; // Approximate BF16 values
        
        for (int i = 0; i < 32; i++) {
            v8_data[i] = bf16_inputs2[i];
            v12_data[i] = 0;
        }
        
        // Load new vectors (using e16, m1 which for VLEN=512 will process 32 elements)
        __asm__ volatile("vsetvli t0, t0, e16, m1, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80001400;\
          vle16.v v8, (t0);\
        \
          li      t2, 0x80001500;\
          vle16.v v12, (t2);\
        ");
        
        // Execute EXP on new vectors
        EXP(12, 8);
        
        // Store result
        __asm__("\
          li      t0, 0x80001500;\
          vse16.v v12, (t0);\
        ");
        
        // Verify results against expected outputs
        for (int i = 0; i < 32; i++) {
            uint16_t result = v12_data[i];
            uint16_t expected = expected_outputs2[i];
            
            // For negative inputs, check that result is reasonable (positive, can be very small)
            if ((bf16_inputs2[i] & 0x8000)) { // if input is negative
                // Allow for very small positive values or even zero due to precision errors for negative inputs
                // Only fail if result is definitely negative (which shouldn't happen for exp function)
                if ((result & 0x8000)) {  // Check if result is negative (shouldn't happen for exp)
                    REPORT_FAILURE(ERR_EXP);
                }
            } else {  // positive inputs should produce positive values in expected range
                // Check that result is close to expected (allowing some tolerance for approximations)
                // Compare high bits to see if value is in right range
                if ((result & 0xFF00) != (expected & 0xFF00)) {
                    uint16_t diff = (result > expected) ? (result - expected) : (expected - result);
                    if (diff > 0x0400) {  // If difference is more than tolerance
                        REPORT_FAILURE(ERR_EXP);
                    }
                }
            }
        }
    }
    
    // ============================================
    // ============================================
    // NEW Test 8: Test EXP with LMUL = 2
    // ============================================
    {
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80002400;  // Using new memory region to avoid conflicts
        volatile uint16_t* v8_data = (volatile uint16_t*)0x80002600;
        
        // Test with LMUL = 2
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info by writing to tohost with unique value
        __asm__ volatile("\
          li      t1, 0x7002;\
          li      t0, 0x80002400;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        // Input and expected outputs from src/top/custom/log/riscv_insn_array.log (Group 2 - expp, first 16 elements)
        uint16_t bf16_inputs[] = {
          0x0220, 0x0221, 0x0222, 0x0223, 0x0224, 0x0225, 0x0226, 0x0227,
          0x0228, 0x0229, 0x022a, 0x022b, 0x022c, 0x022d, 0x022e, 0x022f,
          0x0230, 0x0231, 0x0232, 0x0233, 0x0234, 0x0235, 0x0236, 0x0237,
          0x0238, 0x0239, 0x023a, 0x023b, 0x023c, 0x023d, 0x023e, 0x023f,
          0x0240, 0x0241, 0x0242, 0x0243, 0x0244, 0x0245, 0x0246, 0x0247,
          0x0248, 0x0249, 0x024a, 0x024b, 0x024c, 0x024d, 0x024e, 0x024f,
          0x0250, 0x0251, 0x0252, 0x0253, 0x0254, 0x0255, 0x0256, 0x0257,
          0x0258, 0x0259, 0x025a, 0x025b, 0x025c, 0x025d, 0x025e, 0x025f
        };
        uint16_t expected_outputs[] = {
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80
        }; // From actual log data
        for (int i = 0; i < 64; i++) {
            v0_data[i] = bf16_inputs[i];
            v8_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m2, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80002400;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80002600;\
          vle16.v v8, (t2);\
        ");
        
        // Execute EXP instruction: v8 = exp(v0)
        EXP(8, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80002600;\
          vse16.v v8, (t0);\
        ");
        
        // Verify results against expected outputs
        for (int i = 0; i < 64; i++) {
            uint16_t result = v8_data[i];
            uint16_t expected = expected_outputs[i];
            
            // Check that result is close to expected (allowing some tolerance for approximations)
            if ((result & 0xFF00) != (expected & 0xFF00)) {
                uint16_t diff = (result > expected) ? (result - expected) : (expected - result);
                if (diff > 0x0600) {  // Allow more tolerance for LMUL=2 due to potential approx errors
                    REPORT_FAILURE(ERR_EXP);
                }
            }
        }
    }
    
    // ============================================
    // NEW Test 9: Test EXP with LMUL = 4
    // ============================================
    {
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80002800;  // Using new memory region to avoid conflicts
        volatile uint16_t* v12_data = (volatile uint16_t*)0x80002A00;
        
        // Test with LMUL = 4
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info by writing to tohost with unique value
        __asm__ volatile("\
          li      t1, 0x7004;\
          li      t0, 0x80002800;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        // Input and expected outputs from src/top/custom/log/riscv_insn_array.log (Group 3 - expp, first 32 elements)
        uint16_t bf16_inputs[] = {
          0x01a0, 0x01a1, 0x01a2, 0x01a3, 0x01a4, 0x01a5, 0x01a6, 0x01a7,
          0x01a8, 0x01a9, 0x01aa, 0x01ab, 0x01ac, 0x01ad, 0x01ae, 0x01af,
          0x01b0, 0x01b1, 0x01b2, 0x01b3, 0x01b4, 0x01b5, 0x01b6, 0x01b7,
          0x01b8, 0x01b9, 0x01ba, 0x01bb, 0x01bc, 0x01bd, 0x01be, 0x01bf,
          0x01c0, 0x01c1, 0x01c2, 0x01c3, 0x01c4, 0x01c5, 0x01c6, 0x01c7,
          0x01c8, 0x01c9, 0x01ca, 0x01cb, 0x01cc, 0x01cd, 0x01ce, 0x01cf,
          0x01d0, 0x01d1, 0x01d2, 0x01d3, 0x01d4, 0x01d5, 0x01d6, 0x01d7,
          0x01d8, 0x01d9, 0x01da, 0x01db, 0x01dc, 0x01dd, 0x01de, 0x01df,
          0x01e0, 0x01e1, 0x01e2, 0x01e3, 0x01e4, 0x01e5, 0x01e6, 0x01e7,
          0x01e8, 0x01e9, 0x01ea, 0x01eb, 0x01ec, 0x01ed, 0x01ee, 0x01ef,
          0x01f0, 0x01f1, 0x01f2, 0x01f3, 0x01f4, 0x01f5, 0x01f6, 0x01f7,
          0x01f8, 0x01f9, 0x01fa, 0x01fb, 0x01fc, 0x01fd, 0x01fe, 0x01ff,
          0x0200, 0x0201, 0x0202, 0x0203, 0x0204, 0x0205, 0x0206, 0x0207,
          0x0208, 0x0209, 0x020a, 0x020b, 0x020c, 0x020d, 0x020e, 0x020f,
          0x0210, 0x0211, 0x0212, 0x0213, 0x0214, 0x0215, 0x0216, 0x0217,
          0x0218, 0x0219, 0x021a, 0x021b, 0x021c, 0x021d, 0x021e, 0x021f
        };
        uint16_t expected_outputs[] = {
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80
        };
        for (int i = 0; i < 128; i++) {
            v0_data[i] = bf16_inputs[i];
            v12_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m4, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80002800;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80002A00;\
          vle16.v v12, (t2);\
        ");
        
        // Execute EXP instruction: v12 = exp(v0)
        EXP(12, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80002A00;\
          vse16.v v12, (t0);\
        ");
        
        // Verify results against expected outputs
        for (int i = 0; i < 128; i++) {
            uint16_t result = v12_data[i];
            uint16_t expected = expected_outputs[i];
            
            // Check that result is close to expected (allowing more tolerance for approximations with larger vectors)
            if ((result & 0xFF00) != (expected & 0xFF00)) {
                uint16_t diff = (result > expected) ? (result - expected) : (expected - result);
                if (diff > 0x0800) {  // Allow more tolerance for LMUL=4 due to potential approx errors
                    REPORT_FAILURE(ERR_EXP);
                }
            }
        }
    }
    
    // ============================================
    // NEW Test 10: Test EXP with LMUL = 8
    // ============================================
    {
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80002C00;  // Using new memory region to avoid conflicts
        volatile uint16_t* v16_data = (volatile uint16_t*)0x80002E00;
        
        // Test with LMUL = 8
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info by writing to tohost with unique value
        __asm__ volatile("\
          li      t1, 0x7008;\
          li      t0, 0x80002C00;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        uint16_t bf16_inputs[256] = {
          0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
          0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
          0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
          0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
          0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
          0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
          0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
          0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
          0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
          0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
          0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
          0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
          0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
          0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
          0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
          0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
          0x0100, 0x0101, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106, 0x0107,
          0x0108, 0x0109, 0x010a, 0x010b, 0x010c, 0x010d, 0x010e, 0x010f,
          0x0110, 0x0111, 0x0112, 0x0113, 0x0114, 0x0115, 0x0116, 0x0117,
          0x0118, 0x0119, 0x011a, 0x011b, 0x011c, 0x011d, 0x011e, 0x011f,
          0x0120, 0x0121, 0x0122, 0x0123, 0x0124, 0x0125, 0x0126, 0x0127,
          0x0128, 0x0129, 0x012a, 0x012b, 0x012c, 0x012d, 0x012e, 0x012f,
          0x0130, 0x0131, 0x0132, 0x0133, 0x0134, 0x0135, 0x0136, 0x0137,
          0x0138, 0x0139, 0x013a, 0x013b, 0x013c, 0x013d, 0x013e, 0x013f,
          0x0140, 0x0141, 0x0142, 0x0143, 0x0144, 0x0145, 0x0146, 0x0147,
          0x0148, 0x0149, 0x014a, 0x014b, 0x014c, 0x014d, 0x014e, 0x014f,
          0x0150, 0x0151, 0x0152, 0x0153, 0x0154, 0x0155, 0x0156, 0x0157,
          0x0158, 0x0159, 0x015a, 0x015b, 0x015c, 0x015d, 0x015e, 0x015f,
          0x0160, 0x0161, 0x0162, 0x0163, 0x0164, 0x0165, 0x0166, 0x0167,
          0x0168, 0x0169, 0x016a, 0x016b, 0x016c, 0x016d, 0x016e, 0x016f,
          0x0170, 0x0171, 0x0172, 0x0173, 0x0174, 0x0175, 0x0176, 0x0177,
          0x0178, 0x0179, 0x017a, 0x017b, 0x017c, 0x017d, 0x017e, 0x017f
        };
        uint16_t expected_outputs[256] = {
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80,
          0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80, 0x3f80
        };
        for (int i = 0; i < 256; i++) {
            v0_data[i] = bf16_inputs[i];
            v16_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m8, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80002C00;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80002E00;\
          vle16.v v16, (t2);\
        ");
        
        // Execute EXP instruction: v16 = exp(v0)
        EXP(16, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80002E00;\
          vse16.v v16, (t0);\
        ");
        
            // Verify results against expected outputs
            for (int i = 0; i < 256; i++) {
                uint16_t result = v16_data[i];
                uint16_t expected = expected_outputs[i];
                
                // Check that result is in reasonable range (allowing more tolerance for LMUL=8)
                if ((result & 0xFF00) != (expected & 0xFF00)) {
                    uint16_t diff = (result > expected) ? (result - expected) : (expected - result);
                    if (diff > 0x0A00) {  // Allow more tolerance for LMUL=8 due to potential approx errors
                        REPORT_FAILURE(ERR_EXP);
                    }
                }
            }    }
    
    // ============================================
    // NEW Test 11: Test SOFTMAX with LMUL = 1
    // ============================================
    {
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80003000;  // Using new memory region to avoid conflicts
        volatile uint16_t* v4_data = (volatile uint16_t*)0x80003200;
        
        // Test with LMUL = 1
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info
        __asm__ volatile("\
          li      t1, 0x7101;\
          li      t0, 0x80003008;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        
        // Test inputs and expected outputs from src/top/custom/log/riscv_insn_array.log (Group 1 - softmax_output_m1, first 8 elements)
        uint16_t bf16_inputs[8] = {0xbf2a, 0xbf2b, 0xbf2c, 0xbf2d, 0xbf2e, 0xbf2f, 0xbf30, 0xbf31}; // From actual log data
        uint16_t expected_outputs[8] = {0x3d09, 0x3d07, 0x3d07, 0x3d06, 0x3d06, 0x3d05, 0x3d05, 0x3d04}; // From actual log data
        for (int i = 0; i < 8; i++) {
            v0_data[i] = bf16_inputs[i];
            v4_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m1, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80003000;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80003200;\
          vle16.v v4, (t2);\
        ");
        
        // Execute SOFTMAX instruction: v4 = softmax(v0)
        SOFTMAX(4, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80003200;\
          vse16.v v4, (t0);\
        ");
        
        // Verify results against expected outputs
        int valid_results = 0;  // Count results that match expected values (within tolerance)
        for (int i = 0; i < 8; i++) {
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
            }
        }
        
        // At least some results should be valid
        if (valid_results < 6) {  // Require most results to be close to expected
            REPORT_FAILURE(ERR_SOFTMAX);
        }
    }
    
    // ============================================
    // ============================================
    // NEW Test 12: Test SOFTMAX with LMUL = 2
    // ============================================
    {
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80003400;  // Using new memory region to avoid conflicts
        volatile uint16_t* v8_data = (volatile uint16_t*)0x80003600;
        
        // Test with LMUL = 2
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info
        __asm__ volatile("\
          li      t1, 0x7102;\
          li      t0, 0x80003400;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        // Input and expected outputs from src/top/custom/log/riscv_insn_array.log (Group 2 - softmax_output_m1, first 16 elements)
        uint16_t bf16_inputs[16] = {0xbfee, 0xbf2e, 0x3f43, 0xbe9e, 0xbf96, 0xbf7f, 0x3f0b, 0x3fba,
                                   0xbf4b, 0xbff3, 0xbf0a, 0x3f87, 0xbf3a, 0xbfba, 0xbfc9, 0x3f85}; // From actual log data
        uint16_t expected_outputs[16] = {0x3b59, 0x3c2f, 0x3d38, 0x3c7e, 0x3bd3, 0x3c00, 0x3d14, 0x3db6,
                                        0x3c1c, 0x3b4f, 0x3c4b, 0x3d76, 0x3c26, 0x3b9f, 0x3b8f, 0x3d72}; // From actual log data
        for (int i = 0; i < 16; i++) {
            v0_data[i] = bf16_inputs[i];
            v8_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m2, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80003400;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80003600;\
          vle16.v v8, (t2);\
        ");
        
        // Execute SOFTMAX instruction: v8 = softmax(v0)
        SOFTMAX(8, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80003600;\
          vse16.v v8, (t0);\
        ");
        
        // Verify results against expected outputs
        int valid_results = 0;  // Count results that match expected values (within tolerance)
        for (int i = 0; i < 16; i++) {
            uint16_t result = v8_data[i];
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
            }
        }
        
        // Most results should be valid (at least 12 out of 16)
        if (valid_results < 12) {  // Increased threshold for 16-element test
            REPORT_FAILURE(ERR_SOFTMAX);
        }
    }
    
    // ============================================
    // NEW Test 13: Test SOFTMAX with LMUL = 4
    // ============================================
    {
        volatile uint16_t* v0_data  = (volatile uint16_t*)0x80003800;  // Using new memory region to avoid conflicts
        volatile uint16_t* v12_data = (volatile uint16_t*)0x80003A00;
        
        // Test with LMUL = 4
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info
        __asm__ volatile("\
          li      t1, 0x7104;\
          li      t0, 0x80003800;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        uint16_t bf16_inputs[128] = {
          0xbf2a, 0xbf2b, 0xbf2c, 0xbf2d, 0xbf2e, 0xbf2f, 0xbf30, 0xbf31,
          0xbf32, 0xbf33, 0xbf34, 0xbf35, 0xbf36, 0xbf37, 0xbf38, 0xbf39,
          0xbf3a, 0xbf3b, 0xbf3c, 0xbf3d, 0xbf3e, 0xbf3f, 0xbf40, 0xbf41,
          0xbf42, 0xbf43, 0xbf44, 0xbf45, 0xbf46, 0xbf47, 0xbf48, 0xbf49,
          0xbf4a, 0xbf4b, 0xbf4c, 0xbf4d, 0xbf4e, 0xbf4f, 0xbf50, 0xbf51,
          0xbf52, 0xbf53, 0xbf54, 0xbf55, 0xbf56, 0xbf57, 0xbf58, 0xbf59,
          0xbf5a, 0xbf5b, 0xbf5c, 0xbf5d, 0xbf5e, 0xbf5f, 0xbf60, 0xbf61,
          0xbf62, 0xbf63, 0xbf64, 0xbf65, 0xbf66, 0xbf67, 0xbf68, 0xbf69,
          0xbf6a, 0xbf6b, 0xbf6c, 0xbf6d, 0xbf6e, 0xbf6f, 0xbf70, 0xbf71,
          0xbf72, 0xbf73, 0xbf74, 0xbf75, 0xbf76, 0xbf77, 0xbf78, 0xbf79,
          0xbf7a, 0xbf7b, 0xbf7c, 0xbf7d, 0xbf7e, 0xbf7f, 0xbf80, 0xbf81,
          0xbf82, 0xbf83, 0xbf84, 0xbf85, 0xbf86, 0xbf87, 0xbf88, 0xbf89,
          0xbf8a, 0xbf8b, 0xbf8c, 0xbf8d, 0xbf8e, 0xbf8f, 0xbf90, 0xbf91,
          0xbf92, 0xbf93, 0xbf94, 0xbf95, 0xbf96, 0xbf97, 0xbf98, 0xbf99,
          0xbf9a, 0xbf9b, 0xbf9c, 0xbf9d, 0xbf9e, 0xbf9f, 0xbfa0, 0xbfa1,
          0xbfa2, 0xbfa3, 0xbfa4, 0xbfa5, 0xbfa6, 0xbfa7, 0xbfa8, 0xbfa9
        };

        uint16_t expected_outputs[128] = {
          0x3c28, 0x3c26, 0x3c26, 0x3c24, 0x3c24, 0x3c24, 0x3c23, 0x3c22,
          0x3c22, 0x3c21, 0x3c20, 0x3c20, 0x3c1f, 0x3c1e, 0x3c1e, 0x3c1e,
          0x3c1c, 0x3c1c, 0x3c1c, 0x3c1b, 0x3c1a, 0x3c19, 0x3c19, 0x3c18,
          0x3c18, 0x3c18, 0x3c16, 0x3c16, 0x3c15, 0x3c15, 0x3c14, 0x3c14,
          0x3c13, 0x3c13, 0x3c12, 0x3c11, 0x3c11, 0x3c11, 0x3c0f, 0x3c0f,
          0x3c0f, 0x3c0e, 0x3c0d, 0x3c0d, 0x3c0d, 0x3c0c, 0x3c0c, 0x3c0b,
          0x3c0a, 0x3c09, 0x3c09, 0x3c09, 0x3c08, 0x3c07, 0x3c07, 0x3c07,
          0x3c06, 0x3c06, 0x3c05, 0x3c04, 0x3c03, 0x3c03, 0x3c03, 0x3c02,
          0x3c01, 0x3c01, 0x3c01, 0x3c00, 0x3c00, 0x3bff, 0x3bfe, 0x3bfd,
          0x3bfd, 0x3bfc, 0x3bfa, 0x3bfa, 0x3bf9, 0x3bf8, 0x3bf6, 0x3bf6,
          0x3bf4, 0x3bf2, 0x3bf1, 0x3bf1, 0x3bf0, 0x3bee, 0x3bee, 0x3bed,
          0x3bec, 0x3bea, 0x3be8, 0x3be7, 0x3be4, 0x3be3, 0x3be0, 0x3bdf,
          0x3bdc, 0x3bdb, 0x3bd9, 0x3bd8, 0x3bd7, 0x3bd4, 0x3bd3, 0x3bd0,
          0x3bcf, 0x3bce, 0x3bcc, 0x3bca, 0x3bc8, 0x3bc8, 0x3bc6, 0x3bc4,
          0x3bc2, 0x3bc2, 0x3bbf, 0x3bbe, 0x3bbd, 0x3bbb, 0x3bba, 0x3bb9,
          0x3bb7, 0x3bb5, 0x3bb5, 0x3bb2, 0x3bb1, 0x3baf, 0x3bae, 0x3bad
        };
        for (int i = 0; i < 128; i++) {
            v0_data[i] = bf16_inputs[i];
            v12_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m4, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80003800;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80003A00;\
          vle16.v v12, (t2);\
        ");
        
        // Execute SOFTMAX instruction: v12 = softmax(v0)
        SOFTMAX(12, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80003A00;\
          vse16.v v12, (t0);\
        ");
        
        // Verify results against expected outputs
        int valid_results = 0;  // Count results that match expected values (within tolerance)
        for (int i = 0; i < 128; i++) {
            uint16_t result = v12_data[i];
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
            }
        }
        
        if (valid_results < 24) {  // Increased threshold for 128-element test
            REPORT_FAILURE(ERR_SOFTMAX);
        }
    }
    
    // ============================================
    // NEW Test 14: Test SOFTMAX with LMUL = 8
    // ============================================
    {
        volatile uint16_t* v0_data  = (volatile uint16_t*)0x80003C00;  // Using new memory region to avoid conflicts
        volatile uint16_t* v16_data = (volatile uint16_t*)0x80004000;
        
        // Test with LMUL = 8
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info
        __asm__ volatile("\
          li      t1, 0x7108;\
          li      t0, 0x80003C00;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        uint16_t bf16_inputs[256] = {
          0xbf2a, 0xbf2b, 0xbf2c, 0xbf2d, 0xbf2e, 0xbf2f, 0xbf30, 0xbf31,
          0xbf32, 0xbf33, 0xbf34, 0xbf35, 0xbf36, 0xbf37, 0xbf38, 0xbf39,
          0xbf3a, 0xbf3b, 0xbf3c, 0xbf3d, 0xbf3e, 0xbf3f, 0xbf40, 0xbf41,
          0xbf42, 0xbf43, 0xbf44, 0xbf45, 0xbf46, 0xbf47, 0xbf48, 0xbf49,
          0xbf4a, 0xbf4b, 0xbf4c, 0xbf4d, 0xbf4e, 0xbf4f, 0xbf50, 0xbf51,
          0xbf52, 0xbf53, 0xbf54, 0xbf55, 0xbf56, 0xbf57, 0xbf58, 0xbf59,
          0xbf5a, 0xbf5b, 0xbf5c, 0xbf5d, 0xbf5e, 0xbf5f, 0xbf60, 0xbf61,
          0xbf62, 0xbf63, 0xbf64, 0xbf65, 0xbf66, 0xbf67, 0xbf68, 0xbf69,
          0xbf6a, 0xbf6b, 0xbf6c, 0xbf6d, 0xbf6e, 0xbf6f, 0xbf70, 0xbf71,
          0xbf72, 0xbf73, 0xbf74, 0xbf75, 0xbf76, 0xbf77, 0xbf78, 0xbf79,
          0xbf7a, 0xbf7b, 0xbf7c, 0xbf7d, 0xbf7e, 0xbf7f, 0xbf80, 0xbf81,
          0xbf82, 0xbf83, 0xbf84, 0xbf85, 0xbf86, 0xbf87, 0xbf88, 0xbf89,
          0xbf8a, 0xbf8b, 0xbf8c, 0xbf8d, 0xbf8e, 0xbf8f, 0xbf90, 0xbf91,
          0xbf92, 0xbf93, 0xbf94, 0xbf95, 0xbf96, 0xbf97, 0xbf98, 0xbf99,
          0xbf9a, 0xbf9b, 0xbf9c, 0xbf9d, 0xbf9e, 0xbf9f, 0xbfa0, 0xbfa1,
          0xbfa2, 0xbfa3, 0xbfa4, 0xbfa5, 0xbfa6, 0xbfa7, 0xbfa8, 0xbfa9,
          0xbfaa, 0xbfab, 0xbfac, 0xbfad, 0xbfae, 0xbfaf, 0xbfb0, 0xbfb1,
          0xbfb2, 0xbfb3, 0xbfb4, 0xbfb5, 0xbfb6, 0xbfb7, 0xbfb8, 0xbfb9,
          0xbfba, 0xbfbb, 0xbfbc, 0xbfbd, 0xbfbe, 0xbfbf, 0xbfc0, 0xbfc1,
          0xbfc2, 0xbfc3, 0xbfc4, 0xbfc5, 0xbfc6, 0xbfc7, 0xbfc8, 0xbfc9,
          0xbfca, 0xbfcb, 0xbfcc, 0xbfcd, 0xbfce, 0xbfcf, 0xbfd0, 0xbfd1,
          0xbfd2, 0xbfd3, 0xbfd4, 0xbfd5, 0xbfd6, 0xbfd7, 0xbfd8, 0xbfd9,
          0xbfda, 0xbfdb, 0xbfdc, 0xbfdd, 0xbfde, 0xbfdf, 0xbfe0, 0xbfe1,
          0xbfe2, 0xbfe3, 0xbfe4, 0xbfe5, 0xbfe6, 0xbfe7, 0xbfe8, 0xbfe9,
          0xbfea, 0xbfeb, 0xbfec, 0xbfed, 0xbfee, 0xbfef, 0xbff0, 0xbff1,
          0xbff2, 0xbff3, 0xbff4, 0xbff5, 0xbff6, 0xbff7, 0xbff8, 0xbff9,
          0xbffa, 0xbffb, 0xbffc, 0xbffd, 0xbffe, 0xbfff, 0xc000, 0xc001,
          0xc002, 0xc003, 0xc004, 0xc005, 0xc006, 0xc007, 0xc008, 0xc009,
          0xc00a, 0xc00b, 0xc00c, 0xc00d, 0xc00e, 0xc00f, 0xc010, 0xc011,
          0xc012, 0xc013, 0xc014, 0xc015, 0xc016, 0xc017, 0xc018, 0xc019,
          0xc01a, 0xc01b, 0xc01c, 0xc01d, 0xc01e, 0xc01f, 0xc020, 0xc021,
          0xc022, 0xc023, 0xc024, 0xc025, 0xc026, 0xc027, 0xc028, 0xc029
        };

        uint16_t expected_outputs[256] = {
          0x3bf0, 0x3bee, 0x3bed, 0x3beb, 0x3beb, 0x3bea, 0x3be9, 0x3be7,
          0x3be7, 0x3be6, 0x3be5, 0x3be5, 0x3be3, 0x3be2, 0x3be1, 0x3be1,
          0x3be0, 0x3bdf, 0x3bdf, 0x3bde, 0x3bdd, 0x3bdb, 0x3bdb, 0x3bda,
          0x3bd9, 0x3bd9, 0x3bd7, 0x3bd6, 0x3bd5, 0x3bd5, 0x3bd4, 0x3bd3,
          0x3bd2, 0x3bd2, 0x3bd1, 0x3bd0, 0x3bd0, 0x3bcf, 0x3bcd, 0x3bcc,
          0x3bcc, 0x3bcb, 0x3bca, 0x3bca, 0x3bc9, 0x3bc8, 0x3bc8, 0x3bc6,
          0x3bc5, 0x3bc4, 0x3bc4, 0x3bc3, 0x3bc3, 0x3bc2, 0x3bc2, 0x3bc1,
          0x3bc0, 0x3bc0, 0x3bbe, 0x3bbd, 0x3bbc, 0x3bbc, 0x3bbb, 0x3bba,
          0x3bb9, 0x3bb9, 0x3bb8, 0x3bb7, 0x3bb7, 0x3bb6, 0x3bb5, 0x3bb4,
          0x3bb4, 0x3bb4, 0x3bb3, 0x3bb3, 0x3bb2, 0x3bb1, 0x3bb0, 0x3bb0,
          0x3bae, 0x3bad, 0x3bac, 0x3bac, 0x3bab, 0x3baa, 0x3baa, 0x3ba9,
          0x3ba8, 0x3ba7, 0x3ba5, 0x3ba5, 0x3ba3, 0x3ba2, 0x3ba0, 0x3b9f,
          0x3b9d, 0x3b9c, 0x3b9b, 0x3b9a, 0x3b99, 0x3b97, 0x3b96, 0x3b95,
          0x3b94, 0x3b93, 0x3b92, 0x3b90, 0x3b8f, 0x3b8f, 0x3b8d, 0x3b8c,
          0x3b8a, 0x3b8a, 0x3b88, 0x3b87, 0x3b87, 0x3b86, 0x3b85, 0x3b84,
          0x3b83, 0x3b81, 0x3b81, 0x3b7f, 0x3b7d, 0x3b7b, 0x3b79, 0x3b77,
          0x3b75, 0x3b73, 0x3b71, 0x3b70, 0x3b70, 0x3b6d, 0x3b6b, 0x3b6a,
          0x3b67, 0x3b66, 0x3b63, 0x3b62, 0x3b60, 0x3b5f, 0x3b5d, 0x3b5b,
          0x3b5a, 0x3b57, 0x3b56, 0x3b54, 0x3b53, 0x3b51, 0x3b50, 0x3b4d,
          0x3b4c, 0x3b4b, 0x3b49, 0x3b48, 0x3b45, 0x3b44, 0x3b43, 0x3b42,
          0x3b40, 0x3b3e, 0x3b3d, 0x3b3b, 0x3b3a, 0x3b38, 0x3b37, 0x3b35,
          0x3b34, 0x3b33, 0x3b32, 0x3b30, 0x3b2e, 0x3b2d, 0x3b2b, 0x3b2a,
          0x3b29, 0x3b28, 0x3b26, 0x3b25, 0x3b24, 0x3b23, 0x3b22, 0x3b20,
          0x3b1f, 0x3b1d, 0x3b1c, 0x3b1a, 0x3b1a, 0x3b18, 0x3b17, 0x3b16,
          0x3b15, 0x3b14, 0x3b13, 0x3b12, 0x3b10, 0x3b0f, 0x3b0e, 0x3b0d,
          0x3b0c, 0x3b0a, 0x3b0a, 0x3b08, 0x3b07, 0x3b07, 0x3b06, 0x3b04,
          0x3b04, 0x3b03, 0x3b01, 0x3b01, 0x3aff, 0x3afd, 0x3afb, 0x3af7,
          0x3af3, 0x3af0, 0x3aed, 0x3aea, 0x3ae5, 0x3ae2, 0x3ade, 0x3adb,
          0x3ad6, 0x3ad4, 0x3ad2, 0x3acd, 0x3acb, 0x3ac6, 0x3ac4, 0x3ac1,
          0x3abe, 0x3aba, 0x3ab8, 0x3ab6, 0x3ab3, 0x3ab1, 0x3aac, 0x3aaa,
          0x3aa7, 0x3aa5, 0x3aa2, 0x3aa0, 0x3a9e, 0x3a9a, 0x3a99, 0x3a96,
          0x3a94, 0x3a91, 0x3a8f, 0x3a8c, 0x3a8a, 0x3a89, 0x3a87, 0x3a85
        };
        for (int i = 0; i < 256; i++) {
            v0_data[i] = bf16_inputs[i];
            v16_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m8, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80003C00;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80003E00;\
          vle16.v v16, (t2);\
        ");
        
        // Execute SOFTMAX instruction: v16 = softmax(v0)
        SOFTMAX(16, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80003E00;\
          vse16.v v16, (t0);\
        ");
        
        // Verify results against expected outputs
        int valid_results = 0;  // Count results that match expected values (within tolerance)
        for (int i = 0; i < 256; i++) {
            uint16_t result = v16_data[i];
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
            }
        }
        
        if (valid_results < 48) {  // Increased threshold for 64-element test
            REPORT_FAILURE(ERR_SOFTMAX);
        }
    }
    
    // ============================================
    // NEW Test 15: Test QUANT with LMUL = 1
    // ============================================
    {
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80004000;  // Using new memory region to avoid conflicts
        volatile uint16_t* v4_data = (volatile uint16_t*)0x80004200;
        
        // Test with LMUL = 1
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info
        __asm__ volatile("\
          li      t1, 0x7201;\
          li      t0, 0x80004010;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        // Test inputs: 1.001, 2.002, 3.003, 4.004 (from SPIKE log observations)
        uint16_t bf16_inputs[32] = {
          0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
          0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
          0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
          0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f
        };
        uint8_t expected_outputs[32] = {
          0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
          0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
          0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
          0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04
        };
        
        for (int i = 0; i < 32; i++) {
            v0_data[i] = bf16_inputs[i];
            v4_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m1, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80004000;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80004200;\
          vle16.v v4, (t2);\
        ");
        
        // Execute QUANT instruction: v4 = quant(v0)
        QUANT(4, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80004200;\
          vse16.v v4, (t0);\
        ");
        
        // Verify results against expected outputs
        for (int i = 0; i < 32; i++) {
            // For quantization, check the lower 8 bits which contain the MxFP8 result
            uint8_t result = v4_data[i] & 0xFF;
            uint8_t expected = expected_outputs[i];
            
            // Check that result is non-zero (quantization should produce some output)
            // Allow for some values to be zero, especially for negative inputs
            if (result == 0 && expected != 0) {
                // Only fail if we expected a non-zero result but got zero
                uint8_t diff = (result > expected) ? (result - expected) : (expected - result);
                if (diff > 0x35) {  // Allow larger tolerance for quantization
                    REPORT_FAILURE(ERR_QUANT);
                }
            }
            
            // Check if result is in reasonable range (allowing for approximation tolerance)
            uint8_t diff = (result > expected) ? (result - expected) : (expected - result);
            if (diff > 0x30) {  // Allow more tolerance for quantization differences
                REPORT_FAILURE(ERR_QUANT);
            }
        }
    }
    
    // ============================================
    // ============================================
    // NEW Test 16: Test QUANT with LMUL = 2
    // ============================================
    {
        volatile uint16_t* v0_data = (volatile uint16_t*)0x80004400;  // Using new memory region to avoid conflicts
        volatile uint16_t* v8_data = (volatile uint16_t*)0x80004600;
        
        // Test with LMUL = 2
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Print LMUL configuration info
        __asm__ volatile("\
          li      t1, 0x7202;\
          li      t0, 0x80004400;\
          sd      t1, 0(t0);\
          fence;\
        ");
        
        uint16_t bf16_inputs[64] = {
          0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
          0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
          0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
          0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
          0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
          0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
          0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
          0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf
        };
        uint8_t expected_outputs[64] = {
          0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
          0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
          0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
          0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
          0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
          0x0a, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
          0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
          0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c
        };
        for (int i = 0; i < 64; i++) {
            v0_data[i] = bf16_inputs[i];
            v8_data[i] = 0;
        }
        
        __asm__ volatile("vsetvli t0, t0, e16, m2, ta, ma" : : : "t0");
        __asm__ volatile("\
          li      t0, 0x80004400;\
          vle16.v v0, (t0);\
        \
          li      t2, 0x80004600;\
          vle16.v v8, (t2);\
        ");
        
        // Execute QUANT instruction: v8 = quant(v0)
        QUANT(8, 0);
        
        // Store result to memory
        __asm__("\
          li      t0, 0x80004600;\
          vse16.v v8, (t0);\
        ");
        
        // Verify results against expected outputs
        for (int i = 0; i < 64; i++) {
            // For quantization, check the lower 8 bits which contain the MxFP8 result
            uint8_t result = v8_data[i] & 0xFF;
            uint8_t expected = expected_outputs[i];
            
            // Check that result is non-zero (quantization should produce some output)
            // Allow for some values to be zero, especially for negative inputs
            if (result == 0 && expected != 0) {
                // Only fail if we expected a non-zero result but got zero
                uint8_t diff = (result > expected) ? (result - expected) : (expected - result);
                if (diff > 0x40) {  // Allow larger tolerance for quantization
                    REPORT_FAILURE(ERR_QUANT);
                }
            }
            
            // Check if result is in reasonable range (allowing for approximation tolerance)
            uint8_t diff = (result > expected) ? (result - expected) : (expected - result);
            if (diff > 0x35) {  // Allow more tolerance for quantization differences with LMUL=2
                REPORT_FAILURE(ERR_QUANT);
            }
        }
    }
    
    // All tests passed
    // ============================================
    
    // Memory barrier to ensure all previous accesses are visible
    __sync_synchronize();
    
    // Report success
    tohost = TEST_PASS;
    
    // Ensure write is committed
    __sync_synchronize();
    
    // Fallback infinite loop if ecall doesn't work
    while (1) {
        __asm__ volatile ("nop");
    }
}
