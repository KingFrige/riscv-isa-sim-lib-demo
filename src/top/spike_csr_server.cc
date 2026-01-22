/**
 * @file spike_csr_server.cc
 * @brief CSR 功能端（使用 ZMQ 通信）
 * 
 * 使用 ZMQ 协议接受来自客户端的 CSR 操作命令，执行后返回结果。
 * 支持的命令：
 * - READ_CSR: 读取 CSR 寄存器
 * - WRITE_CSR: 写入 CSR 寄存器
 * - MAIL_CHANNEL: Mail 通道测试
 * - BO_DONE: Bo done 通道测试
 * - SE_UP: Se up 通道测试
 * - SE_QUERY: Se query 通道测试
 * - QUIT: 退出服务器
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
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "../VERSION"

#include "extensions/custom_csr.h"
#include "common/csr_addr.h"

using namespace std::chrono_literals;
using json = nlohmann::json;

// ============================================================================
// 命令定义
// ============================================================================

enum Command {
    CMD_READ_CSR = 1,
    CMD_WRITE_CSR = 2,
    CMD_MAIL_CHANNEL = 3,
    CMD_BO_DONE = 4,
    CMD_SE_UP = 5,
    CMD_SE_QUERY = 6,
    CMD_QUIT = 99
};

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
        
        processor_t* proc = sim_->get_core(0);
        if (!proc) {
            std::cerr << "[Spike] No harts available!" << std::endl;
            return 0;
        }
        
        insn_t insn(0);
        return proc->get_csr(addr, insn, false, false);
    }
    
    void write_csr(uint64_t addr, uint64_t value) {
        if (!sim_ || !running_) {
            std::cerr << "[Spike] Not running!" << std::endl;
            return;
        }
        
        processor_t* proc = sim_->get_core(0);
        if (!proc) {
            std::cerr << "[Spike] No harts available!" << std::endl;
            return;
        }
        
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
// ZMQ 服务器
// ============================================================================

class ZMQServer {
public:
    ZMQServer(SpikeInstance& spike, const std::string& endpoint = "tcp://*:5555")
        : spike_(spike), endpoint_(endpoint), context_(1), socket_(context_, ZMQ_REP), running_(false) {}
    
    void start() {
        socket_.bind(endpoint_);
        std::cout << "[ZMQ] Server started on " << endpoint_ << std::endl;
    }
    
    void stop() {
        running_ = false;
    }
    
    void run() {
        running_ = true;
        while (running_) {
            try {
                // 接收请求
                zmq::message_t request;
                socket_.recv(&request);
                
                // 解析 JSON 请求
                std::string request_str(static_cast<char*>(request.data()), request.size());
                json request_json;
                
                try {
                    request_json = json::parse(request_str);
                } catch (const json::parse_error& e) {
                    std::cerr << "[ZMQ] Failed to parse request: " << request_str << std::endl;
                    send_error("Invalid JSON");
                    continue;
                }
                
                // 处理命令
                json response_json = handle_command(request_json);
                
                // 发送响应
                std::string response_str = response_json.dump();
                zmq::message_t reply(response_str.c_str(), response_str.size());
                socket_.send(reply);
                
            } catch (const std::exception& e) {
                std::cerr << "[ZMQ] Error: " << e.what() << std::endl;
                send_error(e.what());
            }
        }
    }
    
private:
    json handle_command(const json& request) {
        json response;
        response["status"] = "success";
        
        if (!request.contains("cmd")) {
            response["status"] = "error";
            response["message"] = "Missing 'cmd' field";
            return response;
        }
        
        int cmd = request["cmd"].get<int>();
        
        switch (cmd) {
            case CMD_READ_CSR: {
                if (!request.contains("addr")) {
                    response["status"] = "error";
                    response["message"] = "Missing 'addr' field";
                    break;
                }
                uint64_t addr = request["addr"].get<uint64_t>();
                uint64_t value = spike_.read_csr(addr);
                response["value"] = value;
                break;
            }
            
            case CMD_WRITE_CSR: {
                if (!request.contains("addr") || !request.contains("value")) {
                    response["status"] = "error";
                    response["message"] = "Missing 'addr' or 'value' field";
                    break;
                }
                uint64_t addr = request["addr"].get<uint64_t>();
                uint64_t value = request["value"].get<uint64_t>();
                spike_.write_csr(addr, value);
                break;
            }
            
            case CMD_MAIL_CHANNEL: {
                if (!request.contains("data0") || !request.contains("data1") ||
                    !request.contains("data2") || !request.contains("data3")) {
                    response["status"] = "error";
                    response["message"] = "Missing data fields";
                    break;
                }
                uint64_t data0 = request["data0"].get<uint64_t>();
                uint64_t data1 = request["data1"].get<uint64_t>();
                uint64_t data2 = request["data2"].get<uint64_t>();
                uint64_t data3 = request["data3"].get<uint64_t>();
                
                spike_.write_csr(CSR_MAIL_DATA0, data0);
                spike_.write_csr(CSR_MAIL_DATA1, data1);
                spike_.write_csr(CSR_MAIL_DATA2, data2);
                spike_.write_csr(CSR_MAIL_DATA3, data3);
                spike_.write_csr(CSR_MAIL_VALID, 1);
                
                // 等待固件读取
                uint32_t timeout = 1000000;
                uint32_t count = 0;
                while (count < timeout) {
                    uint64_t valid = spike_.read_csr(CSR_MAIL_VALID);
                    if (valid == 0) {
                        response["message"] = "Mail sent successfully";
                        break;
                    }
                    std::this_thread::sleep_for(1ms);
                    count++;
                }
                
                if (count >= timeout) {
                    response["status"] = "error";
                    response["message"] = "Timeout waiting for firmware";
                }
                break;
            }
            
            case CMD_BO_DONE: {
                if (!request.contains("wg_index") || !request.contains("bar_index")) {
                    response["status"] = "error";
                    response["message"] = "Missing wg_index or bar_index";
                    break;
                }
                uint32_t wg_index = request["wg_index"].get<uint32_t>();
                uint32_t bar_index = request["bar_index"].get<uint32_t>();
                uint64_t value = (wg_index << 6) | bar_index;
                spike_.write_csr(CSR_BO_DONE, value);
                response["message"] = "BO_DONE sent";
                break;
            }
            
            case CMD_SE_UP: {
                if (!request.contains("wg_index") || !request.contains("bar_index")) {
                    response["status"] = "error";
                    response["message"] = "Missing wg_index or bar_index";
                    break;
                }
                uint32_t wg_index = request["wg_index"].get<uint32_t>();
                uint32_t bar_index = request["bar_index"].get<uint32_t>();
                uint64_t value = (wg_index << 6) | bar_index;
                spike_.write_csr(CSR_SE_UP, value);
                response["message"] = "SE_UP sent";
                break;
            }
            
            case CMD_SE_QUERY: {
                if (!request.contains("wg_index") || !request.contains("bar_index")) {
                    response["status"] = "error";
                    response["message"] = "Missing wg_index or bar_index";
                    break;
                }
                uint32_t wg_index = request["wg_index"].get<uint32_t>();
                uint32_t bar_index = request["bar_index"].get<uint32_t>();
                uint64_t value = (wg_index << 6) | bar_index;
                spike_.write_csr(CSR_SE_QUERY_LOCK, value);
                
                std::this_thread::sleep_for(100ms);
                uint64_t count = spike_.read_csr(CSR_SE_QUERY_COUNT);
                response["query_count"] = count;
                break;
            }
            
            case CMD_QUIT: {
                response["message"] = "Server shutting down";
                std::cout << "[ZMQ] Quit command received" << std::endl;
                running_ = false;
                break;
            }
            
            default:
                response["status"] = "error";
                response["message"] = "Unknown command";
                break;
        }
        
        return response;
    }
    
    void send_error(const std::string& message) {
        json response;
        response["status"] = "error";
        response["message"] = message;
        
        std::string response_str = response.dump();
        zmq::message_t reply(response_str.c_str(), response_str.size());
        socket_.send(reply);
    }
    
private:
    SpikeInstance& spike_;
    std::string endpoint_;
    zmq::context_t context_;
    zmq::socket_t socket_;
    bool running_;
};

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Spike CSR Server (ZMQ)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 检查命令行参数
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <firmware.elf> [endpoint]" << std::endl;
        std::cerr << "  endpoint: ZMQ endpoint (default: tcp://*:5555)" << std::endl;
        return 1;
    }
    
    const char* firmware_path = argv[1];
    std::string endpoint = (argc > 2) ? argv[2] : "tcp://*:5555";
    
    std::cout << "[Main] Firmware: " << firmware_path << std::endl;
    std::cout << "[Main] Endpoint: " << endpoint << std::endl;
    
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
    std::this_thread::sleep_for(500ms);
    
    // 创建并启动 ZMQ 服务器
    ZMQServer server(spike, endpoint);
    server.start();
    
    std::cout << "[Main] Server ready, waiting for commands..." << std::endl;
    
    // 运行服务器
    server.run();
    
    // 停止 Spike 实例
    std::cout << "[Main] Stopping Spike instance..." << std::endl;
    spike.stop();
    
    std::cout << "[Main] Server stopped successfully!" << std::endl;
    
    return 0;
}