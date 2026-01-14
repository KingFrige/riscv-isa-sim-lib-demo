#include "spike_wrapper.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>

// Spike 头文件
#include "sim.h"
#include "cfg.h"
#include "mmu.h"
#include "mailbox.h"
#include "devices.h"
#include "abstract_device.h"

using namespace std::chrono_literals;

// SpikeWrapper 实现类
class SpikeWrapper::Impl {
public:
    Impl(const std::string& firmware_path, uint64_t mailbox_base)
        : firmware_path_(firmware_path)
        , mailbox_base_(mailbox_base)
        , running_(false)
        , debug_(false)
        , poll_interval_us_(1000)
        , command_timeout_ms_(1000) {
    }
    
    ~Impl() {
        stop();
        
        // 清理内存对象
        for (auto& mem : mems_) {
            delete mem.second;
        }
        mems_.clear();
    }
    
    bool start(const std::vector<std::string>& args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (running_) {
            if (debug_) {
                std::cerr << "[SpikeWrapper] Already running" << std::endl;
            }
            return false;
        }
        
        try {
            // 准备 Spike 参数
            std::vector<std::string> spike_args = {
                "spike",
                "--isa=rv64gc",
                "-m0x80000000:0x10000"  // 128KB 内存
            };
            
            // 添加固件参数
            if (!firmware_path_.empty()) {
                spike_args.push_back("--rom=" + firmware_path_);
            }
            
            // 添加用户参数
            spike_args.insert(spike_args.end(), args.begin(), args.end());
            
            if (debug_) {
                std::cout << "[SpikeWrapper] Starting Spike with args: ";
                for (const auto& arg : spike_args) {
                    std::cout << arg << " ";
                }
                std::cout << std::endl;
            }
            
            // 创建 Spike 实例
            // 注意：这里简化了 Spike 的初始化，实际实现需要更复杂的配置
            sim_ = create_spike_instance(spike_args);
            
            // 简化实现：即使没有真正的 Spike 实例也继续
            // 在实际实现中，这里需要真正的 Spike 实例
            
            // 启动 Spike 线程
            running_ = true;
            spike_thread_ = std::thread(&Impl::run_spike, this);
            
            if (debug_) {
                std::cout << "[SpikeWrapper] Spike started successfully" << std::endl;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[SpikeWrapper] Error starting Spike: " << e.what() << std::endl;
            return false;
        }
    }
    
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                return;
            }
            running_ = false;
        }
        
        cv_.notify_all();
        
        if (spike_thread_.joinable()) {
            spike_thread_.join();
        }
        
        sim_.reset();
        
        if (debug_) {
            std::cout << "[SpikeWrapper] Spike stopped" << std::endl;
        }
    }
    
    bool is_running() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return running_;
    }
    
    bool wait(uint32_t timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (!running_) {
            return true;
        }
        
        if (timeout_ms == 0) {
            cv_.wait(lock, [this] { return !running_; });
            return true;
        } else {
            return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                               [this] { return !running_; });
        }
    }
    
    // Mailbox 命令接口
    uint32_t send_hello() {
        return send_command(0x00000001);  // MAILBOX_CMD_HELLO
    }
    
    uint32_t send_hi() {
        return send_command(0x00000002);  // MAILBOX_CMD_HI
    }
    
    uint32_t send_vector_load(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
        return send_command(0x00000010, data_addr, data_size, vector_config);
    }
    
    uint32_t send_vector_store(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
        return send_command(0x00000011, data_addr, data_size, vector_config);
    }
    
    uint32_t send_vector_compute(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
        return send_command(0x00000012, data_addr, data_size, vector_config);
    }
    
    uint32_t send_softmax(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
        return send_command(0x00000020, data_addr, data_size, vector_config);  // MAILBOX_CMD_SOFTMAX
    }
    
    uint32_t send_command(uint32_t command) {
        return send_command(command, 0, 0, 0);
    }
    
    uint32_t send_command(uint32_t command, uint64_t data_addr,
                         uint32_t data_size, uint64_t vector_config) {
        std::lock_guard<std::mutex> lock(command_mutex_);
        
        if (debug_) {
            std::cout << "[SpikeWrapper] Sending command: 0x" 
                      << std::hex << command << std::dec << std::endl;
        }
        
        // 检查是否有 mailbox 设备
        if (!mailbox_dev_) {
            std::cerr << "[SpikeWrapper] No mailbox device available" << std::endl;
            return 0xFFFFFFFF;  // 错误码
        }
        
        // 检查 Spike 实例是否存在
        if (!sim_) {
            std::cerr << "[SpikeWrapper] No Spike instance available" << std::endl;
            return 0xFFFFFFFF;  // 错误码
        }
        
        // 等待 mailbox 就绪
        if (!wait_for_mailbox_ready()) {
            std::cerr << "[SpikeWrapper] Mailbox not ready" << std::endl;
            return 0xFFFFFFFF;  // 错误码
        }
        
        // 通过 mailbox 设备写入命令参数
        // 注意：mailbox_t 设备通过 store() 方法处理写入
        
        // 写入命令寄存器
        uint32_t cmd = command;
        mailbox_dev_->store(mailbox_base_ + 0x04, 4, (const uint8_t*)&cmd);  // MAILBOX_COMMAND_OFFSET
        
        // 写入数据地址寄存器（64位）
        uint64_t addr = data_addr;
        mailbox_dev_->store(mailbox_base_ + 0x08, 8, (const uint8_t*)&addr);  // MAILBOX_DATA_ADDR_OFFSET
        
        // 写入数据大小寄存器
        uint32_t size = data_size;
        mailbox_dev_->store(mailbox_base_ + 0x10, 4, (const uint8_t*)&size);  // MAILBOX_DATA_SIZE_OFFSET
        
        // 写入向量配置寄存器（64位）
        uint64_t vconfig = vector_config;
        mailbox_dev_->store(mailbox_base_ + 0x18, 8, (const uint8_t*)&vconfig);  // MAILBOX_VECTOR_CONFIG_OFFSET
        
        if (debug_) {
            std::cout << "[SpikeWrapper] Command parameters written to mailbox, triggering command execution..." << std::endl;
        }
        
        // 写入状态寄存器以触发命令执行
        // 根据 mailbox.cc 的实现，写入状态寄存器会触发 process_command()
        uint32_t trigger = 1;  // 任何非零值都会触发命令处理
        mailbox_dev_->store(mailbox_base_ + 0x00, 4, (const uint8_t*)&trigger);  // MAILBOX_STATUS_OFFSET
        
        if (debug_) {
            std::cout << "[SpikeWrapper] Command execution triggered, waiting for firmware response..." << std::endl;
        }
        
        // 等待命令处理完成（固件处理命令并设置响应）
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(command_timeout_ms_);
        
        uint32_t response = 0xFFFFFFFF;
        
        while (true) {
            // 读取状态寄存器
            uint32_t status = 0;
            mailbox_dev_->load(mailbox_base_ + 0x00, 4, (uint8_t*)&status);  // MAILBOX_STATUS_OFFSET
            
            // 检查是否不再忙（固件已处理完命令）
            if (!(status & 0x00000002)) {  // BUSY 位为 0
                // 读取响应寄存器
                mailbox_dev_->load(mailbox_base_ + 0x14, 4, (uint8_t*)&response);  // MAILBOX_RESPONSE_OFFSET
                
                if (debug_) {
                    std::cout << "[SpikeWrapper] Command response: 0x" 
                              << std::hex << response << std::dec << std::endl;
                }
                break;
            }
            
            auto now = std::chrono::steady_clock::now();
            if (now - start > timeout) {
                std::cerr << "[SpikeWrapper] Command timeout" << std::endl;
                response = 0xFFFFFFFF;  // 超时错误
                break;
            }
            
            // 短暂休眠避免过于频繁的轮询
            std::this_thread::sleep_for(std::chrono::microseconds(poll_interval_us_));
        }
        
        // 调用回调函数
        if (command_callback_) {
            command_callback_(command, response);
        }
        
        // 检查错误
        if (response != 0 && error_callback_) {
            error_callback_(response);
        }
        
        return response;
    }
    
    // 状态查询
    uint32_t get_mailbox_status() const {
        if (!mailbox_dev_) {
            return 0;
        }
        uint32_t status = 0;
        mailbox_dev_->load(mailbox_base_ + 0x00, 4, (uint8_t*)&status);  // MAILBOX_STATUS_OFFSET
        return status;
    }
    
    bool is_mailbox_ready() const {
        uint32_t status = get_mailbox_status();
        return (status & 0x00000001) != 0;  // READY 位
    }
    
    bool is_mailbox_busy() const {
        uint32_t status = get_mailbox_status();
        return (status & 0x00000002) != 0;  // BUSY 位
    }
    
    bool has_error() const {
        uint32_t status = get_mailbox_status();
        return (status & 0x00000004) != 0;  // ERROR 位
    }
    
    uint32_t get_error_code() const {
        if (!mailbox_dev_) {
            return 0;
        }
        uint32_t response = 0;
        mailbox_dev_->load(mailbox_base_ + 0x14, 4, (uint8_t*)&response);  // MAILBOX_RESPONSE_OFFSET
        return response;
    }
    
    // 回调函数设置
    void set_command_callback(CommandCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        command_callback_ = std::move(callback);
    }
    
    void set_error_callback(std::function<void(uint32_t)> callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        error_callback_ = std::move(callback);
    }
    
    // 配置选项
    void set_poll_interval(uint32_t interval_us) {
        poll_interval_us_ = interval_us;
    }
    
    void set_command_timeout(uint32_t timeout_ms) {
        command_timeout_ms_ = timeout_ms;
    }
    
    void set_debug(bool enable) {
        debug_ = enable;
    }
    
