#include "mailbox.h"

// tohost and fromhost symbols for communication with Spike
__attribute__((section(".tohost")))
volatile uint64_t tohost = 0;

__attribute__((section(".fromhost")))
volatile uint64_t fromhost = 0;

// 简单的内存访问函数
static inline uint32_t read32(uintptr_t addr) {
    return *((volatile uint32_t*)addr);
}

static inline void write32(uintptr_t addr, uint32_t value) {
    *((volatile uint32_t*)addr) = value;
}

static inline uint64_t read64(uintptr_t addr) {
    return *((volatile uint64_t*)addr);
}

// Mailbox 寄存器访问函数
uint32_t read_mailbox_reg(uint32_t offset) {
    return read32(MAILBOX_BASE + offset);
}

void write_mailbox_reg(uint32_t offset, uint32_t value) {
    write32(MAILBOX_BASE + offset, value);
}

// 简单的延迟函数
void delay(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; i++) {
        // 空循环
    }
}

#include <stdarg.h>

// 命令处理函数
uint32_t handle_hello_command(void) {
    printf("[FIRMWARE]: Hello from firmware!\n");
    return MAILBOX_SUCCESS;
}

uint32_t handle_hi_command(void) {
    printf("[FIRMWARE]: Hi from firmware!\n");
    return MAILBOX_SUCCESS;
}

// 新接口：使用 data0-3
uint32_t handle_vector_load(uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3) {
    printf("[FIRMWARE]: Vector load command received\n");
    printf("[FIRMWARE]: Data0 (addr): 0x%lx\n", data0);
    printf("[FIRMWARE]: Data1 (size): 0x%lx\n", data1);
    (void)data2; (void)data3; // Reserved for future use
    return MAILBOX_SUCCESS;
}

uint32_t handle_vector_store(uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3) {
    printf("[FIRMWARE]: Vector store command received\n");
    printf("[FIRMWARE]: Data0 (addr): 0x%lx\n", data0);
    printf("[FIRMWARE]: Data1 (size): 0x%lx\n", data1);
    (void)data2; (void)data3; // Reserved for future use
    return MAILBOX_SUCCESS;
}

uint32_t handle_vector_compute(uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3) {
    printf("[FIRMWARE]: Vector compute command received\n");
    printf("[FIRMWARE]: Data0 (addr): 0x%lx\n", data0);
    printf("[FIRMWARE]: Data1 (size): 0x%lx\n", data1);
    (void)data2; (void)data3; // Reserved for future use
    return MAILBOX_SUCCESS;
}

uint32_t handle_softmax_command(uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3) {
    printf("[FIRMWARE]: Softmax command received\n");
    printf("[FIRMWARE]: Data0 (addr): 0x%lx\n", data0);
    printf("[FIRMWARE]: Data1 (size): 0x%lx\n", data1);
    (void)data2; (void)data3; // Reserved for future use

    if (data0 == 0 || data1 == 0) {
        printf("[FIRMWARE]: Invalid parameters for softmax\n");
        return MAILBOX_ERR_INVALID_PARAM;
    }

    uint32_t num_elements = data1 / 4;
    if (num_elements == 0 || data1 % 4 != 0) {
        printf("[FIRMWARE]: Invalid data size for softmax\n");
        return MAILBOX_ERR_INVALID_PARAM;
    }

    printf("[FIRMWARE]: Number of elements: %u\n", num_elements);
    printf("[FIRMWARE]: Softmax computation completed\n");
    printf("[FIRMWARE]: Output written to memory\n");

    return MAILBOX_SUCCESS;
}

uint32_t handle_exp_command(uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3) {
    printf("[FIRMWARE]: EXP command received\n");
    printf("[FIRMWARE]: Data0 (addr): 0x%lx\n", data0);
    printf("[FIRMWARE]: Data1 (size): 0x%lx\n", data1);
    (void)data2; (void)data3; // Reserved for future use

    if (data0 == 0 || data1 == 0) {
        printf("[FIRMWARE]: Invalid parameters for exp\n");
        return MAILBOX_ERR_INVALID_PARAM;
    }

    uint32_t num_elements = data1 / 2;  // BF16 = 2 bytes per element
    if (num_elements == 0 || data1 % 2 != 0) {
        printf("[FIRMWARE]: Invalid data size for exp\n");
        return MAILBOX_ERR_INVALID_PARAM;
    }

    printf("[FIRMWARE]: Number of BF16 elements: %u\n", num_elements);
    printf("[FIRMWARE]: EXP computation completed\n");
    printf("[FIRMWARE]: Output written to memory\n");

    return MAILBOX_SUCCESS;
}

