#include <riscv_vector.h>
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

// XPERIB scalar multiplication extension (opcode 0x4b - CUSTOM-2)
#define PERIBMUL(rd, rs1, rs2) __asm__ volatile (".word %0" : : "i"(((rs1) << 15) | ((rs2) << 20) | ((rd) << 7) | 0x4b) : "memory")

// XPERIVMUL vector multiplication extension (opcode 0x5b - CUSTOM-3)
#define PERIVMUL(vd, vs1, vs2) __asm__ volatile (".word %0" : : "i"(((vs1) << 15) | ((vs2) << 20) | ((vd) << 7) | (1 << 25) | 0x5b) : "memory")


// #define PERIAADD(rd, rs1, rs2) __asm__ volatile (".word ((" _XSTR(rs1) " << " _XSTR(OPCODE_RS1_SHIFT_VAL) ") | (" _XSTR(rs2) " << " _XSTR(OPCODE_RS2_SHIFT_VAL) ") | (" _XSTR(rd) " << " _XSTR(OPCODE_DS_SHIFT_VAL) ") | CUSTOM0)" : : : "memory")
// unsigned volatile * const p_finisher = (unsigned *) (FINISHER_BASE + 8);

// #define PERIVADD(vd, vs1, vs2) __asm__ volatile (".word ((" _XSTR(vs1) " << " _XSTR(OPCODE_RS1_SHIFT_VAL) ") | (" _XSTR(vs2) " << " _XSTR(OPCODE_RS2_SHIFT_VAL) ") | (" _XSTR(vd) " << " _XSTR(OPCODE_DS_SHIFT_VAL) ") | CUSTOM1)" : : : "memory")



// Test result codes
#define TEST_PASS 1
#define TEST_FAIL 0xFF

// Error codes for different test failures
#define ERR_XPERIA_ADD 0x10
#define ERR_XPERIB_MUL 0x20
#define ERR_XPERIV_ADD 0x30
#define ERR_XPERIV_MUL 0x40

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
    // Test 2: XPERIB scalar multiplication extension
    // ============================================
    {
        // Set up test values
        long a4 = 7;
        long a5 = 6;
        long a3 = 0;
        
        // Use inline assembly to set registers
        __asm__ volatile (
            "mv a4, %0\n\t"
            "mv a5, %1\n\t"
            : // no outputs
            : "r"(a4), "r"(a5)
            : "a4", "a5"
        );
        
        // Execute XPERIB multiplication: a3 = a4 * a5
        PERIBMUL(13, 14, 15);  // a3 = a4 * a5
        
        // Read result
        __asm__ volatile ("mv %0, a3" : "=r"(a3));
        
        // Verify result: 7 * 6 = 42
        if (a3 != 42) {
            REPORT_FAILURE(ERR_XPERIB_MUL);
        }
    }
    
    // ============================================
    // Test 3 & 4: Vector extensions
    // ============================================
    {
        // Vector test data at fixed addresses
        volatile uint32_t* v0_data = (volatile uint32_t*)0x80001000;
        volatile uint32_t* v2_data = (volatile uint32_t*)0x80001100;
        volatile uint32_t* v4_data = (volatile uint32_t*)0x80001200;
        volatile uint32_t* v6_data = (volatile uint32_t*)0x80001300;
        
        // Initialize test vectors
        for (int i = 0; i < 4; i++) {
            v0_data[i] = i + 1;      // [1, 2, 3, 4]
            v2_data[i] = i + 5;      // [5, 6, 7, 8]
            v4_data[i] = 0;
            v6_data[i] = 0;
        }
        
        // Enable vector extension and load vectors
        __asm__ volatile("\
          li      t0, 0x600;\
          csrs    mstatus, t0;\
        ");
        
        // Set vector length to 4 elements, e32, m1
        __asm__ volatile(".word 0x0112f0d7" : : : "t0");  // vsetvli t0, t0, e32, m1, ta, ma
        
        __asm__ volatile("\
          li      t0, 0x80001000;\
          vle32.v v0, (t0);\
        \
          li      t2, 0x80001100;\
          vle32.v v2, (t2);\
        ");
        
        // Test 3: XPERIV vector addition extension
        // PERIVADD(4, 0, 2);     // v4 = v0 + v2
        // Use the macro instead of hardcoded instruction
        PERIVADD(4, 0, 2);
        
        // Store v4 to memory for verification
        __asm__("\
          li      t0, 0x80001200;\
          vse32.v v4, (t0);\
        ");
        
        // Verify vector addition results
        // Expected: v4[i] = v0[i] + v2[i]
        // [1+5=6, 2+6=8, 3+7=10, 4+8=12]
        uint32_t expected_add[4] = {6, 8, 10, 12};
        for (int i = 0; i < 4; i++) {
            if (v4_data[i] != expected_add[i]) {
                REPORT_FAILURE(ERR_XPERIV_ADD);
            }
        }
        
        // Test 4: XPERIVMUL vector multiplication extension
        PERIVMUL(6, 0, 2);     // v6 = v0 * v2
        
        // Store v6 to memory for verification
        __asm__("\
          li      t0, 0x80001300;\
          vse32.v v6, (t0);\
        ");
        
        // Verify vector multiplication results
        // Expected: v6[i] = v0[i] * v2[i]
        // [1*5=5, 2*6=12, 3*7=21, 4*8=32]
        uint32_t expected_mul[4] = {5, 12, 21, 32};
        for (int i = 0; i < 4; i++) {
            if (v6_data[i] != expected_mul[i]) {
                REPORT_FAILURE(ERR_XPERIV_MUL);
            }
        }
    }
    
    // ============================================
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
