/* mailbox.c - Mailbox device access functions
 * 按 rvv_mailbox_dev.md 设计
 * 地址 0x0000: STATUS (32位)
 * 地址 0x0008: COMMAND (32位)
 * 地址 0x000C: RESPONSE (32位)
 * 地址 0x0020-0x0038: DATA0-3 (64位)
 */
#include "mailbox.h"

// Mailbox 设备状态 (按 rvv_mailbox_dev.md 设计)
typedef struct {
    volatile uint32_t status;              // 0x0000
    volatile uint8_t  reserved0[4];        // padding: 0x0004-0x0007
    volatile uint32_t command;             // 0x0008
    volatile uint32_t response;            // 0x000C
    volatile uint8_t  reserved1[16];       // padding: 0x0010-0x001F
    volatile uint64_t data0;               // 0x0020
    volatile uint64_t data1;               // 0x0028
    volatile uint64_t data2;               // 0x0030
    volatile uint64_t data3;               // 0x0038
} mailbox_regs_t;

static mailbox_regs_t* mailbox = (mailbox_regs_t*)MAILBOX_BASE;

// Mailbox 寄存器访问函数
uint32_t mailbox_read_status(void) {
    return mailbox->status;
}

uint32_t mailbox_read_command(void) {
    return mailbox->command;
}

// 新接口：DATA0-3 访问函数
uint64_t mailbox_read_data0(void) {
    return mailbox->data0;
}

uint64_t mailbox_read_data1(void) {
    return mailbox->data1;
}

uint64_t mailbox_read_data2(void) {
    return mailbox->data2;
}

uint64_t mailbox_read_data3(void) {
    return mailbox->data3;
}

void mailbox_write_response(uint32_t value) {
    mailbox->response = value;
}

void mailbox_clear_busy(void) {
    mailbox->status = MAILBOX_READY;
}

int mailbox_has_command(void) {
    return (mailbox_read_status() & MAILBOX_BUSY) != 0;
}

void mailbox_send_response(uint32_t response) {
    mailbox_write_response(response);
    mailbox_clear_busy();
}
