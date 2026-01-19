# RVV Mailbox device 设计文档

## 概述
RVV Mailbox 是一个用于 RISC-V 向量扩展（RVV）的通信机制，允许主机（host）与 Spike 模拟器中的固件进行通信。该机制主要用于：
1. 主机向固件发送命令和数据
2. 固件向主机返回结果
3. 支持向量数据的传输

## 设计目标
1. 提供简单、高效的通信接口
2. 支持向量数据的批量传输
3. 与现有的 Spike 调试 ROM 架构兼容
4. 提供轮询和中断两种通信模式

## 架构设计

### 内存映射寄存器
RVV Mailbox 使用内存映射 I/O（MMIO）方式，在特定的内存地址上定义一组寄存器：

| 地址偏移 | 寄存器名称 | 宽度 | 描述 |
|------- --|-----------|------|------|
| 0x0000   | MAILBOX_STATUS | 32位 | 邮箱状态寄存器 |
| 0x0008   | MAILBOX_COMMAND | 32位 | 命令寄存器 |
| 0x000C   | MAILBOX_RESPONSE | 32位 | 响应寄存器 |
| 0x0020   | MAILBOX_DATA0 | 64位 | 数据寄存器 |
| 0x0028   | MAILBOX_DATA1 | 64位 | 数据寄存器 |
| 0x0030   | MAILBOX_DATA2 | 64位 | 数据寄存器 |
| 0x0038   | MAILBOX_DATA3 | 64位 | 数据寄存器 |

### 寄存器详细说明

#### MAILBOX_STATUS (0x0000)
```
位域：
[0]    READY      - 1: 邮箱就绪，可以接收新命令
[1]    BUSY       - 1: 邮箱正在处理命令
[2]    ERROR      - 1: 处理命令时发生错误
[3]    VECTOR_MODE - 1: 向量模式使能
[31:4] RESERVED   - 保留位
```

#### MAILBOX_COMMAND (0x0004)
```
命令编码：
0x00000001 - HELLO: 执行 hello 函数
0x00000002 - HI:    执行 hi 函数
0x00000010 - VECTOR_LOAD: 加载向量数据
0x00000011 - VECTOR_STORE: 存储向量数据
0x00000012 - VECTOR_COMPUTE: 执行向量计算
其他值保留
```

#### MAILBOX_DATA0-3
- 64位物理地址，指向数据传输的内存区域

#### MAILBOX_RESPONSE
- 32位响应值，由固件设置
- 成功时通常返回0，错误时返回错误码


## 通信协议

### 命令执行流程
1. **主机发送命令**：
   - 检查 MAILBOX_STATUS.READY = 1
   - 设置 MAILBOX_COMMAND
   - 设置相关参数寄存器（DATA_ADDR, DATA_SIZE, VECTOR_CONFIG）
   - 写入任意值到 MAILBOX_STATUS 触发命令执行（BUSY 位自动置1）

2. **固件处理**：
   - 检测到 MAILBOX_STATUS.BUSY = 1
   - 读取命令和参数
   - 执行相应操作
   - 设置 MAILBOX_RESPONSE
   - 清除 MAILBOX_STATUS.BUSY 位，设置 READY 位

3. **主机接收响应**：
   - 轮询 MAILBOX_STATUS.BUSY，直到为0
   - 读取 MAILBOX_RESPONSE 获取结果

### 向量数据传输
对于向量操作，使用以下流程：

1. **向量加载**（VECTOR_LOAD）：
   - 主机设置向量数据到内存中
   - 设置 DATA_ADDR 指向数据，DATA_SIZE 为元素个数
   - 设置 VECTOR_CONFIG 配置向量参数
   - 发送 VECTOR_LOAD 命令
   - 固件将数据加载到向量寄存器

2. **向量存储**（VECTOR_STORE）：
   - 主机发送 VECTOR_STORE 命令
   - 固件将向量寄存器数据写入内存
   - 主机从 DATA_ADDR 读取结果

## 集成到 Spike

### 修改点
1. **添加 Mailbox 设备**：
   - 在 `riscv/devices.h` 和 `riscv/devices.cc` 中添加 mailbox 设备类
   - 实现内存映射寄存器访问

2. **扩展调试 ROM**：
   - 在 `debug_rom/debug_rom.S` 中添加 mailbox 处理代码
   - 实现命令分发和处理逻辑

3. **固件实现**：
   - 实现 hello 和 hi 函数
   - 实现向量数据搬运功能

### 内存映射地址
建议将 RVV Mailbox 映射到以下地址：
- 基地址：0x4000_0000（可配置）

## 示例使用

### 简单函数调用（hello/hi）
```c
// 主机端代码
void call_hello(void) {
    while (!(mailbox->status & MAILBOX_READY)); // 等待就绪
    mailbox->command = MAILBOX_CMD_HELLO;
    mailbox->status = 0; // 触发执行
    while (mailbox->status & MAILBOX_BUSY); // 等待完成
    printf("Response: %d\n", mailbox->response);
}
```

### 向量操作
```c
// 向量加法示例
void vector_add_example(void) {
    float src1[1024], src2[1024], dst[1024];
    
    // 准备数据
    for (int i = 0; i < 1024; i++) {
        src1[i] = i * 1.0f;
        src2[i] = i * 2.0f;
    }
    
    // 加载第一个向量
    mailbox->data_addr = (uint64_t)src1;
    mailbox->data_size = 1024;
    mailbox->command = MAILBOX_CMD_VECTOR_LOAD;
    mailbox->status = 0;
    while (mailbox->status & MAILBOX_BUSY);
    
    // 加载第二个向量并计算（假设命令支持）
    mailbox->data_addr = (uint64_t)src2;
    mailbox->command = MAILBOX_CMD_VECTOR_COMPUTE;
    mailbox->status = 0;
    while (mailbox->status & MAILBOX_BUSY);
    
    // 存储结果
    mailbox->data_addr = (uint64_t)dst;
    mailbox->command = MAILBOX_CMD_VECTOR_STORE;
    mailbox->status = 0;
    while (mailbox->status & MAILBOX_BUSY);
}
```

## 错误处理

### 错误码定义
```
0x00000000 - 成功
0x00000001 - 无效命令
0x00000002 - 参数错误
0x00000003 - 内存访问错误
0x00000004 - 向量配置错误
0x00000005 - 未实现的功能
```

### 错误恢复
- 发生错误时，固件设置 MAILBOX_STATUS.ERROR 位
- 主机应读取 MAILBOX_RESPONSE 获取具体错误码
- 错误状态需要主机显式清除

## 性能考虑
1. **批量传输**：支持大块向量数据传输，减少通信开销
2. **零拷贝**：直接使用物理地址，避免数据复制
3. **并行处理**：支持命令流水线处理

## 测试计划
1. 单元测试：测试每个命令的正确性
2. 集成测试：测试与 Spike 的集成
3. 性能测试：测试向量数据传输性能
4. 兼容性测试：测试与现有调试功能的兼容性

## 未来扩展
1. 支持更多向量操作命令
2. 添加中断支持
3. 支持多核通信
4. 添加性能计数器和调试功能
