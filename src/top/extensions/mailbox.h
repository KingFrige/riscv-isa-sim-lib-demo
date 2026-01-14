#ifndef _RISCV_MAILBOX_H
#define _RISCV_MAILBOX_H

#include "devices.h"
#include "simif.h"
#include <cstdint>
#include <functional>
#include <vector>

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
    
    // 错误码定义
    static constexpr uint32_t MAILBOX_SUCCESS           = 0x00000000;
    static constexpr uint32_t MAILBOX_ERR_INVALID_CMD   = 0x00000001;
    static constexpr uint32_t MAILBOX_ERR_INVALID_PARAM = 0x00000002;
    static constexpr uint32_t MAILBOX_ERR_MEM_ACCESS    = 0x00000003;
    static constexpr uint32_t MAILBOX_ERR_VECTOR_CONFIG = 0x00000004;
    static constexpr uint32_t MAILBOX_ERR_NOT_IMPLEMENTED = 0x00000005;
    
    mailbox_t(const simif_t* sim);
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
    
private:
    const simif_t* sim;
    
    // 寄存器状态
    uint32_t status_reg;
    uint32_t command_reg;
    uint64_t data_addr_reg;
    uint32_t data_size_reg;
    uint32_t response_reg;
    uint64_t vector_config_reg;
    
    // 命令处理器
    command_handler_t command_handler;
    
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
};

#endif // _RISCV_MAILBOX_H
