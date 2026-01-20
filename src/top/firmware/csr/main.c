#include <riscv_vector.h>
#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "../../common/csr_addr.h"

// 错误代码定义
#define ERR_CSR_READ        0xE1
#define ERR_CSR_WRITE       0xE2
#define ERR_CSR_MAIL_DATA   0xE3
#define ERR_CSR_VALID       0xE4
#define ERR_CSR_BO_DONE     0xE5
#define ERR_CSR_SE_UP       0xE6
#define ERR_CSR_SE_LOCK     0xE7
#define ERR_CSR_SE_COUNT    0xE8

// 错误代码定义
#define ERR_CSR_READ        0xE1
#define ERR_CSR_WRITE       0xE2
#define ERR_CSR_MAIL_DATA   0xE3
#define ERR_CSR_VALID       0xE4
#define ERR_CSR_BO_DONE     0xE5
#define ERR_CSR_SE_UP       0xE6
#define ERR_CSR_SE_LOCK     0xE7
#define ERR_CSR_SE_COUNT    0xE8

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


// 读取 CSR 寄存器 - 使用标准内联汇编（参考 Spike 示例）
static inline uint64_t csr_read(uint64_t csr) {
  uint64_t value;
  asm volatile ("csrr %0, %1" : "=r"(value) : "i"(csr));
  return value;
}

// 写入 CSR 寄存器 - 使用标准内联汇编
static inline void csr_write(uint64_t csr, uint64_t value) {
  asm volatile ("csrw %0, %1" :: "i"(csr), "r"(value));
}


// 简单的延迟函数
void delay(uint32_t cycles) {
  for (volatile uint32_t i = 0; i < cycles; i++) {
    // 空循环
  }
}

// 测试 MAIL_DATA0-3 寄存器
int test_mail_data_csrs(void) {
  uint64_t test_values[] = {
    0x0000000000000000ULL,
    0x123456789ABCDEF0ULL,
    0xFFFFFFFFFFFFFFFFULL,
    0x0000123456789ABCULL,
    0xFEDCBA9876543210ULL
  };

  printf("[TEST] Testing MAIL_DATA0-3 CSRs...\n");

  // 测试 MAIL_DATA0
  for (int i = 0; i < 5; i++) {
    csr_write(CSR_MAIL_DATA0, test_values[i]);
    uint64_t read_val = csr_read(CSR_MAIL_DATA0);
    if (read_val != test_values[i]) {
      printf("[ERROR] MAIL_DATA0 mismatch: wrote 0x%lx, read 0x%lx\n", 
          test_values[i], read_val);
      return -1;
    }
  }
  printf("[TEST] MAIL_DATA0: PASS\n");

  // 测试 MAIL_DATA1
  for (int i = 0; i < 5; i++) {
    csr_write(CSR_MAIL_DATA1, test_values[i] ^ 0xAAAAAAAAAAAAAAAAULL);
    uint64_t read_val = csr_read(CSR_MAIL_DATA1);
    if (read_val != (test_values[i] ^ 0xAAAAAAAAAAAAAAAAULL)) {
      printf("[ERROR] MAIL_DATA1 mismatch\n");
      return -1;
    }
  }
  printf("[TEST] MAIL_DATA1: PASS\n");

  // 测试 MAIL_DATA2
  for (int i = 0; i < 5; i++) {
    csr_write(CSR_MAIL_DATA2, test_values[i] ^ 0x5555555555555555ULL);
    uint64_t read_val = csr_read(CSR_MAIL_DATA2);
    if (read_val != (test_values[i] ^ 0x5555555555555555ULL)) {
      printf("[ERROR] MAIL_DATA2 mismatch\n");
      return -1;
    }
  }
  printf("[TEST] MAIL_DATA2: PASS\n");

  // 测试 MAIL_DATA3
  for (int i = 0; i < 5; i++) {
    csr_write(CSR_MAIL_DATA3, test_values[i] ^ 0xCCCCCCCCCCCCCCCCULL);
    uint64_t read_val = csr_read(CSR_MAIL_DATA3);
    if (read_val != (test_values[i] ^ 0xCCCCCCCCCCCCCCCCULL)) {
      printf("[ERROR] MAIL_DATA3 mismatch\n");
      return -1;
    }
  }
  printf("[TEST] MAIL_DATA3: PASS\n");

  return 0;
}

