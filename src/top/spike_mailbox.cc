/**
 * @file unified_test.cc
 * @brief 统一测试程序
 * 
 * 合并所有测试功能到一个程序中，避免代码重复。
 * 支持 HELLO、HI、向量操作和 Softmax 命令测试。
 */

#include "spike_wrapper.h"
#include "mailbox.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include <cstring>
#include <functional>

using namespace std::chrono_literals;

// ============================================================================
// 通用工具函数和类
// ============================================================================

/**
 * @brief 命令完成回调函数（通用）
 */
void on_command_complete(uint32_t command, uint32_t response) {
    std::cout << "[Callback] Command 0x" << std::hex << command 
              << " completed with response 0x" << response << std::dec << std::endl;
    
    switch (response) {
        case mailbox_t::MAILBOX_SUCCESS:
            std::cout << "[Callback] SUCCESS" << std::endl;
            break;
        case mailbox_t::MAILBOX_ERR_INVALID_CMD:
            std::cout << "[Callback] INVALID COMMAND" << std::endl;
            break;
        case mailbox_t::MAILBOX_ERR_INVALID_PARAM:
            std::cout << "[Callback] INVALID PARAMETER" << std::endl;
            break;
        case mailbox_t::MAILBOX_ERR_MEM_ACCESS:
            std::cout << "[Callback] MEMORY ACCESS ERROR" << std::endl;
            break;
        case mailbox_t::MAILBOX_ERR_VECTOR_CONFIG:
            std::cout << "[Callback] VECTOR CONFIG ERROR" << std::endl;
            break;
        case mailbox_t::MAILBOX_ERR_NOT_IMPLEMENTED:
            std::cout << "[Callback] NOT IMPLEMENTED" << std::endl;
            break;
        default:
            std::cout << "[Callback] UNKNOWN ERROR" << std::endl;
            break;
    }
}

/**
 * @brief 错误回调函数（通用）
 */
void on_error(uint32_t error_code) {
    std::cerr << "[Error] Error code: 0x" << std::hex << error_code << std::dec << std::endl;
}

/**
 * @brief 测试数据缓冲区类
 */
class TestBuffer {
public:
    TestBuffer(size_t size, const std::string& name = "Test") 
        : size_(size), name_(name) {
        data_.resize(size);
        // 初始化测试数据
        for (size_t i = 0; i < size; i++) {
            data_[i] = static_cast<uint8_t>(i % 256);
        }
    }
    
    // 使用特定数据初始化
    TestBuffer(const std::vector<float>& float_data, const std::string& name = "Test")
        : size_(float_data.size() * sizeof(float)), name_(name) {
        data_.resize(size_);
        // 将浮点数据复制到缓冲区
        std::memcpy(data_.data(), float_data.data(), size_);
    }
    
    uint64_t address() const {
        // 在实际系统中，这里需要返回实际的内存地址
        // 这里返回一个模拟地址
        return 0x80001000;
    }
    
    uint32_t size() const {
        return static_cast<uint32_t>(size_);
    }
    
    const uint8_t* data() const {
        return data_.data();
    }
    
    void print_info() const {
        std::cout << name_ << " buffer:" << std::endl;
        std::cout << "  Address: 0x" << std::hex << address() << std::dec << std::endl;
        std::cout << "  Size: " << size() << " bytes" << std::endl;
        
        // 打印前几个字节
        if (size_ > 0) {
            std::cout << "  First 16 bytes: ";
            for (int i = 0; i < 16 && i < size_; i++) {
                std::cout << std::hex << (int)data_[i] << " ";
            }
            std::cout << std::dec << std::endl;
        }
    }
    
private:
    size_t size_;
    std::string name_;
    std::vector<uint8_t> data_;
};

/**
 * @brief 测试结果结构体
 */
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    uint32_t response;
    
    TestResult(const std::string& n, bool p, const std::string& m = "", uint32_t r = 0)
        : name(n), passed(p), message(m), response(r) {}
    
    void print() const {
        std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << name;
        if (!message.empty()) {
            std::cout << ": " << message;
        }
        if (response != 0) {
            std::cout << " (response: 0x" << std::hex << response << std::dec << ")";
        }
        std::cout << std::endl;
    }
};

// ============================================================================
// 测试函数
// ============================================================================

/**
 * @brief 测试 HELLO 命令
 */
TestResult test_hello_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing HELLO command ---" << std::endl;
    
    uint32_t response = wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_HELLO);
    
    if (response == mailbox_t::MAILBOX_SUCCESS) {
        return TestResult("HELLO command", true, "Executed successfully");
    } else {
        return TestResult("HELLO command", false, "Failed", response);
    }
}

