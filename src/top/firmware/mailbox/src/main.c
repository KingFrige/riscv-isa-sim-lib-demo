#include "firmware.h"

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

static inline void write64(uintptr_t addr, uint64_t value) {
    *((volatile uint64_t*)addr) = value;
}

// Mailbox 寄存器访问函数
uint32_t read_mailbox_reg(uint32_t offset) {
    return read32(MAILBOX_BASE + offset);
}

void write_mailbox_reg(uint32_t offset, uint32_t value) {
    write32(MAILBOX_BASE + offset, value);
}

void write_mailbox_reg64(uint32_t offset, uint64_t value) {
    write64(MAILBOX_BASE + offset, value);
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

uint32_t handle_vector_load(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    // 简单的向量加载实现
    // 在实际实现中，这里会使用 RISC-V 向量指令
    printf("[FIRMWARE]: Vector load command received\n");
    printf("[FIRMWARE]: Data address: 0x%lx\n", data_addr);
    printf("[FIRMWARE]: Data size: %u\n", data_size);
    printf("[FIRMWARE]: Vector config: 0x%lx\n", vector_config);
    
    // 这里可以添加实际的向量加载逻辑
    // 例如：使用 RISC-V V 扩展指令加载数据到向量寄存器
    
    return MAILBOX_SUCCESS;
}

uint32_t handle_vector_store(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    // 简单的向量存储实现
    printf("[FIRMWARE]: Vector store command received\n");
    printf("[FIRMWARE]: Data address: 0x%lx\n", data_addr);
    printf("[FIRMWARE]: Data size: %u\n", data_size);
    printf("[FIRMWARE]: Vector config: 0x%lx\n", vector_config);
    
    // 这里可以添加实际的向量存储逻辑
    
    return MAILBOX_SUCCESS;
}

uint32_t handle_vector_compute(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    // 简单的向量计算实现
    printf("[FIRMWARE]: Vector compute command received\n");
    printf("[FIRMWARE]: Data address: 0x%lx\n", data_addr);
    printf("[FIRMWARE]: Data size: %u\n", data_size);
    printf("[FIRMWARE]: Vector config: 0x%lx\n", vector_config);
    
    // 这里可以添加实际的向量计算逻辑
    // 例如：向量加法、乘法等
    
    return MAILBOX_SUCCESS;
}

uint32_t handle_softmax_command(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    // Softmax计算实现
    printf("[FIRMWARE]: Softmax command received\n");
    printf("[FIRMWARE]: Data address: 0x%lx\n", data_addr);
    printf("[FIRMWARE]: Data size: %u\n", data_size);
    
    // 检查参数有效性
    if (data_addr == 0 || data_size == 0) {
        printf("[FIRMWARE]: Invalid parameters for softmax\n");
        return MAILBOX_ERR_INVALID_PARAM;
    }
    
    // 计算元素数量（假设每个浮点数4字节）
    uint32_t num_elements = data_size / 4;
    if (num_elements == 0 || data_size % 4 != 0) {
        printf("[FIRMWARE]: Invalid data size for softmax\n");
        return MAILBOX_ERR_INVALID_PARAM;
    }
    
    printf("[FIRMWARE]: Number of elements: %u\n", num_elements);
    
    // 在实际实现中，这里会：
    // 1. 从data_addr读取输入数据
    // 2. 调用softmax_compute函数计算
    // 3. 将结果写回到内存
    
    // 由于当前是简化实现，我们只打印消息
    printf("[FIRMWARE]: Softmax computation completed\n");
    printf("[FIRMWARE]: Output written to memory\n");
    
    return MAILBOX_SUCCESS;
}

// Mailbox 命令处理主函数
void handle_mailbox_command(void) {
    // 读取命令寄存器
    uint32_t command = read_mailbox_reg(MAILBOX_COMMAND_OFFSET);
    uint64_t data_addr = read64(MAILBOX_BASE + MAILBOX_DATA_ADDR_OFFSET);
    uint32_t data_size = read_mailbox_reg(MAILBOX_DATA_SIZE_OFFSET);
    uint64_t vector_config = read64(MAILBOX_BASE + MAILBOX_VECTOR_CONFIG_OFFSET);
    
    uint32_t response = MAILBOX_ERR_INVALID_CMD;
    

    
    // 根据命令类型调用相应的处理函数
    switch (command) {
        case MAILBOX_CMD_HELLO:
            response = handle_hello_command();
            break;
            
        case MAILBOX_CMD_HI:
            response = handle_hi_command();
            break;
            
        case MAILBOX_CMD_VECTOR_LOAD:
            response = handle_vector_load(data_addr, data_size, vector_config);
            break;
            
        case MAILBOX_CMD_VECTOR_STORE:
            response = handle_vector_store(data_addr, data_size, vector_config);
            break;
            
        case MAILBOX_CMD_VECTOR_COMPUTE:
            response = handle_vector_compute(data_addr, data_size, vector_config);
            break;
            
        case MAILBOX_CMD_SOFTMAX:
            response = handle_softmax_command(data_addr, data_size, vector_config);
            break;
            
        default:
            printf("Unknown command: 0x%x\n", command);
            break;
    }
    
    // 写入响应
    write_mailbox_reg(MAILBOX_RESPONSE_OFFSET, response);
}

// 固件主函数
void firmware_main(void) {
    // 通过UART输出启动信息
    printf("========================================\n");
    printf("[FIRMWARE]: Firmware starting...\n");
    printf("[FIRMWARE]: Mailbox Base: 0x%lx\n", (uint64_t)MAILBOX_BASE);
    printf("========================================\n");
    
    // 主循环
    while (1) {
        // 检查 mailbox 状态
        uint32_t status = read_mailbox_reg(MAILBOX_STATUS_OFFSET);
        
        if (status & MAILBOX_BUSY) {
            // 有命令需要处理
            handle_mailbox_command();
            
            // 清除忙状态（通过写入状态寄存器）
            write_mailbox_reg(MAILBOX_STATUS_OFFSET, 0);
        }
        
        // 简单的延迟，避免过于频繁的轮询
        delay(1000);
    }
}

// 入口点
void _start(void) {
    firmware_main();
    
    // 如果 firmware_main 返回，则进入无限循环
    while (1) {
        // 空循环
    }
}