// 测试 MAIL_VALID 寄存器 (1-bit)
int test_mail_valid_csr(void) {
  printf("[TEST] Testing MAIL_VALID CSR (1-bit)...\n");

  // 测试写 0
  csr_write(CSR_MAIL_VALID, 0);
  uint32_t val = csr_read(CSR_MAIL_VALID);
  if (val != 0) {
    printf("[ERROR] MAIL_VALID: expected 0, got %u\n", val);
    return -1;
  }

  // 测试写 1
  csr_write(CSR_MAIL_VALID, 1);
  val = csr_read(CSR_MAIL_VALID);
  if (val != 1) {
    printf("[ERROR] MAIL_VALID: expected 1, got %u\n", val);
    return -1;
  }

  // 测试写大于 1 的值应该被 mask
  csr_write(CSR_MAIL_VALID, 0xFF);
  val = csr_read(CSR_MAIL_VALID);
  if (val != 1) {
    printf("[ERROR] MAIL_VALID: expected 1 (masked), got %u\n", val);
    return -1;
  }

  printf("[TEST] MAIL_VALID: PASS\n");
  return 0;
}

// 测试 BO_DONE 寄存器 (11-bit: [10:6]=wg_index, [5:0]=bar_index)
int test_bo_done_csr(void) {
  printf("[TEST] Testing BO_DONE CSR (11-bit)...\n");

  struct {
    uint32_t write_val;
    uint32_t expected_read;
    const char* desc;
  } test_cases[] = {
    {0x000, 0x000, "zero"},
    {0x001, 0x001, "bar_index=1"},
    {0x03F, 0x03F, "bar_index=31"},
    {0x040, 0x040, "wg_index=1"},
    {0x400, 0x400, "wg_index=8"},
    {0x7FF, 0x7FF, "max value"},
    {0xFFF, 0x7FF, "overflow masked"},
  };

  for (int i = 0; i < 7; i++) {
    csr_write(CSR_BO_DONE, test_cases[i].write_val);
    uint32_t val = csr_read(CSR_BO_DONE);
    if (val != test_cases[i].expected_read) {
      printf("[ERROR] BO_DONE %s: wrote 0x%03x, expected 0x%03x, got 0x%03x\n",
          test_cases[i].desc, test_cases[i].write_val, 
          test_cases[i].expected_read, val);
      return -1;
    }
  }

  printf("[TEST] BO_DONE: PASS\n");
  return 0;
}

// 测试 SE_UP 寄存器 (11-bit)
int test_se_up_csr(void) {
  printf("[TEST] Testing SE_UP CSR (11-bit)...\n");

  struct {
    uint32_t write_val;
    uint32_t expected_read;
    const char* desc;
  } test_cases[] = {
    {0x000, 0x000, "zero"},
    {0x001, 0x001, "bar_index=1"},
    {0x03F, 0x03F, "bar_index=31"},
    {0x040, 0x040, "wg_index=1"},
    {0x400, 0x400, "wg_index=8"},
    {0x7FF, 0x7FF, "max value"},
    {0xFFF, 0x7FF, "overflow masked"},
  };

  for (int i = 0; i < 7; i++) {
    csr_write(CSR_SE_UP, test_cases[i].write_val);
    uint32_t val = csr_read(CSR_SE_UP);
    if (val != test_cases[i].expected_read) {
      printf("[ERROR] SE_UP %s: wrote 0x%03x, expected 0x%03x, got 0x%03x\n",
          test_cases[i].desc, test_cases[i].write_val, 
          test_cases[i].expected_read, val);
      return -1;
    }
  }

  printf("[TEST] SE_UP: PASS\n");
  return 0;
}