/**
 * @brief 测试 HI 命令
 */
TestResult test_hi_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing HI command ---" << std::endl;
    
    uint32_t response = wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_HI);
    
    if (response == mailbox_t::MAILBOX_SUCCESS) {
        return TestResult("HI command", true, "Executed successfully");
    } else {
        return TestResult("HI command", false, "Failed", response);
    }
}

/**
 * @brief 测试向量加载命令
 */
TestResult test_vector_load_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing VECTOR LOAD command ---" << std::endl;
    
    // 创建测试缓冲区
    TestBuffer buffer(1024, "Vector Load Test");
    buffer.print_info();
    
    uint32_t response = wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_VECTOR_LOAD, buffer.address(), buffer.size(), 0);
    
    if (response == mailbox_t::MAILBOX_SUCCESS) {
        return TestResult("VECTOR LOAD command", true, "Executed successfully");
    } else {
        return TestResult("VECTOR LOAD command", false, "Failed", response);
    }
}

/**
 * @brief 测试向量存储命令
 */
TestResult test_vector_store_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing VECTOR STORE command ---" << std::endl;
    
    // 创建测试缓冲区
    TestBuffer buffer(512, "Vector Store Test");
    buffer.print_info();
    
    uint32_t response = wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_VECTOR_STORE, buffer.address(), buffer.size(), 0);
    
    if (response == mailbox_t::MAILBOX_SUCCESS) {
        return TestResult("VECTOR STORE command", true, "Executed successfully");
    } else {
        return TestResult("VECTOR STORE command", false, "Failed", response);
    }
}

/**
 * @brief 测试向量计算命令
 */
TestResult test_vector_compute_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing VECTOR COMPUTE command ---" << std::endl;
    
    // 创建测试缓冲区
    TestBuffer buffer(256, "Vector Compute Test");
    buffer.print_info();
    
    uint32_t response = wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_VECTOR_COMPUTE, buffer.address(), buffer.size(), 0);
    
    if (response == mailbox_t::MAILBOX_SUCCESS) {
        return TestResult("VECTOR COMPUTE command", true, "Executed successfully");
    } else {
        return TestResult("VECTOR COMPUTE command", false, "Failed", response);
    }
}

/**
 * @brief 测试 Softmax 命令
 */
TestResult test_softmax_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing SOFTMAX command ---" << std::endl;

    // 创建测试数据（5个浮点数）
    std::vector<float> test_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    TestBuffer buffer(test_data, "Softmax Test");
    buffer.print_info();

    std::cout << "Input data: [";
    for (size_t i = 0; i < test_data.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << test_data[i];
    }
    std::cout << "]" << std::endl;

    // 计算参考结果（用于验证）
    std::vector<float> reference_result(test_data.size());
    float max_val = test_data[0];
    for (float val : test_data) {
        if (val > max_val) max_val = val;
    }

    float sum_exp = 0.0f;
    for (float val : test_data) {
        sum_exp += std::exp(val - max_val);
    }

    for (size_t i = 0; i < test_data.size(); i++) {
        reference_result[i] = std::exp(test_data[i] - max_val) / sum_exp;
    }

    std::cout << "Reference result: [";
    for (size_t i = 0; i < reference_result.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << reference_result[i];
    }
    std::cout << "]" << std::endl;

    uint32_t response = wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_SOFTMAX, buffer.address(), buffer.size(), 0);

    if (response == mailbox_t::MAILBOX_SUCCESS) {
        return TestResult("SOFTMAX command", true, "Executed successfully");
    } else {
        return TestResult("SOFTMAX command", false, "Failed", response);
    }
}

/**
 * @brief 测试 EXP 命令
 */
TestResult test_exp_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing EXP command ---" << std::endl;

    // 创建测试数据（5个浮点数）
    std::vector<float> test_data = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f};
    TestBuffer buffer(test_data, "EXP Test");
    buffer.print_info();

    std::cout << "Input data: [";
    for (size_t i = 0; i < test_data.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << test_data[i];
    }
    std::cout << "]" << std::endl;

    // 计算参考结果（e^x）
    std::vector<float> reference_result(test_data.size());
    for (size_t i = 0; i < test_data.size(); i++) {
        reference_result[i] = std::exp(test_data[i]);
    }

    std::cout << "Reference result (e^x): [";
    for (size_t i = 0; i < reference_result.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << reference_result[i];
    }
    std::cout << "]" << std::endl;

    uint32_t response = wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_EXP, buffer.address(), buffer.size(), 0);

    if (response == mailbox_t::MAILBOX_SUCCESS) {
        return TestResult("EXP command", true, "Executed successfully");
    } else {
        return TestResult("EXP command", false, "Failed", response);
    }
}