private:
    // Spike 实例创建
    std::unique_ptr<sim_t> create_spike_instance(const std::vector<std::string>& args) {
        if (debug_) {
            std::cout << "[SpikeWrapper] Creating Spike instance with args: ";
            for (const auto& arg : args) {
                std::cout << arg << " ";
            }
            std::cout << std::endl;
        }
        
        // 检查固件文件是否存在
        if (!firmware_path_.empty()) {
            std::ifstream file(firmware_path_);
            if (!file.good()) {
                std::cerr << "[SpikeWrapper] Firmware file not found: " << firmware_path_ << std::endl;
                // 尝试其他路径
                std::string alt_path = "../" + firmware_path_;
                std::ifstream alt_file(alt_path);
                if (alt_file.good()) {
                    std::cout << "[SpikeWrapper] Found firmware at alternative path: " << alt_path << std::endl;
                    // 更新固件路径为找到的替代路径
                    firmware_path_ = alt_path;
                } else {
                    // 尝试另一个可能的路径
                    alt_path = "../../" + firmware_path_;
                    std::ifstream alt_file2(alt_path);
                    if (alt_file2.good()) {
                        std::cout << "[SpikeWrapper] Found firmware at alternative path: " << alt_path << std::endl;
                        // 更新固件路径为找到的替代路径
                        firmware_path_ = alt_path;
                    } else {
                        std::cerr << "[SpikeWrapper] Firmware not found at any alternative path" << std::endl;
                    }
                }
            } else {
                std::cout << "[SpikeWrapper] Firmware file found: " << firmware_path_ << std::endl;
            }
        }
        
        try {
            // 创建配置
            cfg_t cfg;
            
            // 设置 ISA
            cfg.isa = "rv64gc";
            
            // 设置内存布局 - 128KB 内存从 0x80000000 开始
            std::vector<mem_cfg_t> mem_layout;
            mem_layout.push_back(mem_cfg_t(0x80000000, 0x10000));  // 64KB
            cfg.mem_layout = mem_layout;
            
            // 设置 hartids
            std::vector<size_t> hartids = {0};
            cfg.hartids = hartids;
            
            // 创建内存
            mems_.clear();
            mems_.reserve(mem_layout.size());
            for (const auto &mem_cfg : mem_layout) {
                mems_.push_back(std::make_pair(mem_cfg.get_base(), new mem_t(mem_cfg.get_size())));
            }
            
            // 如果没有固件文件，创建一个简单的固件
            std::vector<std::string> htif_args = args;
            
            // 调试模块配置
            debug_module_config_t dm_config;
            
            // 创建 sim_t 实例
            // 注意：我们使用简化的参数，实际实现可能需要更完整的参数解析
            std::vector<device_factory_sargs_t> plugin_device_factories;
            const char* log_path = nullptr;
            bool dtb_enabled = true;
            const char* dtb_file = nullptr;
            bool socket_enabled = false;
            FILE* cmd_file = nullptr;
            std::optional<unsigned long long> instruction_limit;
            
            // 创建 sim_t 实例
            sim_t* sim = new sim_t(&cfg, false, mems_, plugin_device_factories, htif_args,
                                  dm_config, log_path, dtb_enabled, dtb_file,
                                  socket_enabled, cmd_file, instruction_limit);
            
            // 创建 mailbox 设备
            if (mailbox_base_ != 0) {
                if (debug_) {
                    std::cout << "[SpikeWrapper] Creating mailbox device for address 0x" 
                              << std::hex << mailbox_base_ << std::dec << std::endl;
                }
                
                try {
                    // 创建 mailbox 设备实例
                    mailbox_dev_ = std::make_shared<mailbox_t>(sim);
                    
                    if (debug_) {
                        std::cout << "[SpikeWrapper] Mailbox device created for address 0x" 
                                  << std::hex << mailbox_base_ 
                                  << " with size 0x" << mailbox_dev_->size() << std::dec << std::endl;
                    }
                    
                    // 将 mailbox 设备直接添加到 Spike 实例
                    // 在 Spike 构造函数之外添加设备到总线
                    sim->add_device(mailbox_base_, mailbox_dev_);
                    
                    if (debug_) {
                        std::cout << "[SpikeWrapper] Mailbox device added to Spike at address 0x" 
                                  << std::hex << mailbox_base_ << std::dec << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[SpikeWrapper] Error creating or adding mailbox device: " << e.what() << std::endl;
                }
            } else {
                if (debug_) {
                    std::cout << "[SpikeWrapper] mailbox_base_ is 0, not adding mailbox device" << std::endl;
                }
            }
            
            // UART 设备已经在 riscv-isa-sim 的 sim.cc:121 中添加
            // 地址为 0x10000000，固件可以直接使用
            if (debug_) {
                std::cout << "[SpikeWrapper] UART device already exists at address 0x10000000 (from riscv-isa-sim)" << std::endl;
            }
            
            // 加载固件到内存（如果提供了固件文件）
            if (!firmware_path_.empty()) {
                std::ifstream file(firmware_path_, std::ios::binary);
                if (file.good()) {
                    // 获取文件大小
                    file.seekg(0, std::ios::end);
                    size_t file_size = file.tellg();
                    file.seekg(0, std::ios::beg);
                    
                    // 读取文件内容
                    std::vector<char> buffer(file_size);
                    file.read(buffer.data(), file_size);
                    
                    // 将固件写入内存（从 0x80000000 开始）
                    // 注意：这里简化了固件加载，实际实现可能需要更复杂的处理
                    if (!mems_.empty() && file_size <= mems_[0].second->size()) {
                        // 将固件写入内存
                        // 注意：这里简化了，实际应该批量写入以提高性能
                        for (size_t i = 0; i < file_size; i++) {
                            uint8_t byte = static_cast<uint8_t>(buffer[i]);
                            mems_[0].second->store(mems_[0].first + i, 1, &byte);
                        }
                        
                        if (debug_) {
                            std::cout << "[SpikeWrapper] Loaded firmware (" << file_size 
                                      << " bytes) to memory at 0x" << std::hex << mems_[0].first 
                                      << std::dec << std::endl;
                        }
                    } else {
                        std::cerr << "[SpikeWrapper] Firmware too large for memory or no memory available" << std::endl;
                    }
                }
            }
            
            // 设置调试模式
            sim->set_debug(debug_);
            
            // 配置日志
            sim->configure_log(false, false);
            
            if (debug_) {
                std::cout << "[SpikeWrapper] Spike instance created successfully" << std::endl;
            }
            
            return std::unique_ptr<sim_t>(sim);
            
        } catch (const std::exception& e) {
            std::cerr << "[SpikeWrapper] Error creating Spike instance: " << e.what() << std::endl;
            return nullptr;
        }
    }
    
    // Spike 运行线程
    void run_spike() {
        if (debug_) {
            std::cout << "[SpikeWrapper] Spike thread started" << std::endl;
        }
        
        try {
            // 运行 Spike 模拟器
            if (sim_) {
                if (debug_) {
                    std::cout << "[SpikeWrapper] Running Spike simulation..." << std::endl;
                }
                
                // 不运行完整的模拟器，因为 sim_->run() 需要外部 spike 可执行文件
                // mailbox_t 设备会处理命令，模拟固件的行为
                if (debug_) {
                    std::cout << "[SpikeWrapper] Spike instance ready, mailbox device will handle commands" << std::endl;
                }
                
                // 保持线程运行
                while (running_) {
                    std::this_thread::sleep_for(100ms);
                }
                
                if (debug_) {
                    std::cout << "[SpikeWrapper] Spike simulation thread completed" << std::endl;
                }
            } else {
                std::cerr << "[SpikeWrapper] No Spike instance" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "[SpikeWrapper] Error in Spike thread: " << e.what() << std::endl;
        }
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        cv_.notify_all();
        
        if (debug_) {
            std::cout << "[SpikeWrapper] Spike thread stopped" << std::endl;
        }
    }
    

    
    // 等待 mailbox 就绪
    bool wait_for_mailbox_ready() {
        if (!mailbox_dev_) {
            return false;
        }
        
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(command_timeout_ms_);
        
        while (true) {
            uint32_t status = mailbox_dev_->get_status();
            
            // 检查是否就绪且不忙
            if ((status & 0x00000001) && !(status & 0x00000002)) {  // READY 且不 BUSY
                return true;
            }
            
            auto now = std::chrono::steady_clock::now();
            if (now - start > timeout) {
                return false;  // 超时
            }
            
            // 短暂休眠避免过于频繁的轮询
            std::this_thread::sleep_for(std::chrono::microseconds(poll_interval_us_));
        }
    }
    
    // 成员变量
    std::string firmware_path_;
    uint64_t mailbox_base_;
    
    std::unique_ptr<sim_t> sim_;
    std::thread spike_thread_;
    std::vector<std::pair<reg_t, abstract_mem_t*>> mems_;  // 内存对象
    std::shared_ptr<mailbox_t> mailbox_dev_;  // mailbox 设备
    
    std::atomic<bool> running_;
    std::atomic<bool> debug_;
    std::atomic<uint32_t> poll_interval_us_;
    std::atomic<uint32_t> command_timeout_ms_;
    
    mutable std::mutex mutex_;
    mutable std::mutex command_mutex_;
    mutable std::mutex callback_mutex_;
    std::condition_variable cv_;
    
    CommandCallback command_callback_;
    std::function<void(uint32_t)> error_callback_;
};

// SpikeWrapper 公共接口实现
SpikeWrapper::SpikeWrapper(const std::string& firmware_path, uint64_t mailbox_base)
    : impl_(std::make_unique<Impl>(firmware_path, mailbox_base)) {}

SpikeWrapper::~SpikeWrapper() = default;

bool SpikeWrapper::start(const std::vector<std::string>& args) {
    return impl_->start(args);
}

void SpikeWrapper::stop() {
    impl_->stop();
}

bool SpikeWrapper::is_running() const {
    return impl_->is_running();
}

bool SpikeWrapper::wait(uint32_t timeout_ms) {
    return impl_->wait(timeout_ms);
}

uint32_t SpikeWrapper::send_hello() {
    return impl_->send_hello();
}

uint32_t SpikeWrapper::send_hi() {
    return impl_->send_hi();
}

uint32_t SpikeWrapper::send_vector_load(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    return impl_->send_vector_load(data_addr, data_size, vector_config);
}

uint32_t SpikeWrapper::send_vector_store(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    return impl_->send_vector_store(data_addr, data_size, vector_config);
}

uint32_t SpikeWrapper::send_vector_compute(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    return impl_->send_vector_compute(data_addr, data_size, vector_config);
}

uint32_t SpikeWrapper::send_softmax(uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    return impl_->send_softmax(data_addr, data_size, vector_config);
}

uint32_t SpikeWrapper::send_command(uint32_t command, uint64_t data_addr,
                                   uint32_t data_size, uint64_t vector_config) {
    return impl_->send_command(command, data_addr, data_size, vector_config);
}

uint32_t SpikeWrapper::get_mailbox_status() const {
    return impl_->get_mailbox_status();
}

bool SpikeWrapper::is_mailbox_ready() const {
    return impl_->is_mailbox_ready();
}

bool SpikeWrapper::is_mailbox_busy() const {
    return impl_->is_mailbox_busy();
}

bool SpikeWrapper::has_error() const {
    return impl_->has_error();
}

uint32_t SpikeWrapper::get_error_code() const {
    return impl_->get_error_code();
}

void SpikeWrapper::set_command_callback(CommandCallback callback) {
    impl_->set_command_callback(std::move(callback));
}

void SpikeWrapper::set_error_callback(std::function<void(uint32_t)> callback) {
    impl_->set_error_callback(std::move(callback));
}

void SpikeWrapper::set_poll_interval(uint32_t interval_us) {
    impl_->set_poll_interval(interval_us);
}

void SpikeWrapper::set_command_timeout(uint32_t timeout_ms) {
    impl_->set_command_timeout(timeout_ms);
}

void SpikeWrapper::set_debug(bool enable) {
    impl_->set_debug(enable);
}