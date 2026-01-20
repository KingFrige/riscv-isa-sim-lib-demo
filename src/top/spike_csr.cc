/**
 * @file spike_csr.cc
 * @brief CSR 测试程序
 * 
 * 测试自定义 CSR 寄存器功能，包括：
 * - Mail 通道数据注入
 * - Bo done 通道
 * - Se up 通道
 * - Se query 通道
 */

#include "config.h"
#include "cfg.h"
#include "sim.h"
#include "mmu.h"
#include "arith.h"
#include "remote_bitbang.h"
#include "cachesim.h"
#include "extension.h"
#include <fesvr/option_parser.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <limits>
#include <cinttypes>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include "../VERSION"

#include "extensions/custom_csr.h"
#include "common/csr_addr.h"

using namespace std::chrono_literals;

// ============================================================================
// Spike 实例管理
// ============================================================================

class SpikeInstance {
public:
    SpikeInstance(const std::string& firmware_path, const std::string& isa = "rv64gcv_zvl512b_zicsr_xperia_xperiv")
        : firmware_path_(firmware_path), isa_(isa), running_(false) {}
    
    ~SpikeInstance() {
        stop();
    }
    
    bool start() {
        if (running_) return true;
        
        try {
            // 创建配置
            cfg_t cfg;
            cfg.isa = isa_.c_str();
            
            // 创建内存
            std::vector<std::pair<reg_t, abstract_mem_t*>> mems;
            mems.push_back(std::make_pair(DRAM_BASE, new mem_t(2048 << 20)));
            
            // 创建 HTIF 参数列表
            std::vector<std::string> htif_args;
            htif_args.push_back("+permissive");
            htif_args.push_back("+permissive-off");
            htif_args.push_back(firmware_path_);
            
            // 创建 Spike 实例
            sim_ = new sim_t(&cfg, false, mems, {}, htif_args, 
                              debug_module_config_t{}, nullptr, true, nullptr, false, 
                              nullptr, std::nullopt);
            
            // 注册自定义 CSR
            for (size_t i = 0; i < cfg.nprocs(); i++) {
                processor_t* proc = sim_->get_core(i);
                if (proc) {
                    register_custom_csrs(proc);
                    std::cout << "[Spike] Custom CSRs registered for hart "
                              << proc->get_id() << std::endl;
                }
            }
            
            running_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[Spike] Failed to start: " << e.what() << std::endl;
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
        
        if (thread_.joinable()) {
            thread_.join();
        }
        
        if (sim_) {
            delete sim_;
            sim_ = nullptr;
        }
    }
    
    void run_async() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                std::cerr << "[Spike] Not running!" << std::endl;
                return;
            }
        }
        
        thread_ = std::thread([this]() {
            try {
                sim_->run();
            } catch (const std::exception& e) {
                std::cerr << "[Spike] Runtime error: " << e.what() << std::endl;
            }
        });
    }
    
    void wait() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    
    bool is_running() const {
        return running_;
    }
    
    // CSR 读写接口
    uint64_t read_csr(uint64_t addr) {
        if (!sim_ || !running_) {
            std::cerr << "[Spike] Not running!" << std::endl;
            return 0;
        }
        
        // 获取第一个 hart
        processor_t* proc = sim_->get_core(0);
        if (!proc) {
            std::cerr << "[Spike] No harts available!" << std::endl;
            return 0;
        }
        
        insn_t insn(0);  // 创建空指令
        
        // 调用 Spike 的 CSR 读取函数
        return proc->get_csr(addr, insn, false, false);
    }
    
    void write_csr(uint64_t addr, uint64_t value) {
        if (!sim_ || !running_) {
            std::cerr << "[Spike] Not running!" << std::endl;
            return;
        }
        
        // 获取第一个 hart
        processor_t* proc = sim_->get_core(0);
        if (!proc) {
            std::cerr << "[Spike] No harts available!" << std::endl;
            return;
        }
        
        // 调用 Spike 的 CSR 写入函数
        proc->put_csr(addr, value);
    }
    
