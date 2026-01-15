#ifndef _RISCV_MAILBOX_H
#define _RISCV_MAILBOX_H

#include "riscv/devices.h"
#include "riscv/simif.h"
#include <cstdint>
#include <functional>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

#define MAILBOX_BASE 0x60000000

class mailbox_t : public abstract_device_t {
public:
    // 寄存器偏移定义
    static constexpr reg_t MAILBOX_SIZE = 0x1000;     // 4KB 地址空间
    
    // 寄存器偏移
    static constexpr reg_t MAILBOX_STATUS_OFFSET      = 0x0000;
    static constexpr reg_t MAILBOX_COMMAND_OFFSET     = 0x0004;
    static constexpr reg_t MAILBOX_DATA_ADDR_OFFSET   = 0x0008;
    static constexpr reg_t MAILBOX_DATA_SIZE_OFFSET   = 0x0010;
    static constexpr reg_t MAILBOX_RESPONSE_OFFSET    = 0x0014;
    static constexpr reg_t MAILBOX_VECTOR_CONFIG_OFFSET = 0x0018;
    
    // 状态寄存器位定义
    static constexpr uint32_t MAILBOX_READY      = 0x00000001;
    static constexpr uint32_t MAILBOX_BUSY       = 0x00000002;
    static constexpr uint32_t MAILBOX_ERROR      = 0x00000004;
    static constexpr uint32_t MAILBOX_VECTOR_MODE = 0x00000008;
    
    // 命令编码
    static constexpr uint32_t MAILBOX_CMD_HELLO          = 0x00000001;
    static constexpr uint32_t MAILBOX_CMD_HI             = 0x00000002;
    static constexpr uint32_t MAILBOX_CMD_VECTOR_LOAD    = 0x00000010;
    static constexpr uint32_t MAILBOX_CMD_VECTOR_STORE   = 0x00000011;
    static constexpr uint32_t MAILBOX_CMD_VECTOR_COMPUTE = 0x00000012;
    static constexpr uint32_t MAILBOX_CMD_SOFTMAX        = 0x00000020;
    
    // 错误码定义
    static constexpr uint32_t MAILBOX_SUCCESS           = 0x00000000;
    static constexpr uint32_t MAILBOX_ERR_INVALID_CMD   = 0x00000001;
    static constexpr uint32_t MAILBOX_ERR_INVALID_PARAM = 0x00000002;
    static constexpr uint32_t MAILBOX_ERR_MEM_ACCESS    = 0x00000003;
    static constexpr uint32_t MAILBOX_ERR_VECTOR_CONFIG = 0x00000004;
    static constexpr uint32_t MAILBOX_ERR_NOT_IMPLEMENTED = 0x00000005;
    
    mailbox_t(const simif_t* sim, reg_t base_address = MAILBOX_BASE);
    ~mailbox_t() = default;
    
    // abstract_device_t 接口
    bool load(reg_t addr, size_t len, uint8_t* bytes) override;
    bool store(reg_t addr, size_t len, const uint8_t* bytes) override;
    reg_t size() override { return MAILBOX_SIZE; }
    
    // 命令处理回调函数类型
    using command_handler_t = std::function<uint32_t(uint32_t command, reg_t data_addr, uint32_t data_size, uint64_t vector_config)>;
    
    // 设置命令处理器
    void set_command_handler(command_handler_t handler) { command_handler = handler; }
    
    // 获取当前状态
    uint32_t get_status() const { return status_reg; }
    uint32_t get_response() const { return response_reg; }
    
    // 检查命令是否完成
    bool is_command_complete() const { return !(status_reg & MAILBOX_BUSY); }
    
    // 模拟固件完成命令（在实际固件执行后调用）
    void on_firmware_command_processed(uint32_t response);
    
    // 发送命令到mailbox（从spike_wrapper.cc迁移过来的函数）
    uint32_t send_command(uint32_t command, uint64_t data_addr = 0,
                         uint32_t data_size = 0, uint64_t vector_config = 0);
    
    // 回调函数类型定义
    using CommandCallback = std::function<void(uint32_t command, uint32_t response)>;
    using ErrorCallback = std::function<void(uint32_t error_code)>;
    using TriggerCallback = std::function<void()>;  // 触发命令执行的函数
    
    // 设置回调函数
    void set_command_callback(CommandCallback callback);
    void set_error_callback(ErrorCallback callback);
    
    // 完整的send_command函数（包含等待和回调）
    uint32_t send_command_complete(uint32_t command, uint64_t data_addr = 0,
                                  uint32_t data_size = 0, uint64_t vector_config = 0,
                                  uint32_t timeout_ms = 2000, uint32_t poll_interval_us = 100,
                                  TriggerCallback trigger_callback = nullptr);
    
private:
    const simif_t* sim;
    reg_t base_address_;  // 设备基地址
    
    // 寄存器状态
    uint32_t status_reg;
    uint32_t command_reg;
    uint64_t data_addr_reg;
    uint32_t data_size_reg;
    uint32_t response_reg;
    uint64_t vector_config_reg;
    
    // 命令处理器
    command_handler_t command_handler;
    
    // 回调函数
    CommandCallback command_callback_;
    ErrorCallback error_callback_;
    
    // 调试标志
    bool debug_;
    
    // 内部方法
    void process_command();
    void update_status(bool ready, bool busy, bool error, bool vector_mode);
    
    // 辅助函数
    template<typename T>
    T read_register(reg_t offset) const;
    
    template<typename T>
    void write_register(reg_t offset, T value);
    
    // 验证地址范围
    bool validate_address(reg_t addr, size_t len) const;
    
    // 等待mailbox就绪
    bool wait_for_ready(uint32_t timeout_ms = 2000, uint32_t poll_interval_us = 100) const;
};

#endif // _RISCV_MAILBOX_H
