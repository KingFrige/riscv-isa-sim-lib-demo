#include "mailbox.h"
#include "processor.h"
#include "sim.h"
#include <cstring>
#include <iostream>

mailbox_t::mailbox_t(const simif_t* sim)
    : sim(sim),
      status_reg(MAILBOX_READY),  // 初始状态为就绪
      command_reg(0),
      data_addr_reg(0),
      data_size_reg(0),
      response_reg(MAILBOX_SUCCESS),
      vector_config_reg(0),
      command_handler(nullptr)
{
    // 初始状态：就绪，不忙，无错误，非向量模式
    update_status(true, false, false, false);
}

bool mailbox_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
    if (!validate_address(addr, len)) {
        return false;
    }
    
    // 根据偏移地址读取相应的寄存器
    reg_t offset = addr - MAILBOX_BASE;
    
    switch (offset) {
        case MAILBOX_STATUS_OFFSET:
            if (len == 4) {
                memcpy(bytes, &status_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_COMMAND_OFFSET:
            if (len == 4) {
                memcpy(bytes, &command_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA_ADDR_OFFSET:
            if (len == 8) {
                memcpy(bytes, &data_addr_reg, 8);
                return true;
            } else if (len == 4) {
                // 读取低32位
                uint32_t low = static_cast<uint32_t>(data_addr_reg);
                memcpy(bytes, &low, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA_SIZE_OFFSET:
            if (len == 4) {
                memcpy(bytes, &data_size_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_RESPONSE_OFFSET:
            if (len == 4) {
                memcpy(bytes, &response_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_VECTOR_CONFIG_OFFSET:
            if (len == 8) {
                memcpy(bytes, &vector_config_reg, 8);
                return true;
            } else if (len == 4) {
                // 读取低32位
                uint32_t low = static_cast<uint32_t>(vector_config_reg);
                memcpy(bytes, &low, 4);
                return true;
            }
            break;
            
        default:
            // 未映射的地址返回0
            memset(bytes, 0, len);
            return true;
    }
    
    return false;
}

bool mailbox_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
    if (!validate_address(addr, len)) {
        return false;
    }
    
    reg_t offset = addr - MAILBOX_BASE;
    
    switch (offset) {
        case MAILBOX_STATUS_OFFSET:
            if (len == 4) {
                uint32_t new_status;
                memcpy(&new_status, bytes, 4);
                
                // 写入状态寄存器会触发命令执行
                // 任何非零写入都会触发命令处理
                if (new_status != 0) {
                    // 清除错误位
                    status_reg &= ~MAILBOX_ERROR;
                    
                    // 开始处理命令
                    // 注意：我们不再检查当前状态，因为主机写入状态寄存器就是触发命令的信号
                    process_command();
                }
                return true;
            }
            break;
            
        case MAILBOX_COMMAND_OFFSET:
            if (len == 4) {
                memcpy(&command_reg, bytes, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA_ADDR_OFFSET:
            if (len == 8) {
                memcpy(&data_addr_reg, bytes, 8);
                return true;
            } else if (len == 4) {
                // 写入低32位，高32位保持不变
                uint32_t low;
                memcpy(&low, bytes, 4);
                data_addr_reg = (data_addr_reg & 0xFFFFFFFF00000000ULL) | low;
                return true;
            }
            break;
            
        case MAILBOX_DATA_SIZE_OFFSET:
            if (len == 4) {
                memcpy(&data_size_reg, bytes, 4);
                return true;
            }
            break;
            
        case MAILBOX_RESPONSE_OFFSET:
            if (len == 4) {
                // 响应寄存器通常只读，但允许写入以清除错误状态
                memcpy(&response_reg, bytes, 4);
                return true;
            }
            break;
            
        case MAILBOX_VECTOR_CONFIG_OFFSET:
            if (len == 8) {
                memcpy(&vector_config_reg, bytes, 8);
                return true;
            } else if (len == 4) {
                // 写入低32位，高32位保持不变
                uint32_t low;
                memcpy(&low, bytes, 4);
                vector_config_reg = (vector_config_reg & 0xFFFFFFFF00000000ULL) | low;
                return true;
            }
            break;
            
        default:
            // 忽略对未映射地址的写入
            return true;
    }
    
    return false;
}

void mailbox_t::process_command()
{
    // 设置忙状态
    update_status(false, true, false, vector_config_reg != 0);
    
    // 模拟固件处理命令
    // 在实际系统中，固件会处理命令并写入响应
    // 这里我们模拟固件的行为，并通过UART输出
    
    uint32_t response = MAILBOX_SUCCESS;
    
    // 根据命令类型返回响应
    switch (command_reg) {
        case 0x00000001:  // MAILBOX_CMD_HELLO
            // 模拟固件通过UART输出（UART地址 0x10000000）
            // 在实际系统中，固件会写入UART设备
            // 这里我们直接输出到控制台，模拟UART输出
            std::cout << "[UART 0x10000000]: [FIRMWARE] Hello from firmware!" << std::endl;
            response = MAILBOX_SUCCESS;
            break;
            
        case 0x00000002:  // MAILBOX_CMD_HI
            // 模拟固件通过UART输出
            std::cout << "[UART 0x10000000]: [FIRMWARE] Hi from firmware!" << std::endl;
            response = MAILBOX_SUCCESS;
            break;
            
        case 0x00000010:  // MAILBOX_CMD_VECTOR_LOAD
            // 模拟固件通过UART输出
            std::cout << "[UART 0x10000000]: [FIRMWARE] Vector load command received" << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Data address: 0x" << std::hex << data_addr_reg << std::dec << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Data size: " << data_size_reg << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Vector config: 0x" << std::hex << vector_config_reg << std::dec << std::endl;
            response = MAILBOX_SUCCESS;
            break;
            
        case 0x00000011:  // MAILBOX_CMD_VECTOR_STORE
            // 模拟固件通过UART输出
            std::cout << "[UART 0x10000000]: [FIRMWARE] Vector store command received" << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Data address: 0x" << std::hex << data_addr_reg << std::dec << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Data size: " << data_size_reg << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Vector config: 0x" << std::hex << vector_config_reg << std::dec << std::endl;
            response = MAILBOX_SUCCESS;
            break;
            
        case 0x00000012:  // MAILBOX_CMD_VECTOR_COMPUTE
            // 模拟固件通过UART输出
            std::cout << "[UART 0x10000000]: [FIRMWARE] Vector compute command received" << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Data address: 0x" << std::hex << data_addr_reg << std::dec << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Data size: " << data_size_reg << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Vector config: 0x" << std::hex << vector_config_reg << std::dec << std::endl;
            response = MAILBOX_SUCCESS;
            break;
            
        case 0x00000020:  // MAILBOX_CMD_SOFTMAX
            // 模拟固件通过UART输出
            std::cout << "[UART 0x10000000]: [FIRMWARE] Softmax command received" << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Data address: 0x" << std::hex << data_addr_reg << std::dec << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Data size: " << data_size_reg << " bytes (" << (data_size_reg / 4) << " elements)" << std::endl;
            std::cout << "[UART 0x10000000]: [FIRMWARE] Vector config: 0x" << std::hex << vector_config_reg << std::dec << std::endl;
            response = MAILBOX_SUCCESS;
            break;
            
        default:
            // 模拟固件通过UART输出
            std::cout << "[UART 0x10000000]: [FIRMWARE] Unknown command: 0x" << std::hex << command_reg << std::dec << std::endl;
            response = MAILBOX_ERR_INVALID_CMD;
            break;
    }
    
    // 写入响应
    response_reg = response;
    
    // 清除忙状态，设置就绪状态
    update_status(true, false, false, vector_config_reg != 0);
}

void mailbox_t::update_status(bool ready, bool busy, bool error, bool vector_mode)
{
    status_reg = 0;
    if (ready) status_reg |= MAILBOX_READY;
    if (busy) status_reg |= MAILBOX_BUSY;
    if (error) status_reg |= MAILBOX_ERROR;
    if (vector_mode) status_reg |= MAILBOX_VECTOR_MODE;
}

bool mailbox_t::validate_address(reg_t addr, size_t len) const
{
    return (addr >= MAILBOX_BASE) && (addr + len <= MAILBOX_BASE + MAILBOX_SIZE);
}

// 模板函数定义
template<typename T>
T mailbox_t::read_register(reg_t offset) const
{
    // 这是一个简单的实现，实际中可能需要根据偏移地址读取不同的寄存器
    // 这里返回0作为占位符
    return T(0);
}

template<typename T>
void mailbox_t::write_register(reg_t offset, T value)
{
    // 这是一个简单的实现，实际中可能需要根据偏移地址写入不同的寄存器
    // 这里不做任何操作作为占位符
    (void)offset;
    (void)value;
}

// 模板实例化
template uint32_t mailbox_t::read_register<uint32_t>(reg_t offset) const;
template uint64_t mailbox_t::read_register<uint64_t>(reg_t offset) const;

template void mailbox_t::write_register<uint32_t>(reg_t offset, uint32_t value);
template void mailbox_t::write_register<uint64_t>(reg_t offset, uint64_t value);