/**
 * @brief 测试 QUANT 命令
 */
TestResult test_quant_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing QUANT command ---" << std::endl;

    // 创建测试数据（5个浮点数）
    std::vector<float> test_data = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f};
    TestBuffer buffer(test_data, "QUANT Test");
    buffer.print_info();

    std::cout << "Input data: [";
    for (size_t i = 0; i < test_data.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << test_data[i];
    }
    std::cout << "]" << std::endl;

    // 计算参考结果（量化后的值）
    std::vector<float> reference_result(test_data.size());
    for (size_t i = 0; i < test_data.size(); i++) {
        // 简单的量化：乘以 2 并四舍五入
        reference_result[i] = std::round(test_data[i] * 2.0f) / 2.0f;
    }

    std::cout << "Reference result (quantized): [";
    for (size_t i = 0; i < reference_result.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << reference_result[i];
    }
    std::cout << "]" << std::endl;

    uint32_t response = wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_QUANT, buffer.address(), buffer.size(), 0);

    if (response == mailbox_t::MAILBOX_SUCCESS) {
        return TestResult("QUANT command", true, "Executed successfully");
    } else {
        return TestResult("QUANT command", false, "Failed", response);
    }
}

/**
 * @brief 测试无效命令
 */
TestResult test_invalid_command(SpikeWrapper& wrapper) {
    std::cout << "\n--- Testing INVALID command ---" << std::endl;
    
    uint32_t response = wrapper.send_command(0xFFFFFFFF);
    
    // 无效命令应该返回错误
    if (response == mailbox_t::MAILBOX_ERR_INVALID_CMD) {
        return TestResult("INVALID command", true, "Correctly rejected");
    } else {
        return TestResult("INVALID command", false, "Unexpected response", response);
    }
}

/**
 * @brief 性能测试
 */
TestResult test_performance(SpikeWrapper& wrapper) {
    std::cout << "\n--- Performance Test ---" << std::endl;
    
    const int num_iterations = 10;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; i++) {
        wrapper.mailbox_send_command(mailbox_t::MAILBOX_CMD_HELLO);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Executed " << num_iterations << " HELLO commands in " 
              << duration.count() << " ms" << std::endl;
    std::cout << "Average time per command: " 
              << (duration.count() * 1000.0 / num_iterations) << " us" << std::endl;
    
    return TestResult("Performance test", true, 
                     std::to_string(num_iterations) + " commands in " + 
                     std::to_string(duration.count()) + " ms");
}

// ============================================================================
// 测试运行器
// ============================================================================

/**
 * @brief 测试用例定义
 */
struct TestCase {
    std::string name;
    std::function<TestResult(SpikeWrapper&)> test_func;
    bool enabled;
    
    TestCase(const std::string& n, std::function<TestResult(SpikeWrapper&)> f, bool e = true)
        : name(n), test_func(f), enabled(e) {}
};

/**
 * @brief 运行所有测试
 */
