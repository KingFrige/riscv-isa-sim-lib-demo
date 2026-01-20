/**
 * @file mail_poll.c
 * @brief Mail 通道轮询测试
 * 
 * 实现 Mail 通道的轮询功能：
 * - 轮询 MAIL_VALID 寄存器
 * - 当 valid 为高时，读取 MAIL_DATA0-3
 * - 读取完成后清除 valid
 */

#include <stdint.h>
#include <stdbool.h>
#include "printf.h"
#include "../../common/csr_addr.h"

// 读取 CSR 寄存器
static inline uint64_t csr_read(uint64_t csr) {
  uint64_t value;
  asm volatile ("csrr %0, %1" : "=r"(value) : "i"(csr));
  return value;
}

// 写入 CSR 寄存器
static inline void csr_write(uint64_t csr, uint64_t value) {
  asm volatile ("csrw %0, %1" :: "i"(csr), "r"(value));
}

/**
 * @brief 轮询并读取 Mail 数据
 * 
 * @return true 成功读取数据，false 超时
 */
bool poll_and_read_mail(void) {
    uint32_t timeout = 1000000;  // 1秒超时
    uint32_t count = 0;
    
    printf("[FIRMWARE] Polling MAIL_VALID...\n");
    
    // 轮询 MAIL_VALID 寄存器
    while (count < timeout) {
        uint64_t valid = csr_read(CSR_MAIL_VALID);
        
        if (valid & 0x1) {
            // Mail 有效，读取数据
            printf("[FIRMWARE] Mail valid detected! Reading data...\n");
            
            uint64_t data0 = csr_read(CSR_MAIL_DATA0);
            uint64_t data1 = csr_read(CSR_MAIL_DATA1);
            uint64_t data2 = csr_read(CSR_MAIL_DATA2);
            uint64_t data3 = csr_read(CSR_MAIL_DATA3);
            
            // 输出读取的数据
            printf("[FIRMWARE] Received mail data:\n");
            printf("[FIRMWARE]   DATA0: 0x%016lx\n", data0);
            printf("[FIRMWARE]   DATA1: 0x%016lx\n", data1);
            printf("[FIRMWARE]   DATA2: 0x%016lx\n", data2);
            printf("[FIRMWARE]   DATA3: 0x%016lx\n", data3);
            
            // 清除 valid
            printf("[FIRMWARE] Clearing MAIL_VALID...\n");
            csr_write(CSR_MAIL_VALID, 0);
            
            return true;
        }
        
        // 短暂延迟
        for (volatile uint32_t i = 0; i < 1000; i++);
        count++;
    }
    
    printf("[FIRMWARE] Timeout waiting for mail!\n");
    return false;
}

/**
 * @brief 测试 Mail 轮询功能
 */
void test_mail_poll(void) {
    printf("\n--- Testing Mail Poll ---\n");
    
    // 等待一段时间，让主机写入数据
    for (volatile uint32_t i = 0; i < 10000000; i++);
    
    // 轮询并读取 Mail
    if (poll_and_read_mail()) {
        printf("[SUCCESS] Mail poll test passed!\n");
    } else {
        printf("[ERROR] Mail poll test failed!\n");
    }
}
