/* mailbox.c - Mailbox device access functions
 * 所有函数保留供将来使用，具体使用可在 main.c 中添加
 */
#include "mailbox.h"

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

// Mailbox 寄存器访问函数
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

uint64_t mailbox_read_vector_config(void) {
    return mailbox->vector_config;
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