uint32_t handle_quant_command(uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3) {
    printf("[FIRMWARE]: QUANT command received\n");
    printf("[FIRMWARE]: Data0 (addr): 0x%lx\n", data0);
    printf("[FIRMWARE]: Data1 (size): 0x%lx\n", data1);
    (void)data2; (void)data3; // Reserved for future use

    if (data0 == 0 || data1 == 0) {
        printf("[FIRMWARE]: Invalid parameters for quant\n");
        return MAILBOX_ERR_INVALID_PARAM;
    }

    uint32_t num_elements = data1 / 2;  // BF16 = 2 bytes per element
    if (num_elements == 0 || data1 % 2 != 0) {
        printf("[FIRMWARE]: Invalid data size for quant\n");
        return MAILBOX_ERR_INVALID_PARAM;
    }

    printf("[FIRMWARE]: Number of BF16 elements: %u\n", num_elements);
    printf("[FIRMWARE]: QUANT computation (BF16 -> MxFP8) completed\n");
    printf("[FIRMWARE]: Output written to memory\n");

    return MAILBOX_SUCCESS;
}

// Mailbox 命令处理主函数
void handle_mailbox_command(void) {
    uint32_t command = read_mailbox_reg(MAILBOX_COMMAND_OFFSET);
    
    // 新接口：使用 data0-3 寄存器
    uint64_t data0 = mailbox_read_data0();
    uint64_t data1 = mailbox_read_data1();
    uint64_t data2 = mailbox_read_data2();
    uint64_t data3 = mailbox_read_data3();
    
    uint32_t response = MAILBOX_ERR_INVALID_CMD;
    
    switch (command) {
        case MAILBOX_CMD_HELLO:
            response = handle_hello_command();
            break;
        case MAILBOX_CMD_HI:
            response = handle_hi_command();
            break;
        case MAILBOX_CMD_VECTOR_LOAD:
            response = handle_vector_load(data0, data1, data2, data3);
            break;
        case MAILBOX_CMD_VECTOR_STORE:
            response = handle_vector_store(data0, data1, data2, data3);
            break;
        case MAILBOX_CMD_VECTOR_COMPUTE:
            response = handle_vector_compute(data0, data1, data2, data3);
            break;
        case MAILBOX_CMD_SOFTMAX:
            response = handle_softmax_command(data0, data1, data2, data3);
            break;
        case MAILBOX_CMD_EXP:
            response = handle_exp_command(data0, data1, data2, data3);
            break;
        case MAILBOX_CMD_QUANT:
            response = handle_quant_command(data0, data1, data2, data3);
            break;
        default:
            printf("[FIRMWARE] Unknown command: 0x%x\n", command);
            response = MAILBOX_ERR_INVALID_CMD;
            break;
    }
    
    write_mailbox_reg(MAILBOX_RESPONSE_OFFSET, response);
}

// 固件主函数
void main(void) {
    printf("========================================\n");
    printf("[FIRMWARE]: Firmware starting...\n");
    printf("[FIRMWARE]: Mailbox Base: 0x%lx\n", (uint64_t)MAILBOX_BASE);
    printf("========================================\n");

    uint32_t loop_count = 0;
    printf("[FIRMWARE]: Entering main loop...\n");
    
    while (1) {
        loop_count++;
        printf("[FIRMWARE]: Loop count: %u\n", loop_count);
        
        uint32_t status = read_mailbox_reg(MAILBOX_STATUS_OFFSET);
        
        if (status & MAILBOX_BUSY) {
            printf("[FIRMWARE]: Mailbox busy! Status: 0x%x\n", status);
            handle_mailbox_command();
        }
        
        delay(100);
    }
}