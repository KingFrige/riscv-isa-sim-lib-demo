#ifndef _FIRMWARE_H
#define _FIRMWARE_H

#include <stdint.h>

// Mailbox 寄存器定义
#define MAILBOX_BASE          0x60000000

// 寄存器偏移
#define MAILBOX_STATUS_OFFSET     0x0000
#define MAILBOX_COMMAND_OFFSET    0x0004
#define MAILBOX_DATA_ADDR_OFFSET  0x0008
#define MAILBOX_DATA_SIZE_OFFSET  0x0010
#define MAILBOX_RESPONSE_OFFSET   0x0014
#define MAILBOX_VECTOR_CONFIG_OFFSET 0x0018

// 状态寄存器位定义
#define MAILBOX_READY      0x00000001
#define MAILBOX_BUSY       0x00000002
#define MAILBOX_ERROR      0x00000004
#define MAILBOX_VECTOR_MODE 0x00000008

// 命令编码
#define MAILBOX_CMD_HELLO          0x00000001
#define MAILBOX_CMD_HI             0x00000002
#define MAILBOX_CMD_VECTOR_LOAD    0x00000010
#define MAILBOX_CMD_VECTOR_STORE   0x00000011
#define MAILBOX_CMD_VECTOR_COMPUTE 0x00000012
#define MAILBOX_CMD_SOFTMAX        0x00000020

// 错误码定义
#define MAILBOX_SUCCESS           0x00000000
#define MAILBOX_ERR_INVALID_CMD   0x00000001
#define MAILBOX_ERR_INVALID_PARAM 0x00000002
#define MAILBOX_ERR_MEM_ACCESS    0x00000003
#define MAILBOX_ERR_VECTOR_CONFIG 0x00000004
#define MAILBOX_ERR_NOT_IMPLEMENTED 0x00000005

// 函数声明
void firmware_main(void);
void handle_mailbox_command(void);
uint32_t read_mailbox_reg(uint32_t offset);
void write_mailbox_reg(uint32_t offset, uint32_t value);
void write_mailbox_reg64(uint32_t offset, uint64_t value);

// 命令处理函数
uint32_t handle_hello_command(void);
uint32_t handle_hi_command(void);
uint32_t handle_vector_load(uint64_t data_addr, uint32_t data_size, uint64_t vector_config);
uint32_t handle_vector_store(uint64_t data_addr, uint32_t data_size, uint64_t vector_config);
uint32_t handle_vector_compute(uint64_t data_addr, uint32_t data_size, uint64_t vector_config);
uint32_t handle_softmax_command(uint64_t data_addr, uint32_t data_size, uint64_t vector_config);

// 工具函数
void delay(uint32_t cycles);

// 包含util的printf实现
#include "../util/printf.h"

#endif // _FIRMWARE_H