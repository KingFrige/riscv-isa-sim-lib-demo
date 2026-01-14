# Spike Mailbox 功能说明

## 概述

Spike Mailbox 是一个用于与 RISC-V 模拟器进行通信的框架，允许主机程序与运行在 Spike 模拟器中的固件进行通信。它通过内存映射的寄存器提供了一种 mailbox 通信机制。

## 架构组件

### 固件 (Firmware)
- 位于 `src/top/firmware/mailbox/`
- 包含在 Spike 模拟器中运行的 RISC-V 代码
- 实现了 mailbox 命令处理逻辑
- 支持 HELLO、HI、VECTOR_LOAD、VECTOR_STORE、VECTOR_COMPUTE、SOFTMAX 等命令

### mailbox 设备
- 位于 `src/top/extensions/mailbox.cc`
- 在 Spike 模拟器中实现的内存映射设备
- 处理来自固件的 mailbox 命令
- 提供状态、命令、数据地址等寄存器接口

### Spike 包装器
- 位于 `src/top/extensions/spike_wrapper.cc`
- 提供 C++ 接口来控制 Spike 模拟器
- 实现了与 mailbox 设备的通信逻辑

### 测试程序
- 位于 `src/top/spike_mailbox.cc`
- 提供统一的测试框架
- 包含对各种 mailbox 命令的测试用例

## 构建方法

### 构建 mailbox 固件

```bash
# 使用统一构建脚本
./build_all.sh --mailbox-firmware

# 或者单独构建
cd src/top/firmware/mailbox
make PROJECT_ROOT="." FIRMWARE_DIR="." BUILD_DIR="build"
```

### 构建 spike_mailbox 可执行文件

```bash
# 使用统一构建脚本
./build_all.sh --spike-mailbox

# 或者单独构建（需要先构建 Spike 库）
./build_all.sh --spike
./build_all.sh --spike-mailbox
```

## 命令协议

### 寄存器映射

```
MAILBOX_STATUS_OFFSET      = 0x00  // 状态寄存器 (READY, BUSY, ERROR)
MAILBOX_COMMAND_OFFSET     = 0x04  // 命令寄存器
MAILBOX_DATA_ADDR_OFFSET   = 0x08  // 数据地址寄存器 (64位)
MAILBOX_DATA_SIZE_OFFSET   = 0x10  // 数据大小寄存器
MAILBOX_VECTOR_CONFIG_OFFSET = 0x18 // 向量配置寄存器 (64位)
MAILBOX_RESPONSE_OFFSET    = 0x14  // 响应寄存器
```

### 支持的命令

- `MAILBOX_CMD_HELLO (0x00000001)` - 测试命令
- `MAILBOX_CMD_HI (0x00000002)` - 简单响应命令
- `MAILBOX_CMD_VECTOR_LOAD (0x00000010)` - 向量加载命令
- `MAILBOX_CMD_VECTOR_STORE (0x00000011)` - 向量存储命令
- `MAILBOX_CMD_VECTOR_COMPUTE (0x00000012)` - 向量计算命令
- `MAILBOX_CMD_SOFTMAX (0x00000020)` - Softmax 计算命令

### 状态位

- `MAILBOX_STATUS_READY (0x00000001)` - 设备就绪
- `MAILBOX_STATUS_BUSY (0x00000002)` - 设备繁忙
- `MAILBOX_STATUS_ERROR (0x00000004)` - 发生错误

## 使用方法

### 运行测试

```bash
# 运行 mailbox 测试
./build/mailbox/spike_mailbox

# 指定固件路径
./build/mailbox/spike_mailbox --firmware /path/to/firmware.hex

# 运行特定测试
./build/mailbox/spike_mailbox --test "HELLO command"
```

## 构建选项

在 `build_all.sh` 中添加了以下新选项：

- `--mailbox-firmware`: 构建 mailbox 固件
- `--spike-mailbox`: 构建 spike_mailbox 可执行文件
- `--all`: 构建所有组件（包括 mailbox）

## 输出文件

- `build/firmware/mailbox/firmware.hex` - mailbox 固件文件
- `build/mailbox/spike_mailbox` - spike_mailbox 可执行文件

## 实现细节

mailbox 设备实现了完整的命令处理流程：

1. 命令写入命令寄存器
2. 触发命令执行（写入状态寄存器）
3. 固件处理命令
4. 返回响应

## 错误代码

- `MAILBOX_ERR_SUCCESS (0x00000000)` - 成功
- `MAILBOX_ERR_INVALID_CMD (0x00000001)` - 无效命令
- `MAILBOX_ERR_INVALID_PARAM (0x00000002)` - 无效参数
- `MAILBOX_ERR_MEM_ACCESS (0x00000003)` - 内存访问错误
- `MAILBOX_ERR_VECTOR_CONFIG (0x00000004)` - 向量配置错误
- `MAILBOX_ERR_NOT_IMPLEMENTED (0x00000005)` - 未实现