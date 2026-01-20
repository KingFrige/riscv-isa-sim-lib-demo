/**
 * @file csr_addr.h
 * @brief CSR 寄存器地址定义
 * 
 * 定义所有自定义 CSR 寄存器的地址，供 firmware 和 extensions 共享使用。
 * CSR 地址范围：0xBC0 - 0xBC8
 */

#ifndef _CSR_ADDR_H
#define _CSR_ADDR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CSR 地址定义 (0xBC0 - 0xBC8)
// ============================================================================

// Mail 通道寄存器
#define CSR_MAIL_DATA0     0xBC0  // Mail 数据寄存器 0 (64-bit)
#define CSR_MAIL_DATA1     0xBC1  // Mail 数据寄存器 1 (64-bit)
#define CSR_MAIL_DATA2     0xBC2  // Mail 数据寄存器 2 (64-bit)
#define CSR_MAIL_DATA3     0xBC3  // Mail 数据寄存器 3 (64-bit)
#define CSR_MAIL_VALID     0xBC4  // Mail 有效标志 (1-bit)

// Bo done 通道寄存器
#define CSR_BO_DONE        0xBC5  // Bo done 信号 (11-bit)

// Se up 通道寄存器
#define CSR_SE_UP          0xBC6  // Se up 信号 (11-bit)

// Se query 通道寄存器
#define CSR_SE_QUERY_LOCK  0xBC7  // Se query lock 信号 (11-bit)
#define CSR_SE_QUERY_COUNT 0xBC8  // Se query 计数器 (64-bit)

// ============================================================================
// CSR 位字段定义
// ============================================================================

// BO_DONE 位字段
#define CSR_BO_DONE_WG_INDEX_SHIFT 6   // wg_index 起始位
#define CSR_BO_DONE_WG_INDEX_MASK  0x7C0  // wg_index 掩码 [10:6]
#define CSR_BO_DONE_BAR_INDEX_MASK 0x3F  // bar_index 掩码 [5:0]

// SE_UP 位字段
#define CSR_SE_UP_WG_INDEX_SHIFT 6   // wg_index 起始位
#define CSR_SE_UP_WG_INDEX_MASK  0x7C0  // wg_index 掩码 [10:6]
#define CSR_SE_UP_BAR_INDEX_MASK 0x3F  // bar_index 掩码 [5:0]

// SE_QUERY_LOCK 位字段
#define CSR_SE_QUERY_LOCK_WG_INDEX_SHIFT 6   // wg_index 起始位
#define CSR_SE_QUERY_LOCK_WG_INDEX_MASK  0x7C0  // wg_index 掩码 [10:6]
#define CSR_SE_QUERY_LOCK_BAR_INDEX_MASK 0x3F  // bar_index 掩码 [5:0]

// MAIL_VALID 位字段
#define CSR_MAIL_VALID_VALID 0x1  // valid 标志位 [0]

#ifdef __cplusplus
}
#endif

#endif // _CSR_ADDR_H