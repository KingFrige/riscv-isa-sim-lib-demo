/**
 * @file spike_csr_client.cc
 * @brief CSR 测试端（使用 ZMQ 通信）
 * 
 * 使用 ZMQ 协议向 CSR 功能端发送命令，执行 CSR 操作测试。
 * 支持的命令：
 * - READ_CSR: 读取 CSR 寄存器
 * - WRITE_CSR: 写入 CSR 寄存器
 * - MAIL_CHANNEL: Mail 通道测试
 * - BO_DONE: Bo done 通道测试
 * - SE_UP: Se up 通道测试
 * - SE_QUERY: Se query 通道测试
 * - QUIT: 退出服务器
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
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
// ZMQ 客户端
// ============================================================================

class ZMQClient {
public:
    ZMQClient(const std::string& endpoint = "tcp://localhost:5555")
        : endpoint_(endpoint), context_(1), socket_(context_, ZMQ_REQ) {}
    
    void connect() {
        socket_.connect(endpoint_);
        std::cout << "[Client] Connected to " << endpoint_ << std::endl;
    }
    
    json send_command(const json& request) {
        // 发送请求
        std::string request_str = request.dump();
        zmq::message_t msg(request_str.c_str(), request_str.size());
        socket_.send(msg);
        
        // 接收响应
        zmq::message_t reply;
        socket_.recv(&reply);
        
        // 解析响应
        std::string reply_str(static_cast<char*>(reply.data()), reply.size());
        json response;
        
        try {
            response = json::parse(reply_str);
        } catch (const json::parse_error& e) {
            std::cerr << "[Client] Failed to parse response: " << reply_str << std::endl;
            json error;
            error["status"] = "error";
            error["message"] = "Failed to parse response";
            return error;
        }
        
        return response;
    }
    
    uint64_t read_csr(uint64_t addr) {
        json request;
        request["cmd"] = CMD_READ_CSR;
        request["addr"] = addr;
        
        json response = send_command(request);
        
        if (response["status"] == "error") {
            std::cerr << "[Client] Error: " << response["message"].get<std::string>() << std::endl;
            return 0;
        }
        
        return response["value"].get<uint64_t>();
    }
    
    bool write_csr(uint64_t addr, uint64_t value) {
        json request;
        request["cmd"] = CMD_WRITE_CSR;
        request["addr"] = addr;
        request["value"] = value;
        
        json response = send_command(request);
        
        if (response["status"] == "error") {
            std::cerr << "[Client] Error: " << response["message"].get<std::string>() << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool send_mail(uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3) {
        json request;
        request["cmd"] = CMD_MAIL_CHANNEL;
        request["data0"] = data0;
        request["data1"] = data1;
        request["data2"] = data2;
        request["data3"] = data3;
        
        json response = send_command(request);
        
        if (response["status"] == "error") {
            std::cerr << "[Client] Error: " << response["message"].get<std::string>() << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool send_bo_done(uint32_t wg_index, uint32_t bar_index) {
        json request;
        request["cmd"] = CMD_BO_DONE;
        request["wg_index"] = wg_index;
        request["bar_index"] = bar_index;
        
        json response = send_command(request);
        
        if (response["status"] == "error") {
            std::cerr << "[Client] Error: " << response["message"].get<std::string>() << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool send_se_up(uint32_t wg_index, uint32_t bar_index) {
        json request;
        request["cmd"] = CMD_SE_UP;
        request["wg_index"] = wg_index;
        request["bar_index"] = bar_index;
        
        json response = send_command(request);
        
        if (response["status"] == "error") {
            std::cerr << "[Client] Error: " << response["message"].get<std::string>() << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool send_se_query(uint32_t wg_index, uint32_t bar_index, uint64_t& query_count) {
        json request;
        request["cmd"] = CMD_SE_QUERY;
        request["wg_index"] = wg_index;
        request["bar_index"] = bar_index;
        
        json response = send_command(request);
        
        if (response["status"] == "error") {
            std::cerr << "[Client] Error: " << response["message"].get<std::string>() << std::endl;
            return false;
        }
        
        query_count = response["query_count"].get<uint64_t>();
        return true;
    }
    
    bool quit() {
        json request;
        request["cmd"] = CMD_QUIT;
        
        json response = send_command(request);
        
        if (response["status"] == "error") {
            std::cerr << "[Client] Error: " << response["message"].get<std::string>() << std::endl;
            return false;
        }
        
        return true;
    }
    
private:
    std::string endpoint_;
    zmq::context_t context_;
    zmq::socket_t socket_;
};

// ============================================================================
// 测试函数
// ============================================================================

void test_mail_channel(ZMQClient& client) {
    std::cout << "\n=== Testing Mail Channel ===" << std::endl;
    
    uint64_t mail_data[4] = {
        0x0123456789ABCDEFULL,
        0xFEDCBA9876543210ULL,
        0x1111111122222222ULL,
        0x3333333344444444ULL
    };
    
    std::cout << "[Client] Sending mail data..." << std::endl;
    std::cout << "[Client]   DATA0: 0x" << std::hex << mail_data[0] << std::endl;
    std::cout << "[Client]   DATA1: 0x" << mail_data[1] << std::endl;
    std::cout << "[Client]   DATA2: 0x" << mail_data[2] << std::endl;
    std::cout << "[Client]   DATA3: 0x" << mail_data[3] << std::dec << std::endl;
    
    if (client.send_mail(mail_data[0], mail_data[1], mail_data[2], mail_data[3])) {
        std::cout << "[Client] Mail sent successfully!" << std::endl;
    } else {
        std::cerr << "[Client] Failed to send mail!" << std::endl;
    }
}

void test_bo_done_channel(ZMQClient& client) {
    std::cout << "\n=== Testing Bo Done Channel ===" << std::endl;
    
    uint32_t wg_index = 1;
    uint32_t bar_index = 2;
    
    std::cout << "[Client] Sending BO_DONE (wg_index=" << wg_index 
              << ", bar_index=" << bar_index << ")..." << std::endl;
    
    if (client.send_bo_done(wg_index, bar_index)) {
        std::cout << "[Client] BO_DONE sent successfully!" << std::endl;
    } else {
        std::cerr << "[Client] Failed to send BO_DONE!" << std::endl;
    }
}

void test_se_up_channel(ZMQClient& client) {
    std::cout << "\n=== Testing Se Up Channel ===" << std::endl;
    
    uint32_t wg_index = 3;
    uint32_t bar_index = 4;
    
    std::cout << "[Client] Sending SE_UP (wg_index=" << wg_index 
              << ", bar_index=" << bar_index << ")..." << std::endl;
    
    if (client.send_se_up(wg_index, bar_index)) {
        std::cout << "[Client] SE_UP sent successfully!" << std::endl;
    } else {
        std::cerr << "[Client] Failed to send SE_UP!" << std::endl;
    }
}

void test_se_query_channel(ZMQClient& client) {
    std::cout << "\n=== Testing Se Query Channel ===" << std::endl;
    
    uint32_t wg_index = 5;
    uint32_t bar_index = 6;
    
    std::cout << "[Client] Sending SE_QUERY_LOCK (wg_index=" << wg_index 
              << ", bar_index=" << bar_index << ")..." << std::endl;
    
    uint64_t query_count = 0;
    if (client.send_se_query(wg_index, bar_index, query_count)) {
        std::cout << "[Client] SE_QUERY sent successfully!" << std::endl;
        std::cout << "[Client] SE_QUERY_COUNT: " << query_count << std::endl;
    } else {
        std::cerr << "[Client] Failed to send SE_QUERY!" << std::endl;
    }
}

void test_csr_read_write(ZMQClient& client) {
    std::cout << "\n=== Testing CSR Read/Write ===" << std::endl;
    
    // 测试写入和读取 MAIL_DATA0
    uint64_t test_value = 0xDEADBEEFCAFEBABEULL;
    std::cout << "[Client] Writing 0x" << std::hex << test_value 
              << " to CSR_MAIL_DATA0..." << std::dec << std::endl;
    
    if (!client.write_csr(CSR_MAIL_DATA0, test_value)) {
        std::cerr << "[Client] Failed to write CSR!" << std::endl;
        return;
    }
    
    uint64_t read_value = client.read_csr(CSR_MAIL_DATA0);
    std::cout << "[Client] Read 0x" << std::hex << read_value 
              << " from CSR_MAIL_DATA0" << std::dec << std::endl;
    
    if (read_value == test_value) {
        std::cout << "[Client] CSR read/write test PASSED!" << std::endl;
    } else {
        std::cerr << "[Client] CSR read/write test FAILED!" << std::endl;
        std::cerr << "[Client] Expected: 0x" << std::hex << test_value << std::endl;
        std::cerr << "[Client] Got: 0x" << read_value << std::dec << std::endl;
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Spike CSR Client (ZMQ)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 检查命令行参数
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test> [endpoint]" << std::endl;
        std::cerr << "Tests:" << std::endl;
        std::cerr << "  mail      - Test Mail channel" << std::endl;
        std::cerr << "  bo_done   - Test Bo done channel" << std::endl;
        std::cerr << "  se_up     - Test Se up channel" << std::endl;
        std::cerr << "  se_query  - Test Se query channel" << std::endl;
        std::cerr << "  csr_rw    - Test CSR read/write" << std::endl;
        std::cerr << "  all       - Run all tests" << std::endl;
        std::cerr << "  quit      - Quit server" << std::endl;
        std::cerr << "  endpoint: ZMQ endpoint (default: tcp://localhost:5555)" << std::endl;
        return 1;
    }
    
    std::string test_name = argv[1];
    std::string endpoint = (argc > 2) ? argv[2] : "tcp://localhost:5555";
    
    std::cout << "[Main] Test: " << test_name << std::endl;
    std::cout << "[Main] Endpoint: " << endpoint << std::endl;
    
    // 创建客户端并连接
    ZMQClient client(endpoint);
    client.connect();
    
    // 等待服务器准备就绪
    std::this_thread::sleep_for(100ms);
    
    // 运行测试
    if (test_name == "mail" || test_name == "all") {
        test_mail_channel(client);
    }
    
    if (test_name == "bo_done" || test_name == "all") {
        test_bo_done_channel(client);
    }
    
    if (test_name == "se_up" || test_name == "all") {
        test_se_up_channel(client);
    }
    
    if (test_name == "se_query" || test_name == "all") {
        test_se_query_channel(client);
    }
    
    if (test_name == "csr_rw" || test_name == "all") {
        test_csr_read_write(client);
    }
    
    if (test_name == "quit") {
        std::cout << "[Main] Sending quit command..." << std::endl;
        if (client.quit()) {
            std::cout << "[Main] Quit command sent successfully!" << std::endl;
        } else {
            std::cerr << "[Main] Failed to send quit command!" << std::endl;
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test completed!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}