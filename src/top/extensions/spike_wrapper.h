#ifndef _SPIKE_WRAPPER_H
#define _SPIKE_WRAPPER_H

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <functional>
#include <vector>

#include "mailbox.h"

// 前向声明
class sim_t;
class mailbox_t;

/**
 * @brief Spike 包装器类
 * 
 * 提供对 Spike 模拟器的 C++ 包装，集成 mailbox 通信功能。
 */
class SpikeWrapper {
public:
    /**
     * @brief 构造函数
     * @param firmware_path 固件文件路径
     * @param mailbox_base mailbox 基地址
     */
    explicit SpikeWrapper(const std::string& firmware_path = "", 
                         uint64_t mailbox_base = MAILBOX_BASE);
    
    /**
     * @brief 析构函数
     */
    ~SpikeWrapper();
    
    // 禁止拷贝和赋值
    SpikeWrapper(const SpikeWrapper&) = delete;
    SpikeWrapper& operator=(const SpikeWrapper&) = delete;
    
    /**
     * @brief 启动 Spike 模拟器
     * @param args 额外的命令行参数
     * @return 是否启动成功
     */
    bool start(const std::vector<std::string>& args = {});
    
    /**
     * @brief 停止 Spike 模拟器
     */
    void stop();
    
    /**
     * @brief 检查模拟器是否正在运行
     * @return 运行状态
     */
    bool is_running() const;
    
    /**
     * @brief 等待模拟器完成
     * @param timeout_ms 超时时间（毫秒，0 表示无限等待）
     * @return 是否成功等待
     */
    bool wait(uint32_t timeout_ms = 0);
    
    // Mailbox 通信接口
    
    /**
     * @brief 发送 HELLO 命令
     * @return 命令执行结果
     */
    uint32_t send_hello();
    
    /**
     * @brief 发送 HI 命令
     * @return 命令执行结果
     */
    uint32_t send_hi();
    
    /**
     * @brief 发送向量加载命令
     * @param data_addr 数据地址
     * @param data_size 数据大小（字节）
     * @param vector_config 向量配置
     * @return 命令执行结果
     */
    uint32_t send_vector_load(uint64_t data_addr, uint32_t data_size, 
                             uint64_t vector_config = 0);
    
    /**
     * @brief 发送向量存储命令
     * @param data_addr 数据地址
     * @param data_size 数据大小（字节）
     * @param vector_config 向量配置
     * @return 命令执行结果
     */
    uint32_t send_vector_store(uint64_t data_addr, uint32_t data_size,
                              uint64_t vector_config = 0);
    
    /**
     * @brief 发送向量计算命令
     * @param data_addr 数据地址
     * @param data_size 数据大小（字节）
     * @param vector_config 向量配置
     * @return 命令执行结果
     */
    uint32_t send_vector_compute(uint64_t data_addr, uint32_t data_size,
                                uint64_t vector_config = 0);
    
    /**
     * @brief 发送Softmax计算命令
     * @param data_addr 数据地址
     * @param data_size 数据大小（字节）
     * @param vector_config 向量配置
     * @return 命令执行结果
     */
    uint32_t send_softmax(uint64_t data_addr, uint32_t data_size,
                         uint64_t vector_config = 0);
    
    /**
     * @brief 发送自定义命令
     * @param command 命令代码
     * @param data_addr 数据地址
     * @param data_size 数据大小
     * @param vector_config 向量配置
     * @return 命令执行结果
     */
    uint32_t send_command(uint32_t command, uint64_t data_addr = 0,
                         uint32_t data_size = 0, uint64_t vector_config = 0);
    
    // 状态查询
    
    /**
     * @brief 获取 mailbox 状态
     * @return 状态寄存器值
     */
    uint32_t get_mailbox_status() const;
    
    /**
     * @brief 检查 mailbox 是否就绪
     * @return 就绪状态
     */
    bool is_mailbox_ready() const;
    
    /**
     * @brief 检查 mailbox 是否忙
     * @return 忙状态
     */
    bool is_mailbox_busy() const;
    
    /**
     * @brief 检查是否有错误
     * @return 错误状态
     */
    bool has_error() const;
    
    /**
     * @brief 获取错误码
     * @return 错误码
     */
    uint32_t get_error_code() const;
    
    // 回调函数支持
    
    using CommandCallback = std::function<void(uint32_t command, uint32_t response)>;
    
    /**
     * @brief 设置命令完成回调
     * @param callback 回调函数
     */
    void set_command_callback(CommandCallback callback);
    
    /**
     * @brief 设置错误回调
     * @param callback 回调函数
     */
    void set_error_callback(std::function<void(uint32_t error_code)> callback);
    
    // 配置选项
    
    /**
     * @brief 设置轮询间隔（微秒）
     * @param interval_us 轮询间隔
     */
    void set_poll_interval(uint32_t interval_us);
    
    /**
     * @brief 设置命令超时（毫秒）
     * @param timeout_ms 超时时间
     */
    void set_command_timeout(uint32_t timeout_ms);
    
    /**
     * @brief 启用/禁用调试输出
     * @param enable 是否启用
     */
    void set_debug(bool enable);
    
private:
    // 内部实现类
    class Impl;
    std::unique_ptr<Impl> impl_;
};



#endif // _SPIKE_WRAPPER_H