// 测试 SE_QUERY_LOCK 寄存器 (11-bit)
int test_se_query_lock_csr(void) {
  printf("[TEST] Testing SE_QUERY_LOCK CSR (11-bit)...\n");

  struct {
    uint32_t write_val;
    uint32_t expected_read;
    const char* desc;
  } test_cases[] = {
    {0x000, 0x000, "zero"},
    {0x001, 0x001, "bar_index=1"},
    {0x03F, 0x03F, "bar_index=31"},
    {0x040, 0x040, "wg_index=1"},
    {0x400, 0x400, "wg_index=8"},
    {0x7FF, 0x7FF, "max value"},
    {0xFFF, 0x7FF, "overflow masked"},
  };

  for (int i = 0; i < 7; i++) {
    csr_write(CSR_SE_QUERY_LOCK, test_cases[i].write_val);
    uint32_t val = csr_read(CSR_SE_QUERY_LOCK);
    if (val != test_cases[i].expected_read) {
      printf("[ERROR] SE_QUERY_LOCK %s: wrote 0x%03x, expected 0x%03x, got 0x%03x\n",
          test_cases[i].desc, test_cases[i].write_val, 
          test_cases[i].expected_read, val);
      return -1;
    }
  }

  printf("[TEST] SE_QUERY_LOCK: PASS\n");
  return 0;
}

// 测试 SE_QUERY_COUNT 寄存器
int test_se_query_count_csr(void) {
  printf("[TEST] Testing SE_QUERY_COUNT CSR...\n");

  uint64_t test_values[] = {
    0x0000000000000000ULL,
    0x0000000000000001ULL,
    0x000000000000FFFFULL,
    0xFFFFFFFFFFFFFFFFULL,
    0x123456789ABCDEF0ULL
  };

  for (int i = 0; i < 5; i++) {
    csr_write(CSR_SE_QUERY_COUNT, test_values[i]);
    uint64_t read_val = csr_read(CSR_SE_QUERY_COUNT);
    if (read_val != test_values[i]) {
      printf("[ERROR] SE_QUERY_COUNT: wrote 0x%lx, read 0x%lx\n",
          test_values[i], read_val);
      return -1;
    }
  }

  printf("[TEST] SE_QUERY_COUNT: PASS\n");
  return 0;
}

// Mail 通道测试：模拟数据发送流程
int test_mail_channel(void) {
  printf("[TEST] Testing Mail channel protocol...\n");

  // 1. 写入 4 个 64-bit 数据
  uint64_t mail_data[4] = {
    0x1122334455667788ULL,
    0x99AABBCCDDEEFF00ULL,
    0x123456789ABCDEF0ULL,
    0xFEDCBA9876543210ULL
  };

  csr_write(CSR_MAIL_DATA0, mail_data[0]);
  csr_write(CSR_MAIL_DATA1, mail_data[1]);
  csr_write(CSR_MAIL_DATA2, mail_data[2]);
  csr_write(CSR_MAIL_DATA3, mail_data[3]);

  // 2. 验证数据写入
  if (csr_read(CSR_MAIL_DATA0) != mail_data[0]) return -1;
  if (csr_read(CSR_MAIL_DATA1) != mail_data[1]) return -1;
  if (csr_read(CSR_MAIL_DATA2) != mail_data[2]) return -1;
  if (csr_read(CSR_MAIL_DATA3) != mail_data[3]) return -1;

  // 3. 置位 valid
  csr_write(CSR_MAIL_VALID, 1);
  if (csr_read(CSR_MAIL_VALID) != 1) {
    printf("[ERROR] MAIL_VALID not set\n");
    return -1;
  }

  // 4. 模拟 RVV 读取后清除 valid
  csr_write(CSR_MAIL_VALID, 0);
  if (csr_read(CSR_MAIL_VALID) != 0) {
    printf("[ERROR] MAIL_VALID not cleared\n");
    return -1;
  }

  printf("[TEST] Mail channel: PASS\n");
  return 0;
}

// BO done 通道测试
int test_bo_done_channel(void) {
  printf("[TEST] Testing BO done channel...\n");

  // 模拟发送完成信号 (wg_index=5, bar_index=3)
  uint32_t done_signal = (5 << 6) | 3;  // wg_index [10:6], bar_index [5:0]
  csr_write(CSR_BO_DONE, done_signal);

  uint32_t read_val = csr_read(CSR_BO_DONE);
  if (read_val != done_signal) {
    printf("[ERROR] BO_DONE: wrote 0x%03x, read 0x%03x\n", done_signal, read_val);
    return -1;
  }

  printf("[TEST] BO done channel: PASS\n");
  return 0;
}

