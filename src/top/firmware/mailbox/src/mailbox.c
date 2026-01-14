#include "firmware.h"
#include <stddef.h>

// Mailbox 设备状态
typedef struct {
    volatile uint32_t status;
    volatile uint32_t command;
    volatile uint64_t data_addr;
    volatile uint32_t data_size;
    volatile uint32_t response;
    volatile uint64_t vector_config;
} mailbox_regs_t;

static mailbox_regs_t* mailbox = (mailbox_regs_t*)MAILBOX_BASE;

// 增强的 Mailbox 寄存器访问函数
uint32_t mailbox_read_status(void) {
    return mailbox->status;
}

uint32_t mailbox_read_command(void) {
    return mailbox->command;
}

uint64_t mailbox_read_data_addr(void) {
    return mailbox->data_addr;
}

uint32_t mailbox_read_data_size(void) {
    return mailbox->data_size;
}

uint32_t mailbox_read_response(void) {
    return mailbox->response;
}

uint64_t mailbox_read_vector_config(void) {
    return mailbox->vector_config;
}

void mailbox_write_response(uint32_t value) {
    mailbox->response = value;
}

void mailbox_clear_busy(void) {
    // 清除 BUSY 位，设置 READY 位
    mailbox->status = MAILBOX_READY;
}

// 检查是否有待处理的命令
int mailbox_has_command(void) {
    return (mailbox_read_status() & MAILBOX_BUSY) != 0;
}

// 等待 mailbox 就绪
void mailbox_wait_ready(void) {
    while (!(mailbox_read_status() & MAILBOX_READY)) {
        // 忙等待
    }
}

// 发送命令响应
void mailbox_send_response(uint32_t response) {
    mailbox_write_response(response);
    mailbox_clear_busy();
}

// 解析向量配置
typedef struct {
    uint8_t sew;    // 元素宽度 (0:8位, 1:16位, 2:32位, 3:64位)
    uint8_t lmul;   // 向量长度乘数 (0:1, 1:2, 2:4, 3:8)
    uint32_t vlen;  // 向量寄存器长度（位）
} vector_config_t;

vector_config_t parse_vector_config(uint64_t config) {
    vector_config_t vc;
    vc.sew = (config >> 0) & 0x7;
    vc.lmul = (config >> 4) & 0x7;
    vc.vlen = (config >> 32) & 0xFFFFFFFF;
    return vc;
}

// 计算向量元素大小（字节）
uint32_t get_element_size(uint8_t sew) {
    switch (sew) {
        case 0: return 1;  // 8位
        case 1: return 2;  // 16位
        case 2: return 4;  // 32位
        case 3: return 8;  // 64位
        default: return 0;
    }
}

// 验证向量配置
int validate_vector_config(vector_config_t vc) {
    if (vc.sew > 3) {
        return 0;  // 无效的 SEW
    }
    if (vc.lmul > 3) {
        return 0;  // 无效的 LMUL
    }
    if (vc.vlen == 0) {
        return 0;  // 无效的 VLEN
    }
    return 1;
}

// 内存复制函数（简单的字节复制）
void memory_copy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

// 内存设置函数
void memory_set(void* dest, uint8_t value, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = value;
    }
}

// 验证内存地址范围
int validate_memory_range(uint64_t addr, uint32_t size) {
    // 简单的验证：确保地址和大小不会溢出
    // 在实际系统中，可能需要更复杂的地址范围检查
    if (addr + size < addr) {
        return 0;  // 地址溢出
    }
    
    // 这里可以添加更多的地址范围检查
    // 例如：检查地址是否在有效的内存范围内
    
    return 1;
}