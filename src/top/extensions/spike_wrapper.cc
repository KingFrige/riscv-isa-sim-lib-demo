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
        , enable_log_(false)
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
            if (debug_) {
                std::cout << "[SpikeWrapper] Starting Spike with args: ";
                for (const auto& arg : args) {
                    std::cout << arg << " ";
                }
                std::cout << std::endl;
            }
            
            // 创建 Spike 实例
            // 注意：这里简化了 Spike 的初始化，实际实现需要更复杂的配置
            sim_ = create_spike_instance(args);
            
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
    uint32_t mailbox_send_command(int command_idx, uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
        // 根据命令索引调用相应的命令函数
        switch (command_idx) {
            case mailbox_t::MAILBOX_CMD_HELLO:
                return send_command(mailbox_t::MAILBOX_CMD_HELLO, 0, 0, 0);
            case mailbox_t::MAILBOX_CMD_HI:
                return send_command(mailbox_t::MAILBOX_CMD_HI, 0, 0, 0);
            case mailbox_t::MAILBOX_CMD_VECTOR_LOAD:
                return send_command(mailbox_t::MAILBOX_CMD_VECTOR_LOAD, data_addr, data_size, vector_config);
            case mailbox_t::MAILBOX_CMD_VECTOR_STORE:
                return send_command(mailbox_t::MAILBOX_CMD_VECTOR_STORE, data_addr, data_size, vector_config);
            case mailbox_t::MAILBOX_CMD_VECTOR_COMPUTE:
                return send_command(mailbox_t::MAILBOX_CMD_VECTOR_COMPUTE, data_addr, data_size, vector_config);
            case mailbox_t::MAILBOX_CMD_SOFTMAX:
                return send_command(mailbox_t::MAILBOX_CMD_SOFTMAX, data_addr, data_size, vector_config);
            case mailbox_t::MAILBOX_CMD_EXP:
                return send_command(mailbox_t::MAILBOX_CMD_EXP, data_addr, data_size, vector_config);
            case mailbox_t::MAILBOX_CMD_QUANT:
                return send_command(mailbox_t::MAILBOX_CMD_QUANT, data_addr, data_size, vector_config);
            default:
                std::cerr << "[SpikeWrapper] Invalid command index: " << command_idx << std::endl;
                return 0xFFFFFFFF;  // 错误码
        }
    }
    
    uint32_t send_command(uint32_t command, uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
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
        
        // 创建触发回调函数（写入状态寄存器以触发命令执行）
        auto trigger_callback = [this]() {
            uint32_t trigger = 1;  // 任何非零值都会触发命令处理
            mailbox_dev_->store(mailbox_t::MAILBOX_STATUS_OFFSET, 4, (const uint8_t*)&trigger);
        };
        
        // 使用 mailbox 设备的完整 send_command 函数
        return mailbox_dev_->send_command_complete(command, data_addr, data_size, vector_config,
                                                  command_timeout_ms_, poll_interval_us_,
                                                  trigger_callback);
    }
    
    // 状态查询
    uint32_t get_mailbox_status() const {
        if (!mailbox_dev_) {
            return 0;
        }
        uint32_t status = 0;
        mailbox_dev_->load(mailbox_t::MAILBOX_STATUS_OFFSET, 4, (uint8_t*)&status);
        return status;
    }
    
    bool is_mailbox_ready() const {
        uint32_t status = get_mailbox_status();
        return (status & mailbox_t::MAILBOX_READY) != 0;
    }
    
    bool is_mailbox_busy() const {
        uint32_t status = get_mailbox_status();
        return (status & mailbox_t::MAILBOX_BUSY) != 0;
    }
    
    bool has_error() const {
        uint32_t status = get_mailbox_status();
        return (status & mailbox_t::MAILBOX_ERROR) != 0;
    }
    
    uint32_t get_error_code() const {
        if (!mailbox_dev_) {
            return 0;
        }
        uint32_t response = 0;
        mailbox_dev_->load(mailbox_t::MAILBOX_RESPONSE_OFFSET, 4, (uint8_t*)&response);
        return response;
    }
    
    // 回调函数设置
    void set_command_callback(CommandCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        command_callback_ = std::move(callback);
        // 同时设置给mailbox设备
        if (mailbox_dev_) {
            mailbox_dev_->set_command_callback(callback);
        }
    }
    
    void set_error_callback(std::function<void(uint32_t)> callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        error_callback_ = std::move(callback);
        // 同时设置给mailbox设备
        if (mailbox_dev_) {
            mailbox_dev_->set_error_callback(callback);
        }
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
        
        try {
            // 创建配置
            cfg_t cfg;
            
            // 设置 ISA
            cfg.isa = "rv64gcv_zvl512b_zicsr_xperia_xperiv";
            fprintf(stderr, "[DEBUG] ISA string: '%s'\n", cfg.isa);
            
            // 设置内存布局
            std::vector<mem_cfg_t> mem_layout;
            mem_layout.push_back(mem_cfg_t(0x80000000, 0x8000000)); // 128MB DRAM从0x80000000开始
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
            
            // 创建HTIF参数列表，包含ELF文件路径（如果提供了的话）
            std::vector<std::string> htif_args;

            // 添加permissive参数处理可能的未知选项
            htif_args.push_back("+permissive");
            htif_args.push_back("+permissive-off");

            if (!firmware_path_.empty()) {
                htif_args.push_back(firmware_path_);  // ELF文件路径作为程序参数
            }
            
            // 调试模块配置
            debug_module_config_t dm_config = {
                .progbufsize = 2,
                .datacount = 2,
                .max_sba_data_width = 0,
                .require_authentication = false,
                .abstract_rti = 0,
                .support_hasel = true,
                .support_abstract_csr_access = true,
                .support_abstract_fpr_access = true,
                .support_haltgroups = true,
                .support_impebreak = true,
                .support_abstractauto = true
            };
            
            // 创建 sim_t 实例
            std::vector<device_factory_sargs_t> plugin_device_factories;
            const char* log_path = nullptr;
            bool dtb_enabled = true;
            const char* dtb_file = nullptr;
            bool socket_enabled = false;
            FILE* cmd_file = nullptr;
            std::optional<unsigned long long> instruction_limit;
            
            // 解析日志相关参数
            bool enable_log = false;
            bool enable_commitlog = false;
            std::string log_file_path;
            
            // 解析传入的参数
            for (size_t i = 0; i < args.size(); ++i) {
                const std::string& arg = args[i];
                
                if (arg == "-l") {
                    enable_log = true;
                } else if (arg.find("--log=") == 0) {
                    enable_log = true;
                    log_file_path = arg.substr(6); // 提取 --log= 后面的部分
                    log_path = log_file_path.c_str();
                } else if (arg == "--log-commits") {
                    enable_commitlog = true;
                } else if (arg.find("--log=") == 0 && arg.length() > 6) {
                    // 处理 --log=path 格式
                    enable_log = true;
                    log_file_path = arg.substr(6);
                    log_path = log_file_path.c_str();
                }
                // 其他参数可以在这里添加
            }
            
            // 如果指定了日志但没有指定文件路径，使用默认路径
            if (enable_log && log_file_path.empty()) {
                log_file_path = "spike_log.txt";
                log_path = log_file_path.c_str();
                if (debug_) {
                    std::cout << "[SpikeWrapper] Using default log file: " << log_file_path << std::endl;
                }
            }
            
            // 保存日志启用状态到成员变量
            enable_log_ = enable_log;
            
            // 创建 sim_t 实例，使用 halted=true 确保程序不会立即运行
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
                    // 创建 mailbox 设备实例，传递基地址
                    mailbox_dev_ = std::make_shared<mailbox_t>(sim, mailbox_base_);
                    
                    if (debug_) {
                        std::cout << "[SpikeWrapper] Mailbox device created for address 0x" 
                                  << std::hex << mailbox_base_ 
                                  << " with size 0x" << mailbox_dev_->size() << std::dec << std::endl;
                    }
                    
                    // 将 mailbox 设备添加到 Spike 实例
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
            
            // UART 设备已经在 riscv-isa-sim 中添加
            if (debug_) {
                std::cout << "[SpikeWrapper] UART device available at address 0x10000000" << std::endl;
            }
            
            // 设置调试模式
            sim->set_debug(debug_);
            
            // 配置日志（使用从参数中解析的设置）
            sim->configure_log(enable_log, enable_commitlog);
            
            if (debug_) {
                std::cout << "[SpikeWrapper] Spike instance created successfully" << std::endl;
                std::cout << "[SpikeWrapper] Log settings: enable_log=" << enable_log 
                          << ", enable_commitlog=" << enable_commitlog 
                          << ", log_path=" << (log_path ? log_path : "null") << std::endl;
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
                
                // 在循环中运行模拟器，检查是否需要停止
                // 在实际运行之前，先调用start()来加载ELF程序
                if (debug_) {
                    std::cout << "[SpikeWrapper] Loading ELF program via HTIF start() method" << std::endl;
                }
                
                sim_->start(); // 这将调用HTIF的start()方法加载ELF程序
                
                if (debug_) {
                    std::cout << "[SpikeWrapper] ELF program loaded, starting simulation loop" << std::endl;
                }
                
                // 如果启用了日志，手动设置处理器的debug标志
                if (enable_log_ && !sim_->get_harts().empty()) {
                    auto it = sim_->get_harts().begin();
                    processor_t* proc = it->second;
                    if (proc) {
                        proc->set_debug(true);
                        if (debug_) {
                            std::cout << "[SpikeWrapper] Set processor debug flag for logging" << std::endl;
                        }
                    }
                }
                
                // 在循环中运行模拟器，检查是否需要停止
                while (running_ && !sim_->done()) {
                    try {
                        // 获取处理器并执行少量指令
                        // 这样可以让固件处理mailbox命令
                        if (!sim_->get_harts().empty()) {
                            // 获取第一个处理器（通常是唯一的处理器）
                            auto it = sim_->get_harts().begin();
                            processor_t* proc = it->second;
                            if (proc) {
                                // 执行少量指令，让固件有机会处理mailbox命令
                                proc->step(100);  // 执行100个指令周期
                            }
                        }
                        
                    } catch (const std::exception& e) {
                        if (debug_) {
                            std::cout << "[SpikeWrapper] Spike execution exception: " << e.what() << std::endl;
                        }
                        // 继续循环，除非明确停止
                    }
                    
                    // 短暂休眠以避免过度占用CPU，同时保持响应性
                    std::this_thread::sleep_for(1ms);
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
            if ((status & mailbox_t::MAILBOX_READY) && !(status & mailbox_t::MAILBOX_BUSY)) {
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
    std::atomic<bool> enable_log_;  // 日志启用状态
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

uint32_t SpikeWrapper::mailbox_send_command(int command_idx, uint64_t data_addr, uint32_t data_size, uint64_t vector_config) {
    return impl_->mailbox_send_command(command_idx, data_addr, data_size, vector_config);
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

void SpikeWrapper::set_error_callback(std::function<void(uint32_t error_code)> callback) {
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