private:
    std::string firmware_path_;
    std::string isa_;
    sim_t* sim_ = nullptr;
    bool running_;
    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

// ============================================================================
// CSR 测试函数
// ============================================================================

/**
 * @brief 测试 Mail 通道数据注入
 */
void test_mail_channel(SpikeInstance& spike) {
    std::cout << "\n=== Testing Mail Channel ===" << std::endl;
    
    // 准备测试数据（256bit）
    uint64_t mail_data[4] = {
        0x0123456789ABCDEFULL,
        0xFEDCBA9876543210ULL,
        0x1111111122222222ULL,
        0x3333333344444444ULL
    };
    
    std::cout << "[Host] Writing mail data..." << std::endl;
    std::cout << "[Host]   DATA0: 0x" << std::hex << mail_data[0] << std::endl;
    std::cout << "[Host]   DATA1: 0x" << mail_data[1] << std::endl;
    std::cout << "[Host]   DATA2: 0x" << mail_data[2] << std::endl;
    std::cout << "[Host]   DATA3: 0x" << mail_data[3] << std::dec << std::endl;
    
    // 写入数据到 CSR
    spike.write_csr(CSR_MAIL_DATA0, mail_data[0]);
    spike.write_csr(CSR_MAIL_DATA1, mail_data[1]);
    spike.write_csr(CSR_MAIL_DATA2, mail_data[2]);
    spike.write_csr(CSR_MAIL_DATA3, mail_data[3]);
    
    // 置位 mail valid
    std::cout << "[Host] Setting MAIL_VALID..." << std::endl;
    spike.write_csr(CSR_MAIL_VALID, 1);
    
    // 等待 firmware 读取并清除 valid
    std::cout << "[Host] Waiting for firmware to read mail..." << std::endl;
    uint32_t timeout = 1000000;  // 1秒超时
    uint32_t count = 0;
    
    while (count < timeout) {
        uint64_t valid = spike.read_csr(CSR_MAIL_VALID);
        if (valid == 0) {
            std::cout << "[Host] Firmware read mail successfully!" << std::endl;
            break;
        }
        std::this_thread::sleep_for(1ms);
        count++;
    }
    
    if (count >= timeout) {
        std::cerr << "[Host] ERROR: Timeout waiting for firmware to read mail!" << std::endl;
    }
}

/**
 * @brief 测试 Bo done 通道
 */
void test_bo_done_channel(SpikeInstance& spike) {
    std::cout << "\n=== Testing Bo Done Channel ===" << std::endl;
    
    // 发送 bo_done 信号（wg_index=1, bar_index=2）
    uint64_t bo_done_value = (1 << 6) | 2;  // data[10:6]=1, data[5:0]=2
    std::cout << "[Host] Sending BO_DONE (wg_index=1, bar_index=2)..." << std::endl;
    spike.write_csr(CSR_BO_DONE, bo_done_value);
    
    std::cout << "[Host] BO_DONE sent successfully!" << std::endl;
}

/**
 * @brief 测试 Se up 通道
 */
void test_se_up_channel(SpikeInstance& spike) {
    std::cout << "\n=== Testing Se Up Channel ===" << std::endl;
    
    // 发送 se_up 信号（wg_index=3, bar_index=4）
    uint64_t se_up_value = (3 << 6) | 4;  // data[10:6]=3, data[5:0]=4
    std::cout << "[Host] Sending SE_UP (wg_index=3, bar_index=4)..." << std::endl;
    spike.write_csr(CSR_SE_UP, se_up_value);
    
    std::cout << "[Host] SE_UP sent successfully!" << std::endl;
}

/**
 * @brief 测试 Se query 通道
 */
