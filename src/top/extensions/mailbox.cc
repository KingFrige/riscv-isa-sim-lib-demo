#include "mailbox.h"
#include "processor.h"
#include "sim.h"
#include <cstring>
#include <iostream>

mailbox_t::mailbox_t(const simif_t* sim, reg_t base_address)
    : sim(sim),
      base_address_(base_address),
      status_reg(MAILBOX_READY),  // 初始状态为就绪
      command_reg(0),
      response_reg(MAILBOX_SUCCESS),
      data0_reg(0),
      data1_reg(0),
      data2_reg(0),
      data3_reg(0),
      command_handler(nullptr),
      command_callback_(nullptr),
      error_callback_(nullptr),
      debug_(false)
{
    // 初始状态：就绪，不忙，无错误，非向量模式
    update_status(true, false, false, false);
    
    std::cerr << "[MAILBOX] Created at base address: 0x" << std::hex << base_address_ << std::dec << std::endl;
}

bool mailbox_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
    std::cerr << "[MAILBOX] load called: addr=0x" << std::hex << addr << ", len=" << std::dec << len << std::endl;
    
    // addr是相对于设备基地址的偏移量
    reg_t offset = addr;
    
    // 检查地址是否在设备范围内
    if (offset + len > MAILBOX_SIZE) {
        return false;
    }
    
    // 初始化返回数据为0
    memset(bytes, 0, len);
    
    // 根据偏移地址读取相应的寄存器
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
            
        case MAILBOX_RESPONSE_OFFSET:
            if (len == 4) {
                memcpy(bytes, &response_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA0_OFFSET:
            if (len == 8) {
                memcpy(bytes, &data0_reg, 8);
                return true;
            } else if (len == 4) {
                // 读取低32位
                uint32_t low = static_cast<uint32_t>(data0_reg);
                memcpy(bytes, &low, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA1_OFFSET:
            if (len == 8) {
                memcpy(bytes, &data1_reg, 8);
                return true;
            } else if (len == 4) {
                uint32_t low = static_cast<uint32_t>(data1_reg);
                memcpy(bytes, &low, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA2_OFFSET:
            if (len == 8) {
                memcpy(bytes, &data2_reg, 8);
                return true;
            } else if (len == 4) {
                uint32_t low = static_cast<uint32_t>(data2_reg);
                memcpy(bytes, &low, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA3_OFFSET:
            if (len == 8) {
                memcpy(bytes, &data3_reg, 8);
                return true;
            } else if (len == 4) {
                uint32_t low = static_cast<uint32_t>(data3_reg);
                memcpy(bytes, &low, 4);
                return true;
            }
            break;
            
        default:
            // 对于未映射的地址，返回0
            return true;
    }
    
    return false;
}

bool mailbox_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
    std::cerr << "[MAILBOX] store called: addr=0x" << std::hex << addr << ", len=" << std::dec << len << std::endl;
    
    // addr是相对于设备基地址的偏移量
    reg_t offset = addr;
    
    // 检查地址是否在设备范围内
    if (offset + len > MAILBOX_SIZE) {
        std::cerr << "[MAILBOX] Address out of range: offset=0x" << std::hex << offset << ", len=" << std::dec << len << ", MAILBOX_SIZE=0x" << std::hex << MAILBOX_SIZE << std::dec << std::endl;
        return false;
    }
    
    switch (offset) {
        case MAILBOX_STATUS_OFFSET:
            if (len == 4) {
                uint32_t new_status;
                memcpy(&new_status, bytes, 4);
                
                std::cerr << "[MAILBOX] Host wrote STATUS: 0x" << std::hex << new_status << std::dec << std::endl;
                
                // 写入状态寄存器会触发命令执行
                // 任何非零写入都会触发命令处理
                if (new_status != 0) {
                    // 清除错误位
                    status_reg &= ~MAILBOX_ERROR;
                    
                    // 开始处理命令
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
            
        case MAILBOX_RESPONSE_OFFSET:
            if (len == 4) {
                // 响应寄存器写入：这通常是固件写入响应的地方
                uint32_t new_response;
                memcpy(&new_response, bytes, 4);
                response_reg = new_response;
                
                std::cerr << "[MAILBOX] Firmware wrote response: 0x" << std::hex << response_reg << std::dec << std::endl;
                
                // 更新状态：命令已完成，清除忙状态，设置就绪状态
                bool vector_mode = (status_reg & MAILBOX_VECTOR_MODE) != 0;
                update_status(true, false, false, vector_mode);
                
                return true;
            }
            break;
            
        case MAILBOX_DATA0_OFFSET:
            if (len == 8) {
                memcpy(&data0_reg, bytes, 8);
                return true;
            } else if (len == 4) {
                // 写入低32位，高32位保持不变
                uint32_t low;
                memcpy(&low, bytes, 4);
                data0_reg = (data0_reg & 0xFFFFFFFF00000000ULL) | low;
                return true;
            }
            break;
            
        case MAILBOX_DATA1_OFFSET:
            if (len == 8) {
                memcpy(&data1_reg, bytes, 8);
                return true;
            } else if (len == 4) {
                uint32_t low;
                memcpy(&low, bytes, 4);
                data1_reg = (data1_reg & 0xFFFFFFFF00000000ULL) | low;
                return true;
            }
            break;
            
        case MAILBOX_DATA2_OFFSET:
            if (len == 8) {
                memcpy(&data2_reg, bytes, 8);
                return true;
            } else if (len == 4) {
                uint32_t low;
                memcpy(&low, bytes, 4);
                data2_reg = (data2_reg & 0xFFFFFFFF00000000ULL) | low;
                return true;
            }
            break;
            
        case MAILBOX_DATA3_OFFSET:
            if (len == 8) {
                memcpy(&data3_reg, bytes, 8);
                return true;
            } else if (len == 4) {
                uint32_t low;
                memcpy(&low, bytes, 4);
                data3_reg = (data3_reg & 0xFFFFFFFF00000000ULL) | low;
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
    // 设置忙状态，但不立即处理命令
    // 让固件代码在 Spike 中运行时检测到状态变化并处理命令
    update_status(false, true, false, false);
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
    return T(0);
}

template<typename T>
void mailbox_t::write_register(reg_t offset, T value)
{
    (void)offset;
    (void)value;
}

// 当固件处理完命令后调用此方法来更新响应
void mailbox_t::on_firmware_command_processed(uint32_t response) {
    // 更新响应寄存器
    response_reg = response;
    
    // 清除忙状态，设置就绪状态
    update_status(true, false, false, false);
}

// 发送命令到mailbox（从spike_wrapper.cc迁移过来的函数）
uint32_t mailbox_t::send_command(uint32_t command, uint64_t data0,
                                uint64_t data1, uint64_t data2, uint64_t data3) {
    // 检查mailbox是否就绪
    if (!(status_reg & MAILBOX_READY) || (status_reg & MAILBOX_BUSY)) {
        std::cerr << "[MAILBOX] Mailbox not ready or busy" << std::endl;
        return 0xFFFFFFFF;  // 错误码
    }
    
    // 设置命令参数
    command_reg = command;
    data0_reg = data0;
    data1_reg = data1;
    data2_reg = data2;
    data3_reg = data3;
    
    // 清除之前的响应
    response_reg = MAILBOX_SUCCESS;
    
    // 设置忙状态，清除就绪状态
    update_status(false, true, false, false);
    
    // 触发命令处理
    process_command();
    
    return MAILBOX_SUCCESS;
}

// 设置回调函数
void mailbox_t::set_command_callback(CommandCallback callback) {
    command_callback_ = callback;
}

void mailbox_t::set_error_callback(ErrorCallback callback) {
    error_callback_ = callback;
}

// 等待mailbox就绪
bool mailbox_t::wait_for_ready(uint32_t timeout_ms, uint32_t poll_interval_us) const {
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);
    
    while (true) {
        if ((status_reg & MAILBOX_READY) && !(status_reg & MAILBOX_BUSY)) {
            return true;
        }
        
        auto now = std::chrono::steady_clock::now();
        if (now - start > timeout) {
            if (debug_) {
                std::cerr << "[MAILBOX] Wait for ready timeout" << std::endl;
            }
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(poll_interval_us));
    }
}

// 完整的send_command函数（包含等待和回调）
uint32_t mailbox_t::send_command_complete(uint32_t command, uint64_t data0,
                                         uint64_t data1, uint64_t data2, uint64_t data3,
                                         uint32_t timeout_ms, uint32_t poll_interval_us,
                                         TriggerCallback trigger_callback) {
    if (debug_) {
        std::cout << "[MAILBOX] Sending command: 0x" 
                  << std::hex << command << std::dec << std::endl;
    }
    
    // 检查mailbox是否就绪
    if (!wait_for_ready(timeout_ms, poll_interval_us)) {
        std::cerr << "[MAILBOX] Mailbox not ready" << std::endl;
        return 0xFFFFFFFF;
    }
    
    // 设置命令参数
    uint32_t response = send_command(command, data0, data1, data2, data3);
    
    if (response == 0xFFFFFFFF) {
        return response;
    }
    
    // 触发命令执行（通过回调函数）
    if (trigger_callback) {
        trigger_callback();
    } else {
        if (debug_) {
            std::cout << "[MAILBOX] No trigger callback provided" << std::endl;
        }
        return 0xFFFFFFFF;
    }
    
    if (debug_) {
        std::cout << "[MAILBOX] Command execution triggered, waiting for firmware response..." << std::endl;
    }
    
    // 等待命令处理完成
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);
    
    while (true) {
        if (!(status_reg & MAILBOX_BUSY)) {
            response = response_reg;
            
            if (debug_) {
                std::cout << "[MAILBOX] Command response: 0x" 
                          << std::hex << response << std::dec << std::endl;
            }
            break;
        }
        
        auto now = std::chrono::steady_clock::now();
        if (now - start > timeout) {
            std::cerr << "[MAILBOX] Command timeout" << std::endl;
            response = 0xFFFFFFFF;
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(poll_interval_us));
    }
    
    // 调用回调函数
    if (command_callback_) {
        command_callback_(command, response);
    }
    
    if (response != 0 && error_callback_) {
        error_callback_(response);
    }
    
    return response;
}

// 模板实例化
template uint32_t mailbox_t::read_register<uint32_t>(reg_t offset) const;
template uint64_t mailbox_t::read_register<uint64_t>(reg_t offset) const;

template void mailbox_t::write_register<uint32_t>(reg_t offset, uint32_t value);
template void mailbox_t::write_register<uint64_t>(reg_t offset, uint64_t value);