// SE up 通道测试
int test_se_up_channel(void) {
  printf("[TEST] Testing SE up channel...\n");

  // 模拟发送更新信号 (wg_index=7, bar_index=10)
  uint32_t up_signal = (7 << 6) | 10;
  csr_write(CSR_SE_UP, up_signal);

  uint32_t read_val = csr_read(CSR_SE_UP);
  if (read_val != up_signal) {
    printf("[ERROR] SE_UP: wrote 0x%03x, read 0x%03x\n", up_signal, read_val);
    return -1;
  }

  printf("[TEST] SE up channel: PASS\n");
  return 0;
}

// SE query 通道测试
int test_se_query_channel(void) {
  printf("[TEST] Testing SE query channel...\n");

  // 1. 发送 lock 信号 (wg_index=3, bar_index=5)
  uint32_t lock_signal = (3 << 6) | 5;
  csr_write(CSR_SE_QUERY_LOCK, lock_signal);

  if (csr_read(CSR_SE_QUERY_LOCK) != lock_signal) {
    printf("[ERROR] SE_QUERY_LOCK not set\n");
    return -1;
  }

  // 2. 模拟 query count 递增
  csr_write(CSR_SE_QUERY_COUNT, 5);
  if (csr_read(CSR_SE_QUERY_COUNT) != 5) return -1;

  csr_write(CSR_SE_QUERY_COUNT, 10);
  if (csr_read(CSR_SE_QUERY_COUNT) != 10) return -1;
  printf("[TEST] SE query channel: PASS\n");
  return 0;
}

// 固件主函数
void main(void) {
  int test_result = 0;

  printf("========================================\n");
  printf("[FIRMWARE]: CSR Test Firmware starting...\n");
  printf("========================================\n");

  // 测试所有自定义 CSR
  printf("\n--- CSR Register Tests ---\n");

  if (test_mail_data_csrs() != 0) test_result = ERR_CSR_MAIL_DATA;
  else if (test_mail_valid_csr() != 0) test_result = ERR_CSR_VALID;
  else if (test_bo_done_csr() != 0) test_result = ERR_CSR_BO_DONE;
  else if (test_se_up_csr() != 0) test_result = ERR_CSR_SE_UP;
  else if (test_se_query_lock_csr() != 0) test_result = ERR_CSR_SE_LOCK;
  else if (test_se_query_count_csr() != 0) test_result = ERR_CSR_SE_COUNT;

  // 测试通道协议
  printf("\n--- Channel Protocol Tests ---\n");

  if (test_result == 0 && test_mail_channel() != 0) test_result = ERR_CSR_READ;
  if (test_result == 0 && test_bo_done_channel() != 0) test_result = ERR_CSR_READ;
  if (test_result == 0 && test_se_up_channel() != 0) test_result = ERR_CSR_READ;
  if (test_result == 0 && test_se_query_channel() != 0) test_result = ERR_CSR_READ;

  // 输出测试结果
  printf("\n========================================\n");
  if (test_result == 0) {
    printf("[PASS]: All CSR tests passed!\n");
  } else {
    printf("[FAIL]: CSR test failed with error code: 0x%02X\n", test_result);
  }
  printf("========================================\n");

  // 循环等待，轮询 Mail 数据
  uint32_t loop_count = 0;
  uint32_t mail_poll_count = 0;
  printf("[FIRMWARE]: Entering main loop...\n");

  while (1) {
    loop_count++;
    
    // 每 100000 次循环轮询一次 Mail
    if (loop_count % 100000 == 0) {
      mail_poll_count++;
      
      // 轮询 MAIL_VALID
      uint64_t valid = csr_read(CSR_MAIL_VALID);
      
      if (valid & 0x1) {
        // Mail 有效，读取数据
        printf("[FIRMWARE] Mail valid detected! (poll #%u)\n", mail_poll_count);
        
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
      }
      
      if (loop_count % 1000000 == 0) {
        printf("[FIRMWARE]: Loop count: %u, Mail polls: %u\n", loop_count, mail_poll_count);
      }
    }
    
    delay(100);
  }
}