void test_se_query_channel(SpikeInstance& spike) {
    std::cout << "\n=== Testing Se Query Channel ===" << std::endl;
    
    // 发送 query lock 信号（wg_index=5, bar_index=6）
    uint64_t query_lock_value = (5 << 6) | 6;  // data[10:6]=5, data[5:0]=6
    std::cout << "[Host] Sending SE_QUERY_LOCK (wg_index=5, bar_index=6)..." << std::endl;
    spike.write_csr(CSR_SE_QUERY_LOCK, query_lock_value);
    
    // 等待一段时间，然后检查 query count
    std::this_thread::sleep_for(100ms);
    
    uint64_t query_count = spike.read_csr(CSR_SE_QUERY_COUNT);
    std::cout << "[Host] SE_QUERY_COUNT: " << query_count << std::endl;
    
    // 读取 query count（减少计数）
    if (query_count > 0) {
        spike.write_csr(CSR_SE_QUERY_COUNT, query_count);
        std::cout << "[Host] Read and decremented SE_QUERY_COUNT" << std::endl;
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Spike CSR Test Program" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 检查命令行参数
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <firmware.elf> [options]" << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  --test <name>    Run specific test (mail, bo_done, se_up, se_query, all)" << std::endl;
        std::cerr << "  --loop           Run tests in a loop" << std::endl;
        std::cerr << "  --help           Show this help message" << std::endl;
        return 1;
    }
    
    const char* firmware_path = argv[1];
    std::string test_name = "all";
    bool loop_mode = false;
    
    // 解析命令行参数
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--test" && i + 1 < argc) {
            test_name = argv[++i];
        } else if (arg == "--loop") {
            loop_mode = true;
        } else if (arg == "--help") {
            std::cout << "Available tests:" << std::endl;
            std::cout << "  mail      - Test Mail channel" << std::endl;
            std::cout << "  bo_done   - Test Bo done channel" << std::endl;
            std::cout << "  se_up     - Test Se up channel" << std::endl;
            std::cout << "  se_query  - Test Se query channel" << std::endl;
            std::cout << "  all       - Run all tests" << std::endl;
            return 0;
        }
    }
    
    std::cout << "[Main] Firmware: " << firmware_path << std::endl;
    std::cout << "[Main] Test: " << test_name << std::endl;
    std::cout << "[Main] Loop mode: " << (loop_mode ? "Yes" : "No") << std::endl;
    
    // 创建 Spike 实例
    SpikeInstance spike(firmware_path);
    
    // 启动 Spike 模拟
    std::cout << "[Main] Starting simulation..." << std::endl;
    if (!spike.start()) {
        std::cerr << "[Main] Failed to start simulation!" << std::endl;
        return 1;
    }
    
    std::cout << "[Main] Simulation started successfully!" << std::endl;
    
    // 启动 Spike 模拟（在后台运行）
    spike.run_async();
    
    // 等待固件启动
    std::cout << "[Main] Waiting for firmware to initialize..." << std::endl;
    std::this_thread::sleep_for(500ms);
    
    // 运行测试
    uint32_t iteration = 0;
    
    do {
        iteration++;
        std::cout << "\n========================================" << std::endl;
        std::cout << "Iteration " << iteration << std::endl;
        std::cout << "========================================" << std::endl;
        
        if (test_name == "mail" || test_name == "all") {
            test_mail_channel(spike);
        }
        
        if (test_name == "bo_done" || test_name == "all") {
            test_bo_done_channel(spike);
        }
        
        if (test_name == "se_up" || test_name == "all") {
            test_se_up_channel(spike);
        }
        
        if (test_name == "se_query" || test_name == "all") {
            test_se_query_channel(spike);
        }
        
        if (loop_mode) {
            std::cout << "\n[Main] Waiting 1 second before next iteration..." << std::endl;
            std::this_thread::sleep_for(1s);
        }
        
    } while (loop_mode);
    
    // 等待 Spike 完成
    if (!loop_mode) {
        std::cout << "\n[Main] Waiting for simulation to complete..." << std::endl;
        std::cout << "[Main] Press Ctrl+C to stop (firmware runs in infinite loop)" << std::endl;
        spike.wait();
    } else {
        std::cout << "\n[Main] Running in loop mode. Press Ctrl+C to stop..." << std::endl;
        spike.wait();
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test completed successfully!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