int run_all_tests(const std::vector<TestCase>& test_cases,
                  const std::string& firmware_path = "",
                  const std::vector<std::string>& spike_args = {}) {    std::cout << "========================================" << std::endl;
    std::cout << "Spike Wrapper Unified Test Program" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        // 创建 SpikeWrapper 实例（不指定固件路径，因为ELF会通过参数传入）
        SpikeWrapper wrapper(firmware_path, MAILBOX_BASE);
        
        // 设置回调函数
        wrapper.set_command_callback(on_command_complete);
        wrapper.set_error_callback(on_error);
        
        // 启用调试输出
        wrapper.set_debug(true);
        
        // 设置命令超时为 2 秒
        wrapper.set_command_timeout(2000);
        
        // 启动 Spike 模拟器（传递参数）
        std::cout << "Starting Spike simulator..." << std::endl;
        if (!wrapper.start(spike_args)) {
            std::cerr << "Failed to start Spike simulator" << std::endl;
            return 1;
        }
        
        // 等待 Spike 初始化
        std::cout << "Waiting for Spike initialization..." << std::endl;
        std::this_thread::sleep_for(1s);
        
        // 如果提供了固件路径，使用ELF加载功能（如果SpikeWrapper支持的话）
        if (!firmware_path.empty()) {
            std::cout << "Loading program from: " << firmware_path << std::endl;
        }

        // 运行所有测试用例
        std::vector<TestResult> results;
        int passed_count = 0;
        int total_count = 0;
        
        for (const auto& test_case : test_cases) {
            if (!test_case.enabled) {
                continue;
            }
            
            total_count++;
            std::cout << "\n[" << total_count << "/" << test_cases.size() 
                      << "] Running test: " << test_case.name << std::endl;
            
            try {
                TestResult result = test_case.test_func(wrapper);
                results.push_back(result);
                
                if (result.passed) {
                    passed_count++;
                }
                
                // 短暂延迟，避免命令冲突
                std::this_thread::sleep_for(100ms);
                
            } catch (const std::exception& e) {
                TestResult result(test_case.name, false, 
                                 std::string("Exception: ") + e.what());
                results.push_back(result);
            }
        }
        
        // 停止 Spike 模拟器
        std::cout << "\nStopping Spike simulator..." << std::endl;
        wrapper.stop();
        
        // 等待停止完成
        if (wrapper.wait(3000)) {
            std::cout << "Spike simulator stopped successfully" << std::endl;
        } else {
            std::cerr << "Failed to stop Spike simulator" << std::endl;
        }
        
        // 打印测试结果摘要
        std::cout << "\n========================================" << std::endl;
        std::cout << "Test Results Summary" << std::endl;
        std::cout << "========================================" << std::endl;
        
        for (const auto& result : results) {
            result.print();
        }
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Total: " << passed_count << "/" << total_count 
                  << " tests passed (" 
                  << (total_count > 0 ? (passed_count * 100 / total_count) : 100)
                  << "%)" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return (passed_count == total_count) ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

// ============================================================================
// 主函数
// ============================================================================

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    // 定义所有测试用例
    std::vector<TestCase> test_cases = {
        {"HELLO command", test_hello_command},
        {"HI command", test_hi_command},
        {"Vector Load", test_vector_load_command},
        {"Vector Store", test_vector_store_command},
        {"Vector Compute", test_vector_compute_command},
        {"Softmax", test_softmax_command},
        {"EXP", test_exp_command},
        {"QUANT", test_quant_command},
        {"Invalid command", test_invalid_command},
        {"Performance", test_performance}
    };
    
    // 检查命令行参数
    std::string firmware_path = "";
    bool run_all = true;
    std::vector<std::string> spike_args;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [firmware.elf] [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --help, -h       Show this help message" << std::endl;
            std::cout << "  --firmware PATH  Specify firmware path" << std::endl;
            std::cout << "  --list           List all test cases" << std::endl;
            std::cout << "  --test NAME      Run specific test case" << std::endl;
            std::cout << "  -l               Enable execution log" << std::endl;
            std::cout << "  --log=PATH       Enable execution log to file" << std::endl;
            std::cout << "  --log-commits    Enable commit log" << std::endl;
            std::cout << "  --debug          Enable debug output" << std::endl;
            return 0;
        } else if (arg == "--list") {
            std::cout << "Available test cases:" << std::endl;
            for (size_t i = 0; i < test_cases.size(); i++) {
                std::cout << "  " << (i + 1) << ". " << test_cases[i].name << std::endl;
            }
            return 0;
        } else if (arg == "--test" && i + 1 < argc) {
            std::string test_name = argv[++i];
            run_all = false;
            
            // 启用指定的测试用例
            for (auto& test_case : test_cases) {
                test_case.enabled = (test_case.name == test_name);
            }
        } else if (arg == "--firmware" && i + 1 < argc) {
            firmware_path = argv[++i];
        } else if (arg == "-l" || arg == "--log-commits" || arg == "--debug") {
            // 这些是 Spike 参数，传递给 SpikeWrapper
            spike_args.push_back(arg);
        } else if (arg.find("--log=") == 0) {
            // 日志文件参数
            spike_args.push_back(arg);
        } else if (arg[0] != '-') {
            // 假设第一个非选项参数是固件路径
            if (firmware_path.empty()) {
                firmware_path = arg;
            }
        }
    }
    
    // 如果没有明确指定固件路径，可以尝试使用命令行参数作为固件路径
    if (firmware_path.empty() && argc > 1 && argv[1][0] != '-') {
        firmware_path = argv[1];
    }
    
    if (run_all) {
        std::cout << "Running all test cases..." << std::endl;
    } else {
        std::cout << "Running specific test case..." << std::endl;
    }
    
    if (!firmware_path.empty()) {
        std::cout << "Using firmware: " << firmware_path << std::endl;
    } else {
        std::cout << "No firmware file specified" << std::endl;
    }
    
    if (!spike_args.empty()) {
        std::cout << "Spike arguments: ";
        for (const auto& arg : spike_args) {
            std::cout << arg << " ";
        }
        std::cout << std::endl;
    }
    
    return run_all_tests(test_cases, firmware_path, spike_args);
}
