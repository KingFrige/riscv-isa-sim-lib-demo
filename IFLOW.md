# RISC-V ISA 模拟器库集成演示项目

## 项目概述

本项目演示如何将 [RISC-V ISA 模拟器 (Spike)](https://github.com/riscv-software-src/riscv-isa-sim) 作为库使用，并与外部模拟器环境集成。项目已进一步扩展，集成了针对 Llama.cpp 的 RISC-V 向量扩展，专门用于 AI 推理加速的自定义指令集扩展。

目前包含七个主要用例：
1. **C++ 内存模拟器集成** - 将 Spike 与自定义 C++ 内存模拟器连接
2. **SystemC 包装器** - 将 Spike 嵌入 SystemC 环境，创建完整的系统级仿真平台
3. **静态链接 Spike** - 将自定义扩展与 Spike 静态链接，创建独立的可执行文件（已实现）
4. **AI 推理 RISC-V 向量扩展** - 为 AI 推理实现自定义的 exp、softmax、quant 指令
5. **Mailbox 通信框架** - 提供主机程序与模拟器固件之间的通信机制
6. **Fuse 固件测试** - 使用 Mailbox 通信框架测试自定义指令（exp、softmax、quant 等）
7. **自定义 CSR 功能** - 实现自定义控制状态寄存器，支持 Mail、BO、SE 等同步机制
8. **ZMQ 通信框架** - 基于 ZeroMQ 的服务器/客户端通信，支持远程 CSR 访问

此外，项目还包含一个实验性扩展示例 (`src/xperimental`)，展示了如何为 Spike 添加自定义指令扩展。

**当前开发状态**: 项目已完成自定义 CSR (Control and Status Register) 功能实现，包括 Mail、Barrier Event (BO) 和 Synchronization Event (SE) 等高级同步机制，并添加了 ZMQ 通信框架支持远程访问。

## 技术栈

- **核心模拟器**: RISC-V ISA Simulator (Spike)
- **编程语言**: C++ (主程序), C (测试软件), SystemC (系统级建模), RISC-V 汇编
- **构建系统**: GNU Make, Autotools (Spike 子模块), 统一构建脚本 (`build_all.sh`)
- **仿真环境**: SystemC 2.3+ (可选)
- **工具链**: RISC-V GNU 工具链 (需支持 rv64imafdcv 架构)
- **AI 推理**: 集成 BFloat16、MxFP8 等 AI 精度处理
- **通信框架**: ZeroMQ (ZMQ) 用于服务器/客户端通信
- **CSR 支持**: 自定义 CSR 寄存器实现（Mail、BO、SE 通道）

## 目录结构

```
.
├── build/                  # 统一构建输出目录
│   ├── csr/                # CSR 测试构建输出
│   │   ├── firmware/       # CSR 固件
│   │   └── spike_csr       # CSR 测试可执行文件
│   ├── custom/             # 自定义扩展对象文件
│   ├── extensions/         # Spike 扩展对象文件
│   ├── firmware/           # 固件构建输出
│   │   ├── csr/            # CSR 测试固件
│   │   ├── fuse/           # Fuse 固件（Mailbox 通信 + 指令测试）
│   │   ├── insn/           # 指令测试固件
│   │   └── mailbox/        # Mailbox 通信固件
│   ├── insn/               # spike_insn 构建输出
│   ├── mailbox/            # spike_mailbox 构建输出
│   ├── spike/              # Spike 构建缓存
│   ├── spike-install/      # Spike 安装目录
│   ├── zmq/                # ZMQ 服务器/客户端构建输出
│   │   ├── spike_csr_server  # ZMQ 服务器
│   │   └── spike_csr_client  # ZMQ 客户端
│   └── log/                # 测试日志输出
├── docs/                   # 项目文档
│   ├── insn-decode.jpg     # 指令解码图示
│   ├── insn.jpg            # 指令图示
│   ├── rvv_mailbox_dev.md  # Mailbox 设备设计文档
│   ├── rvv-custom-csr.excalidraw # CSR 设计图（Excalidraw 格式）
│   ├── rvv-custom-csr.md   # 自定义 CSR 设计文档
│   ├── rvv-custom-csr.png  # CSR 架构图
│   └── spike_mailbox.README.md # Mailbox 功能说明
├── riscv-isa-sim/          # Spike 子模块 (RISC-V ISA 模拟器)
├── src/                    # 源代码目录
│   ├── common/             # 通用代码
│   │   └── csr_addr.h      # CSR 地址定义
│   ├── cpp/                # C++ 内存模拟器集成
│   │   ├── sw/             # 测试软件
│   │   ├── util/           # 工具函数
│   │   ├── demo_core.cc    # 核心演示逻辑
│   │   ├── memory_simulator.cc # 内存模拟器实现
│   │   ├── main.cc         # 主程序入口
│   │   └── Makefile        # 构建配置
│   ├── systemc/            # SystemC 集成
│   │   ├── memory/         # 内存模型
│   │   ├── turbo/          # 处理器核心包装器
│   │   ├── uncore/         # 非核心逻辑
│   │   ├── util/           # 工具函数
│   │   ├── sw/             # 测试软件
│   │   ├── sc_main.cpp     # SystemC 主程序
│   │   ├── Makefile        # 构建配置
│   │   └── README.md       # 详细使用说明
│   ├── top/                # 静态链接 Spike 集成 + AI 扩展
│   │   ├── common/         # 通用头文件
│   │   │   └── csr_addr.h  # CSR 地址定义
│   │   ├── custom/         # 自定义 RISC-V 扩展算法实现
│   │   │   ├── config.h    # 配置文件
│   │   │   ├── util.c/h    # 工具函数
│   │   │   └── riscv/      # RISC-V 向量扩展实现
│   │   │       ├── BF16.cpp/hpp    # BFloat16 处理单元
│   │   │       ├── custom_expp.cpp/hpp # BF16 e^x 近似计算
│   │   │       ├── MxFp8ActQuant.cpp/hpp # MXFP8 量化核心
│   │   │       └── SoftmaxCore.cpp/hpp # Softmax 计算核心
│   │   ├── extensions/     # 自定义扩展实现
│   │   │   ├── custom_csr.cc/h    # 自定义 CSR 实现
│   │   │   ├── decode_macros.h
│   │   │   ├── extension.h
│   │   │   ├── insn_macros.h
│   │   │   ├── mailbox.cc/h    # Mailbox 设备实现
│   │   │   ├── primitiveTypes.h
│   │   │   ├── specialize.h
│   │   │   ├── spike_wrapper.cc/h # Spike 包装器
│   │   │   ├── util.h
│   │   │   ├── v_ext_macros.h
│   │   │   ├── xperia.cc   # 标量扩展（加法）
│   │   │   └── xperiv.cc   # 向量扩展（加法、乘法及 AI 指令）
│   │   ├── firmware/       # 测试固件
│   │   │   ├── common/     # 通用固件代码
│   │   │   ├── csr/        # CSR 测试固件
│   │   │   │   ├── main.c  # CSR 测试主程序
│   │   │   │   ├── mail_poll.c # Mail 轮询实现
│   │   │   │   └── Makefile # 构建配置
│   │   │   ├── fuse/       # Fuse 固件（Mailbox 通信 + 指令测试）
│   │   │   │   ├── Makefile      # 构建配置
│   │   │   │   ├── README.md     # 说明文档
│   │   │   │   ├── main.c        # 主程序（Mailbox 命令处理）
│   │   │   │   └── test_insn.c   # 指令测试
│   │   │   ├── insn/       # 指令测试固件
│   │   │   │   ├── main.c  # 测试程序（包含 exp/softmax/quant 测试）
│   │   │   │   ├── start.S # 启动代码
│   │   │   │   ├── script.ld # 链接脚本
│   │   │   │   ├── util.c/h # 工具函数
│   │   │   │   └── Makefile # 固件构建配置
│   │   │   ├── mailbox/    # Mailbox 通信固件
│   │   │   │   ├── include/ # 头文件
│   │   │   │   ├── linker/ # 链接脚本
│   │   │   │   ├── src/     # 源代码
│   │   │   │   ├── Makefile # 构建配置
│   │   │   │   └── README.md # 说明文档
│   │   │   └── generic.mk   # 通用 Makefile 模板
│   │   ├── spike_csr.cc    # CSR 测试主程序
│   │   ├── spike_csr_server.cc # ZMQ 服务器
│   │   ├── spike_csr_client.cc # ZMQ 客户端
│   │   ├── spike_insn.cc   # 自定义 Spike 主程序（指令测试）
│   │   ├── spike_mailbox.cc # Mailbox 测试主程序
│   │   └── Makefile        # 静态链接构建配置
│   └── xperimental/        # 自定义扩展实验
│       ├── xperimental_ext/    # 扩展实现 (.so 动态库)
│       ├── xperimental_sw/     # 测试软件
│       └── README.md           # 扩展使用指南
├── build_all.sh            # 统一构建脚本
├── run-fuse.sh             # Fuse 测试快捷脚本
├── IFLOW.md                # 项目文档
├── LICENSE                 # 许可证文件
├── README.md               # 项目总览
├── require.txt             # 项目需求说明
├── set-env.sh              # 环境变量设置脚本
└── tags                    # 代码标签文件
```

## 快速开始

### 1. 初始化仓库

```bash
# 克隆仓库（使用 dev 分支）
git clone -b dev git@github.com:KingFrige/riscv-isa-sim-lib-demo.git
cd riscv-isa-sim-lib-demo

# 初始化子模块
git submodule update --init --recursive
```

### 2. 设置环境变量

```bash
# 设置 Spike 路径和环境变量
source set-env.sh

# 注意：set-env.sh 中包含 module load 命令，仅在支持 module 的环境中使用
# 若不可用，请手动设置 RISCV_TOOLCHAIN 等环境变量
```

### 3. 构建项目

```bash
# 使用统一构建脚本（推荐）- 构建所有组件并运行测试
bash build_all.sh --all

# 构建并运行指令测试
bash build_all.sh --run-tests insn

# 构建特定固件类型
bash build_all.sh --firmware insn    # 仅构建指令固件
bash build_all.sh --firmware mailbox # 仅构建 Mailbox 固件
bash build_all.sh --firmware fuse    # 仅构建 Fuse 固件
bash build_all.sh --firmware csr     # 仅构建 CSR 固件
bash build_all.sh --firmware all     # 构建所有固件

# 构建特定包装器
bash build_all.sh --top insn     # 仅构建 spike_insn
bash build_all.sh --top mailbox  # 仅构建 spike_mailbox
bash build_all.sh --top csr      # 仅构建 spike_csr
bash build_all.sh --top zmq      # 仅构建 ZMQ 服务器/客户端
bash build_all.sh --top all      # 构建所有包装器

# 清理构建目录
bash build_all.sh --clean
```

### 4. 运行演示程序

#### C++ 内存模拟器演示

```bash
cd src/cpp
make           # 编译
./demo         # 运行
```

#### SystemC 集成演示

```bash
cd src/systemc

# 设置必要的环境变量（根据系统配置）
export SYSTEMC_INCLUDE=<SystemC头文件路径>
export SYSTEMC_LIBDIR=<SystemC库路径>
export RISCV_PATH=<RISC-V工具链路径>

make demo      # 编译
./demo         # 运行
```

#### 静态链接 Spike 演示（包含 AI 指令扩展）

```bash
# 使用统一构建脚本运行测试
bash build_all.sh --run-tests insn

# 或者手动构建
cd src/top
make spike_build  # 构建 Spike 库
make              # 构建静态链接的 top-main
make run-build    # 使用 build/ 目录中的文件运行测试（生成 spike.log）

# 直接运行测试程序
./build/insn/spike_insn --isa=rv64imafdcv_zvl512b_zicsr_xperia_xperiv \
    -l --log=build/insn/spike.log --log-commits \
    --instructions=80000 build/firmware/insn/firmware.elf
```

#### Mailbox 通信框架演示

```bash
# 构建并运行 Mailbox 测试
bash build_all.sh --run-tests mailbox

# 或者单独构建
bash build_all.sh --firmware mailbox  # 构建 Mailbox 固件
bash build_all.sh --top mailbox       # 构建 spike_mailbox 包装器

# 运行 Mailbox 测试
./build/mailbox/spike_mailbox -l --log=build/mailbox/spike.log \
    build/firmware/mailbox/firmware.elf
```

#### Fuse 固件演示（Mailbox 通信测试）

```bash
# 构建 Fuse 固件和测试
bash build_all.sh --firmware fuse    # 构建 Fuse 固件
bash build_all.sh --top mailbox      # 确保 spike_mailbox 已构建

# 运行 Fuse 测试
bash build_all.sh --run-tests        # 自动运行 Fuse 测试

# 或者使用快捷脚本
bash run-fuse.sh

# 单独运行 Fuse 调试
bash build_all.sh -d --debug-fuse
```

#### CSR 功能演示

```bash
# 构建 CSR 固件和测试程序
bash build_all.sh --firmware csr     # 构建 CSR 固件
bash build_all.sh --top csr          # 构建 spike_csr

# 运行 CSR 测试
bash build_all.sh --run-tests csr

# 或者直接运行
./build/csr/spike_csr build/csr/firmware/firmware.elf --test mail
```

#### ZMQ 通信框架演示

```bash
# 构建 ZMQ 服务器和客户端
bash build_all.sh --top zmq

# 运行 ZMQ 测试（会自动启动服务器并运行客户端测试）
bash build_all.sh --run-tests zmq

# 手动运行
# 终端 1：启动服务器
./build/zmq/spike_csr_server build/csr/firmware/firmware.elf

# 终端 2：运行客户端测试
./build/zmq/spike_csr_client all

# 发送退出命令
./build/zmq/spike_csr_client quit
```

### 5. 运行特定扩展测试

```bash
# 运行完整的 AI 指令测试
cd src/top
make clean
make spike_build
make
./build/insn/spike_insn --isa=rv64imafdcv_zvl512b_zicsr_xperia_xperiv \
    -l --log=build/insn/spike.log --log-commits \
    --instructions=80000 build/firmware/insn/firmware.elf

# 使用 RISC-V 工具链编译固件
cd src/top/firmware/insn
make clean
make all
```

## 详细构建说明

### 统一构建系统

项目使用 `build_all.sh` 作为统一的构建脚本，将所有构建输出集中到 `build/` 目录下。该脚本支持以下功能：

- 构建 Spike 模拟器
- 构建指令测试固件（insn）
- 构建 Mailbox 通信固件（mailbox）
- 构建 Fuse 固件（fuse）
- 构建 CSR 测试固件（csr）
- 构建顶层包装器（spike_insn, spike_mailbox, spike_csr）
- 构建 ZMQ 服务器/客户端（spike_csr_server, spike_csr_client）
- 运行测试
- 管理构建依赖

**主要命令：**

| 命令 | 描述 |
|------|------|
| `bash build_all.sh` | 显示帮助信息 |
| `bash build_all.sh --all` | 构建所有组件并运行所有测试 |
| `bash build_all.sh --run-tests` | 构建并运行所有测试（insn + mailbox + fuse + csr + zmq） |
| `bash build_all.sh --run-tests insn` | 仅构建并运行指令测试 |
| `bash build_all.sh --run-tests mailbox` | 仅构建并运行 Mailbox 测试 |
| `bash build_all.sh --run-tests fuse` | 仅构建并运行 Fuse 测试 |
| `bash build_all.sh --run-tests csr` | 仅构建并运行 CSR 测试 |
| `bash build_all.sh --run-tests zmq` | 仅构建并运行 ZMQ 测试 |
| `bash build_all.sh --clean` | 清理构建目录 |
| `bash build_all.sh --spike` | 仅构建 Spike |
| `bash build_all.sh --firmware insn\|mailbox\|fuse\|csr\|all` | 构建指定类型的固件 |
| `bash build_all.sh --top insn\|mailbox\|csr\|zmq\|all` | 构建指定类型的顶层包装器 |
| `bash build_all.sh -d --debug-fuse` | 单独运行 Fuse 调试（跳过构建） |
| `bash build_all.sh --help` | 显示帮助信息 |
| `bash build_all.sh --riscv PATH` | 设置 RISC-V 工具链路径 |
| `bash build_all.sh --spike-src PATH` | 设置 Spike 源码路径 |

### Spike 构建配置

Spike 作为子模块位于 `riscv-isa-sim/` 目录。构建过程会自动配置和安装到 `riscv-isa-sim/install` 目录。

关键环境变量（通过 `set-env.sh` 设置）：

- `RISCV_TOOLCHAIN`: RISC-V 工具链路径
- `SPIKE_INSTALL_DIR`: Spike 安装目录
- `SPIKE_BIN_DIR`: Spike 二进制路径
- `SPIKE_LIB_DIR`: Spike 库路径
- `SPIKE_INC_DIR`: Spike 包含路径
- `SPIKE_SOURCE_DIR`: Spike 源码路径
- `PATH`: 添加 Spike 二进制路径
- `LD_LIBRARY_PATH`: 添加 Spike 库路径
- `module load riscv-toolchain/master-v20251230`: 加载 RISC-V 工具链模块（环境依赖）

### C++ 内存模拟器构建

`src/cpp/Makefile` 提供了以下目标：

- `make demo`: 编译并链接完整演示程序
- `make compile_only`: 仅编译对象文件
- `make clean`: 清理生成文件
- `make spike_build`: 构建 Spike 子模块
- `make clean_spike`: 清理 Spike 构建

### 静态链接 Spike 构建（AI 指令扩展版本）

`src/top/Makefile` 提供了完整的静态链接构建流程：

**主要目标：**

| 目标 | 描述 |
|------|------|
| `make all` 或 `make` | 构建静态链接的 `spike_insn` 可执行文件 |
| `make build_spike` | 构建并安装 Spike 库 |
| `make build_insn_test` | 构建 `spike_insn` 可执行文件 |
| `make build_mailbox_test` | 构建 `spike_mailbox` 可执行文件 |
| `make build_csr_test` | 构建 `spike_csr` 可执行文件 |
| `make run_insn_test` | 运行指令测试程序 |
| `make run_mailbox_test` | 运行 Mailbox 测试程序 |
| `make clean` | 清理生成文件 |
| `make clean_spike` | 清理 Spike 构建 |
| `make distclean` | 清理所有生成文件 |

**构建特点：**

- 将自定义扩展（xperia, xperiv, exp, softmax, quant）直接编译到可执行文件中
- 支持 `--isa=rv64imafdcv_zvl512b_zicsr_xperia_xperiv` 指令集
- 无需动态加载扩展库
- 测试固件自动编译并链接
- 包含对不同 LMUL 配置（1/2/4/8）的全面测试支持

### SystemC 集成构建

`src/systemc/Makefile` 支持两种构建模式：

**模式1：使用系统安装的 Spike**

```bash
make demo
```

**模式2：使用自定义 Spike 构建**

```bash
make SPIKE_INCLUDE_DIR=/path/to/spike/headers demo
```

### 自定义扩展构建

实验性扩展位于 `src/xperimental/`：

```bash
cd src/xperimental/xperimental_ext
make           # 构建 libxperi.so

cd ../xperimental_sw
make           # 构建测试程序 main.elf
```

### Fuse 固件构建

Fuse 固件使用 Mailbox 通信框架进行测试：

```bash
# 构建 Fuse 固件
bash build_all.sh --firmware fuse

# 运行 Fuse 测试
bash build_all.sh --run-tests  # 会自动运行 Fuse 测试

# 或者使用快捷脚本
bash run-fuse.sh

# 单独调试模式
bash build_all.sh -d --debug-fuse
```

Fuse 固件输出文件：
- `build/firmware/fuse/firmware.elf`: ELF 可执行文件
- `build/firmware/fuse/firmware.bin`: 原始二进制
- `build/firmware/fuse/firmware.dump`: 反汇编

### CSR 固件构建

CSR 固件用于测试自定义 CSR 寄存器功能：

```bash
# 构建 CSR 固件
bash build_all.sh --firmware csr

# 构建 spike_csr 测试程序
bash build_all.sh --top csr

# 运行 CSR 测试
bash build_all.sh --run-tests csr
```

CSR 固件输出文件：
- `build/csr/firmware/firmware.elf`: ELF 可执行文件

### ZMQ 通信框架构建

ZMQ 框架提供服务器/客户端通信：

```bash
# 构建 ZMQ 服务器和客户端
bash build_all.sh --top zmq

# 运行 ZMQ 测试
bash build_all.sh --run-tests zmq
```

ZMQ 输出文件：
- `build/zmq/spike_csr_server`: ZMQ 服务器可执行文件
- `build/zmq/spike_csr_client`: ZMQ 客户端可执行文件

### Mailbox 通信框架构建

Mailbox 框架允许主机程序与运行在 Spike 模拟器中的固件进行通信：

```bash
# 构建 Mailbox 固件
bash build_all.sh --firmware mailbox

# 构建 spike_mailbox 可执行文件
bash build_all.sh --top mailbox

# 或者一起构建所有组件
bash build_all.sh --all
```

### 输出文件结构

```
build/
├── csr/
│   ├── firmware/
│   │   └── firmware.elf      # CSR 测试固件
│   └── spike_csr             # CSR 测试可执行文件
├── custom/                   # 自定义扩展对象文件
│   └── riscv/
├── extensions/               # Spike 扩展对象文件
├── firmware/
│   ├── csr/
│   │   └── firmware.elf      # CSR 固件
│   ├── insn/
│   │   └── firmware.elf      # 指令测试固件
│   ├── mailbox/
│   │   └── firmware.elf      # Mailbox 通信固件
│   └── fuse/
│       ├── firmware.elf      # Fuse 固件
│       ├── firmware.bin
│       └── firmware.dump
├── insn/
│   ├── spike_insn            # 指令测试可执行文件
│   └── spike.log             # 执行日志
├── mailbox/
│   ├── spike_mailbox         # Mailbox 测试可执行文件
│   └── spike.log             # 执行日志
├── zmq/
│   ├── spike_csr_server      # ZMQ 服务器
│   └── spike_csr_client      # ZMQ 客户端
└── log/
    ├── info_insn.log         # insn 测试输出
    ├── info_mailbox.log      # mailbox 测试输出
    ├── info_fuse.log         # fuse 测试输出
    ├── info_fuse-debug.log   # fuse 调试输出
    ├── info_csr.log          # csr 测试输出
    ├── info_zmq.log          # zmq 测试输出
    ├── zmq_server.log        # ZMQ 服务器日志
    └── zmq_client.log        # ZMQ 客户端日志
```

## 自定义扩展说明

### 现有扩展

1. **XPERIA** - 标量扩展，包含 1 条指令 `peri.a.add`
   - 操作码：0x0b (CUSTOM0)
   - 功能：标量加法，`rd = rs1 + rs2`

2. **XPERIV** - 向量扩展，包含 1 条指令 `peri.v.add`
   - 操作码：0x2b (CUSTOM1)
   - 功能：向量加法，`vd[i] = vs1[i] + vs2[i]`

3. **XPERIV_MUL** - 向量乘法扩展，包含 1 条指令 `peri.v.mul`
   - 操作码：0x5b (CUSTOM3)
   - 功能：向量乘法，`vd[i] = vs1[i] * vs2[i]`

4. **EXP** - 向量指数扩展，包含 1 条指令 `exp`
   - 操作码：0x0b (CUSTOM0), func7=0x03, func3=0x6
   - 功能：向量 BF16 指数运算，`vd[i] = e^(vs1[i])`

5. **SOFTMAX** - 向量 Softmax 扩展，包含 1 条指令 `softmax`
   - 操作码：0x0b (CUSTOM0), func7=0x03, func3=0x2
   - 功能：向量 Softmax 运算，对 vs1 中的值执行 Softmax

6. **QUANT** - 向量量化扩展，包含 1 条指令 `quant`
   - 操作码：0x0b (CUSTOM0), func7=0x05, func3=0x6
   - 功能：BF16 到 MXFP8 量化，`vd[i] = quantize(vs1[i])`

### Mailbox 设备扩展

Mailbox 设备提供主机与固件之间的通信机制（基于内存映射 I/O）：

**寄存器映射（MMIO）：**

| 地址偏移 | 寄存器名称 | 位宽 | 功能描述 |
|---------|-----------|------|---------|
| 0x0000 | MAILBOX_STATUS | 32位 | 状态寄存器 (READY, BUSY, ERROR) |
| 0x0004 | MAILBOX_COMMAND | 32位 | 命令寄存器 |
| 0x000C | MAILBOX_RESPONSE | 32位 | 响应寄存器 |
| 0x0020 | MAILBOX_DATA0 | 64位 | 数据寄存器 0 |
| 0x0028 | MAILBOX_DATA1 | 64位 | 数据寄存器 1 |
| 0x0030 | MAILBOX_DATA2 | 64位 | 数据寄存器 2 |
| 0x0038 | MAILBOX_DATA3 | 64位 | 数据寄存器 3 |

**支持的命令：**

| 命令值 | 名称 | 功能描述 |
|--------|------|---------|
| 0x00000001 | MAILBOX_CMD_HELLO | 测试命令 |
| 0x00000002 | MAILBOX_CMD_HI | 简单响应命令 |
| 0x00000010 | MAILBOX_CMD_VECTOR_LOAD | 向量加载命令 |
| 0x00000011 | MAILBOX_CMD_VECTOR_STORE | 向量存储命令 |
| 0x00000012 | MAILBOX_CMD_VECTOR_COMPUTE | 向量计算命令 |
| 0x00000020 | MAILBOX_CMD_SOFTMAX | Softmax 计算命令 |
| 0x00000021 | MAILBOX_CMD_EXP | EXP 计算命令 |
| 0x00000022 | MAILBOX_CMD_QUANT | QUANT 量化命令 |

**状态位：**

| 位 | 名称 | 功能描述 |
|----|------|---------|
| [0] | READY | 设备就绪 |
| [1] | BUSY | 设备繁忙 |
| [2] | ERROR | 发生错误 |

### RVV 自定义 CSR 设计

项目已实现自定义 CSR 寄存器，用于支持 Mailbox、Barrier Event (BO) 和 Synchronization Event (SE) 等高级同步机制。

**CSR 寄存器映射：**

| CSR 地址 | 通道 | 寄存器名称 | 位宽 | 功能描述 |
|---------|------|-----------|------|---------|
| 0xBC0 | Mail | MAIL_DATA0 | 64bit | Mail 数据寄存器 0 |
| 0xBC1 | Mail | MAIL_DATA1 | 64bit | Mail 数据寄存器 1 |
| 0xBC2 | Mail | MAIL_DATA2 | 64bit | Mail 数据寄存器 2 |
| 0xBC3 | Mail | MAIL_DATA3 | 64bit | Mail 数据寄存器 3 |
| 0xBC4 | Mail | MAIL_VALID | 1bit | Mail 有效状态标志 |
| 0xBC5 | Bo done | BO_DONE | 11bit | 完成信号（5bit wg_index + 6bit bar_index） |
| 0xBC6 | Se up | SE_UP | 11bit | 更新信号（5bit wg_index + 6bit bar_index） |
| 0xBC7 | Se query | SE_QUERY_LOCK | 11bit | 查询锁信号（5bit wg_index + 6bit bar_index） |
| 0xBC8 | Se query | SE_QUERY_COUNT | - | 查询计数寄存器 |

**CSR 地址定义：**

CSR 地址定义统一在 `src/top/common/csr_addr.h` 中：

```c
#define CSR_MAIL_DATA0      0xBC0
#define CSR_MAIL_DATA1      0xBC1
#define CSR_MAIL_DATA2      0xBC2
#define CSR_MAIL_DATA3      0xBC3
#define CSR_MAIL_VALID      0xBC4
#define CSR_BO_DONE         0xBC5
#define CSR_SE_UP           0xBC6
#define CSR_SE_QUERY_LOCK   0xBC7
#define CSR_SE_QUERY_COUNT  0xBC8
```

**Mail 通道详细说明：**

Mail 通道用于通过 scheduler ring buffer 发送邮件，最大支持 256bit 数据。

**数据传输流程：**
1. 主机写入 4 个 64bit 数据到 CSR 0xBC0-0xBC3
2. 主机写入 CSR 0xBC4 置位 mail valid，通知 RVV 获取 mail
3. RVV 通过轮询 CSR 0xBC4 查询 mail 状态
4. 当 valid 为高时，RVV 依次读取 CSR 0xBC0-0xBC3 获取 256bit 数据
5. 读取完成后，RVV 写入 CSR 0xBC4 清除 valid

**Se query 通道详细说明：**

Se query 通道用于查询操作的同步机制。

**操作流程：**
1. RVV 写入 CSR 0xBC7，data[5:0]=bar_index，data[10:6]=wg_index，发送 lock pulse
2. 同时将 query_cnt 清零
3. 每次收到 notify pulse，query_cnt 加 1
4. RVV 查询 CSR 0xBC8，如果大于 0 表示上游有信号发送
5. RVV 写入 CSR 0xBC8 对 query_cnt 做减法操作（减少值不能大于查询得到的值）

**Se up 通道详细说明：**

Se up 通道用于更新操作。

**操作方式：**
- RVV 写入 CSR 0xBC6，data[5:0]=bar_index，data[10:6]=wg_index，发送 se_up_done

**Bo done 通道详细说明：**

Bo done 通道用于完成信号通知。

**操作方式：**
- RVV 写入 CSR 0xBC5，data[5:0]=bar_index，data[10:6]=wg_index，发送 bo_done

详细设计请参考 `docs/rvv-custom-csr.md`。

### ZMQ 通信框架

ZMQ 框架提供基于 ZeroMQ 的服务器/客户端通信，支持远程 CSR 访问。

**组件：**

- **spike_csr_server**: ZMQ 服务器，运行 Spike 模拟器并监听客户端请求
- **spike_csr_client**: ZMQ 客户端，向服务器发送 CSR 读写请求

**支持的命令：**

- `all`: 运行所有测试
- `quit`: 退出服务器

**使用方式：**

```bash
# 启动服务器
./build/zmq/spike_csr_server build/csr/firmware/firmware.elf

# 运行客户端测试
./build/zmq/spike_csr_client all

# 退出服务器
./build/zmq/spike_csr_client quit
```

### 扩展开发

添加自定义扩展时：

1. 在 `src/top/extensions/` 或 `src/xperimental/xperimental_ext/` 中创建新文件
2. 参考 `xperia.cc` 和 `xperiv.cc` 实现扩展（需实现指令解码、执行和反汇编）
3. 通过静态链接方式加载
4. 提供对应的测试软件（参考 `src/top/firmware/insn/main.c`）
5. 将算法实现放在 `src/top/custom/riscv/` 目录中，避免外部依赖
6. 如果涉及 CSR 寄存器，在 `src/top/common/csr_addr.h` 中定义地址

### 自定义扩展目录结构

项目采用双目录结构来维护算法实现：

- `src/top/custom/riscv/`: 独立的算法实现和单元测试
- `src/top/extensions/`: Spike 扩展实现（静态链接方式）
- `src/top/common/`: 通用头文件（如 CSR 地址定义）

这种结构允许：

- 独立的算法开发和测试（在 custom 目录中）
- 与 Spike 静态链接时的自包含实现
- 避免构建时的外部依赖问题
- 统一的 CSR 地址管理

## 开发约定

### 代码组织

1. **模块分离**: 每个用例有独立目录，包含完整的构建和测试设施
2. **头文件管理**: 公共头文件放置在对应目录的根级别，通用定义放在 `common/` 目录
3. **测试软件**: 每个演示都有对应的测试软件目录 (`sw/` 或 `firmware/`)
4. **AI 扩展**: 算法实现在 `src/top/custom/riscv/` 中，Spike 扩展在 `src/top/extensions/` 中
5. **Mailbox 框架**: 固件在 `firmware/mailbox/` 和 `firmware/fuse/`，设备实现在 `extensions/mailbox.cc`
6. **CSR 功能**: 固件在 `firmware/csr/`，CSR 实现在 `extensions/custom_csr.cc`，地址定义在 `common/csr_addr.h`
7. **ZMQ 通信**: 服务器实现在 `spike_csr_server.cc`，客户端在 `spike_csr_client.cc`
8. **固件类型**: `insn/` 用于指令测试，`mailbox/` 用于通信测试，`fuse/` 用于 Mailbox 通信测试，`csr/` 用于 CSR 测试

### 构建系统

- 使用 `build_all.sh` 统一管理构建过程
- 输出集中到 `build/` 目录
- 支持环境变量覆盖配置
- 提供 `clean` 目标确保可重复构建
- Spike 构建通过子模块和自动化脚本管理
- 固件构建使用 `generic.mk` 通用模板
- CSR 地址定义统一在 `common/csr_addr.h` 中

### 扩展开发

添加自定义扩展时：

1. 在 `src/top/extensions/` 中创建新文件
2. 参考 `xperia.cc` 和 `xperiv.cc` 实现扩展
3. 通过静态链接方式加载
4. 提供对应的测试软件
5. 将算法实现放在 `src/top/custom/` 目录中，避免外部依赖
6. 如果涉及 CSR，在 `common/csr_addr.h` 中定义地址
7. 在 `extensions/custom_csr.cc` 中实现 CSR 类

## 测试和验证

### 测试软件

每个演示都包含测试软件：

- `src/cpp/sw/`: C++ 演示的测试程序
- `src/systemc/sw/`: SystemC 演示的测试程序
- `src/top/firmware/insn/`: 静态链接演示的测试固件（包含 AI 指令测试）
- `src/top/firmware/fuse/`: Fuse 固件（Mailbox 通信测试自定义指令）
- `src/top/firmware/mailbox/`: Mailbox 通信测试固件
- `src/top/firmware/csr/`: CSR 测试固件
- `src/xperimental/xperimental_sw/`: 自定义扩展测试程序

### 运行验证

1. **基本功能验证**: 运行演示程序检查是否正确执行
2. **扩展验证**: 使用 `spike_insn` 运行包含自定义扩展的程序
3. **AI 指令验证**: 使用 `spike_insn` 运行包含 exp/softmax/quant 指令的测试程序
4. **日志分析**: 通过 `-l` 和 `--log` 参数生成执行日志
5. **Mailbox 验证**: 使用 `spike_mailbox` 测试主机与固件的通信
6. **Fuse 验证**: 使用 `spike_mailbox` 运行 fuse 固件测试 Mailbox 通信
7. **CSR 验证**: 使用 `spike_csr` 测试自定义 CSR 寄存器功能
8. **ZMQ 验证**: 使用 ZMQ 服务器/客户端测试远程通信

### 调试支持

- **SystemC 调试**: 使用 `--debug` 或 `-d` 参数启用调试输出
- **远程调试**: 支持远程 bitbang 调试 (`--rbb-port`)
- **JTAG 接口**: 集成 Spike 的 JTAG DTM 模块
- **自定义扩展调试**: 扩展实现中包含 fprintf 输出用于调试
- **Fuse 调试**: 使用 `-d --debug-fuse` 单独运行 Fuse 调试模式
- **CSR 调试**: 使用 `spike_csr` 的 `--test mail` 参数测试 Mail 通道
- **ZMQ 调试**: 查看 `log/zmq_server.log` 和 `log/zmq_client.log`

## 测试框架和预期结果校验

项目实现了完整的测试框架，用于验证自定义扩展的功能正确性，特别针对 AI 推理中的数学运算进行了优化。

### 测试框架特性

1. **预期结果校验**: 每个自定义指令都有对应的预期结果验证
2. **错误报告机制**: 通过 `tohost` 机制报告测试失败和错误代码
3. **全面覆盖**: 支持标量、向量及 AI 指令的测试
4. **自动化验证**: 测试程序自动验证指令执行结果
5. **AI 精度测试**: 针对 BF16、MXFP8 等 AI 精度进行专门测试
6. **LMUL 测试**: 针对不同向量长度乘数（1/2/4/8）进行测试
7. **Mailbox 通信测试**: 使用 Fuse 固件测试 Mailbox 通信框架
8. **CSR 功能测试**: 使用 CSR 固件测试自定义 CSR 寄存器
9. **ZMQ 通信测试**: 使用 ZMQ 服务器/客户端测试远程通信

### 测试错误代码

测试框架定义了以下错误代码：

| 错误代码 | 描述 |
|----------|------|
| `ERR_XPERIA_ADD (0x10)` | XPERIA 标量加法扩展测试失败 |
| `ERR_XPERIV_ADD (0x30)` | XPERIV 向量加法扩展测试失败 |
| `ERR_XPERIV_MUL (0x40)` | XPERIVMUL 向量乘法扩展测试失败 |
| `ERR_EXP (0x50)` | EXP 向量指数运算扩展测试失败 |
| `ERR_SOFTMAX (0x60)` | SOFTMAX 向量 Softmax 运算扩展测试失败 |
| `ERR_QUANT (0x70)` | QUANT 向量量化扩展测试失败 |

### Mailbox 错误代码

| 错误代码 | 描述 |
|----------|------|
| `MAILBOX_ERR_SUCCESS (0x00000000)` | 成功 |
| `MAILBOX_ERR_INVALID_CMD (0x00000001)` | 无效命令 |
| `MAILBOX_ERR_INVALID_PARAM (0x00000002)` | 无效参数 |
| `MAILBOX_ERR_MEM_ACCESS (0x00000003)` | 内存访问错误 |
| `MAILBOX_ERR_VECTOR_CONFIG (0x00000004)` | 向量配置错误 |
| `MAILBOX_ERR_NOT_IMPLEMENTED (0x00000005)` | 未实现 |

### 当前测试状态

✅ **已通过验证的功能**:
- XPERIA 标量加法扩展 (`peri.a.add`)
- XPERIV 向量加法扩展 (`peri.v.add`)
- XPERIVMUL 向量乘法扩展 (`peri.v.mul`)
- EXP 向量指数运算扩展 (`exp`)
- SOFTMAX 向量 Softmax 运算扩展 (`softmax`)
- QUANT 向量量化扩展 (`quant`)
- Mailbox 通信框架（支持新 DATA0-3 接口）
- Fuse 固件 Mailbox 通信测试（支持 exp/quant/softmax）
- 自定义 CSR 寄存器（Mail、BO、SE 通道）
- CSR 固件测试
- ZMQ 通信框架

🔧 **已修复的问题**:
- EXP/softmax/quant 指令的 commit log 显示问题 - 已通过改进扩展实现修复
- 提升了 LMUL (1/2/4/8) 不同配置下的测试覆盖
- Mailbox 通信稳定性问题
- 构建系统整合（统一使用 build_all.sh）
- Mailbox 寄存器映射更新（使用 DATA0-3 替代 DATA_ADDR/DATA_SIZE/VECTOR_CONFIG）
- CSR 地址从 0xF20-0xF28 更新为 0xBC0-0xBC8

📋 **待实现功能** (TODO):
- [ ] 扩展 CSR 测试覆盖（BO、SE 通道）
- [ ] 集成更多 Barrier Event (BO) 测试用例
- [ ] 集成更多 Synchronization Event (SE) 测试用例
- [ ] ZMQ 通信框架功能扩展

### 运行测试

```bash
# 运行完整测试（包含 AI 指令、CSR、ZMQ）
bash build_all.sh --all

# 运行指令测试
bash build_all.sh --run-tests insn

# 运行 Mailbox 测试
bash build_all.sh --run-tests mailbox

# 运行所有测试（包括 Fuse）
bash build_all.sh --run-tests

# 运行 Fuse 测试
bash build_all.sh --run-tests fuse

# 运行 CSR 测试
bash build_all.sh --run-tests csr

# 运行 ZMQ 测试
bash build_all.sh --run-tests zmq

# 查看测试日志
cat build/log/info_insn.log
cat build/log/info_csr.log
cat build/log/info_zmq.log

# 使用快捷脚本运行 Fuse 测试
bash run-fuse.sh

# 单独调试 Fuse（跳过构建）
bash build_all.sh -d --debug-fuse
```

### 测试程序结构

测试程序 (`src/top/firmware/insn/main.c`) 包含：

1. **测试设置**: 初始化测试环境，设置向量扩展
2. **指令执行**: 执行所有自定义指令（包括 AI 指令）
3. **LMUL 配置**: 针对不同长度乘数 (1/2/4/8) 的测试
4. **结果验证**: 验证每个指令的执行结果
5. **错误处理**: 报告测试失败并传递错误代码
6. **成功报告**: 通过 `tohost` 机制报告测试通过

Fuse 测试程序 (`src/top/firmware/fuse/main.c`) 包含：

1. **Mailbox 通信**: 使用 Mailbox 框架进行命令处理
2. **命令分发**: 支持 HELLO、HI、VECTOR_LOAD、VECTOR_STORE、VECTOR_COMPUTE、SOFTMAX、EXP、QUANT 等命令
3. **指令测试**: 先运行 test_insn() 执行指令测试
4. **主循环**: 轮询 Mailbox 状态并处理命令

CSR 测试程序 (`src/top/firmware/csr/main.c`) 包含：

1. **CSR 初始化**: 初始化 CSR 寄存器
2. **Mail 通道测试**: 测试 Mail 数据传输
3. **主循环**: 轮询 CSR 状态并处理请求

### LMUL 测试覆盖

测试程序包含对不同 LMUL (Vector Length Multiplier) 配置的全面测试：

- **Test 7-10**: EXP 指令 (LMUL=1, 2, 4, 8)
- **Test 11-14**: SOFTMAX 指令 (LMUL=1, 2, 4, 8)
- **Test 15-18**: QUANT 指令 (LMUL=1, 2, 4, 8)

每个测试都使用不同的向量长度验证指令的正确性，确保在所有支持的向量配置下正确运行。

## 故障排除

### 常见问题

1. **Spike 构建失败**
   - 检查依赖项：确保安装了必要的开发工具（gcc, g++, make, autoconf, automake）
   - 验证子模块是否正确初始化：`git submodule status`
   - 检查 RISC-V 工具链是否可用：`which riscv64-unknown-elf-gcc`
   - 尝试手动配置：`cd riscv-isa-sim && mkdir build && cd build && ../configure --prefix=$(pwd)/../install`

2. **库加载错误**
   - 确保 `LD_LIBRARY_PATH` 正确设置：`echo $LD_LIBRARY_PATH`
   - 检查动态库路径和权限：`ls -la riscv-isa-sim/install/lib/`
   - 验证 Spike 是否正确安装：`ls riscv-isa-sim/install/bin/spike`

3. **SystemC 链接错误**
   - 验证 `SYSTEMC_INCLUDE` 和 `SYSTEMC_LIBDIR` 环境变量
   - 检查 SystemC 库版本兼容性：SystemC 2.3+ 推荐
   - 确认 SystemC 库是否编译为共享库

4. **静态链接构建失败**
   - 确保 Spike 库已正确构建和安装：`bash build_all.sh --spike`
   - 检查扩展文件路径和编译选项：`ls src/top/extensions/`
   - 验证依赖库路径设置：`echo $LIBRARY_PATH`
   - 检查编译器版本：`g++ --version`（需要支持 C++17）

5. **AI 指令精度问题**
   - 检查 BFloat16 精度：验证输入输出是否符合预期精度
   - 查看日志输出：使用 `-l` 和自定义调试输出来调试精度问题
   - 参考 `src/top/custom/riscv/` 中的算法实现验证正确性

6. **Mailbox 通信失败**
   - 检查固件是否正确构建：`ls build/firmware/mailbox/firmware.elf`
   - 验证 Mailbox 包装器是否正确构建：`ls build/mailbox/spike_mailbox`
   - 查看通信日志：`cat build/mailbox/spike.log`
   - 检查 Mailbox 寄存器映射是否正确

7. **Fuse 测试失败**
   - 检查 Fuse 固件是否构建：`ls build/firmware/fuse/firmware.elf`
   - 验证 spike_mailbox 是否包含扩展：`ls build/mailbox/spike_mailbox`
   - 查看 Fuse 日志：`cat build/log/info_fuse.log`
   - 使用调试模式运行：`bash build_all.sh -d --debug-fuse`

8. **CSR 测试失败**
   - 检查 CSR 固件是否构建：`ls build/csr/firmware/firmware.elf`
   - 验证 spike_csr 是否正确构建：`ls build/csr/spike_csr`
   - 查看 CSR 日志：`cat build/log/info_csr.log`
   - 检查 CSR 地址定义：`cat src/top/common/csr_addr.h`

9. **ZMQ 测试失败**
   - 检查 ZMQ 是否安装：`pkg-config --exists libzmq && echo "ZMQ installed"`
   - 检查 ZMQ 服务器是否构建：`ls build/zmq/spike_csr_server`
   - 检查 ZMQ 客户端是否构建：`ls build/zmq/spike_csr_client`
   - 查看 ZMQ 日志：`cat build/log/zmq_server.log` 和 `cat build/log/zmq_client.log`

10. **module load 命令不可用**
    - 手动设置 RISC-V 工具链路径：`export RISCV_TOOLCHAIN=/path/to/riscv/toolchain`
    - 更新 `set-env.sh` 文件，注释掉 `module load` 行

### 环境检查

运行环境检查脚本（如有）或手动验证：

```bash
# 检查 Spike 是否可用
which spike
spike --version

# 检查库路径
echo $LD_LIBRARY_PATH

# 检查 RISC-V 工具链
which riscv64-unknown-elf-gcc

# 检查 Spike 安装
ls riscv-isa-sim/install/bin/spike 2>/dev/null || echo "Spike not installed"

# 检查构建输出
ls -la build/

# 检查各固件是否构建
ls build/firmware/insn/    # 指令测试固件
ls build/firmware/mailbox/ # Mailbox 固件
ls build/firmware/fuse/    # Fuse 固件
ls build/csr/firmware/     # CSR 固件

# 检查各可执行文件是否构建
ls build/insn/spike_insn
ls build/mailbox/spike_mailbox
ls build/csr/spike_csr
ls build/zmq/spike_csr_server
ls build/zmq/spike_csr_client

# 检查日志
ls -la build/log/

# 检查 ZMQ
pkg-config --exists libzmq && echo "ZMQ installed" || echo "ZMQ not installed"
```

## 项目状态

根据 `require.txt` 的需求，已完成以下开发重点：

✅ **已完成的功能**:
- 理解现有代码架构
- 实现新的顶层设计，将外部库与 Spike 静态链接
- 添加指令扩展（exp、softmax、quant）
- 测试和验证新实现
- 修复新增加指令的 commit_log 问题
- 整合构建系统到 `build_all.sh`
- 输出文件到 `build/` 目录
- 修复一元操作的打印问题
- 增加更多测试用例（LMUL 测试）
- 添加 Mailbox 通信框架
- 固件集成 printf 功能
- 统一构建系统（build_all.sh）
- 新增 Fuse 固件测试
- 改进固件构建系统（使用 generic.mk）
- 添加 --firmware 和 --top 选项
- 更新 Mailbox 寄存器映射（使用 DATA0-3）
- 添加 exp/quant 命令支持
- 添加自定义 CSR 设计文档
- 实现自定义 CSR 寄存器（Mail、BO、SE 通道）
- 实现 CSR 固件和测试程序
- 添加 common/csr_addr.h 统一 CSR 地址定义
- 实现 ZMQ 通信框架
- 添加 ZMQ 服务器和客户端
- 更新 CSR 地址映射（0xBC0-0xBC8）

📋 **当前开发中**:
- 扩展 CSR 测试覆盖（BO、SE 通道）
- 集成更多 Barrier Event (BO) 测试用例
- 集成更多 Synchronization Event (SE) 测试用例
- ZMQ 通信框架功能扩展

**注意**: 避免直接修改 `riscv-isa-sim/` 子模块中的代码，应通过外部层级和编译系统扩展功能。`src/top/` 目录展示了如何在不修改 Spike 源代码的情况下实现静态链接集成。

## 版本更新说明

### 新增功能

1. **自定义 CSR 功能**: 实现自定义 CSR 寄存器支持（Mail、BO、SE 通道）
2. **CSR 测试固件**: 新增 `firmware/csr/` 目录，包含 CSR 测试固件
3. **CSR 测试程序**: 新增 `spike_csr.cc`，专门用于测试 CSR 功能
4. **ZMQ 通信框架**: 新增基于 ZeroMQ 的服务器/客户端通信
5. **ZMQ 服务器**: 新增 `spike_csr_server.cc`，ZMQ 服务器实现
6. **ZMQ 客户端**: 新增 `spike_csr_client.cc`，ZMQ 客户端实现
7. **CSR 地址定义**: 新增 `common/csr_addr.h`，统一 CSR 地址管理
8. **Custom CSR 扩展**: 新增 `custom_csr.cc/h`，自定义 CSR 实现
9. **更新 CSR 地址映射**: 从 0xF20-0xF28 更新为 0xBC0-0xBC8
10. **Fuse 固件测试**: 新增 `firmware/fuse/` 目录，包含 Mailbox 通信测试固件
11. **Mailbox 通信框架**: 提供主机程序与模拟器固件之间的通信机制
12. **统一构建系统** (`build_all.sh`): 集中化构建流程，输出到 `build/` 目录
13. **静态链接 Spike 集成** (`src/top/`): 实现了将自定义扩展与 Spike 静态链接的功能
14. **AI 指令扩展**: 添加了 exp、softmax、quant 指令用于 AI 推理加速
15. **改进的构建系统**: 支持静态链接方式，避免动态库依赖问题
16. **完整测试框架**: 包含预期结果校验和错误报告机制
17. **LMUL 测试覆盖**: 增加了对不同向量长度乘数 (1/2/4/8) 的测试
18. **指令编码参考**: 根据 docs/insn-decode.jpg, docs/insn.jpg 实现扩展
19. **算法集成**: 将算法实现放在 `src/top/custom/riscv/` 目录中
20. **扩展测试**: 提取测试方法到测试函数中
21. **Firmware 复杂测试**: 增加更多测试到 firmware 中
22. **Custom 目录**: 创建 `src/top/custom/` 目录，包含完整的 RISC-V 扩展算法实现
23. **固件 printf 支持**: 成功集成 printf 功能到固件中
24. **run-fuse.sh 脚本**: 提供 Fuse 测试的快捷运行方式
25. **通用固件构建**: 添加 generic.mk 模板统一固件构建配置
26. **Mailbox 寄存器更新**: 更新为使用 DATA0-3 寄存器接口
27. **自定义 CSR 设计**: 添加 rvv-custom-csr.md 文档

### 修复改进

1. **Commit Log 问题**: 修复了 EXP/softmax/quant 指令在日志中不显示的问题
2. **指令打印格式**: 修复了一元操作指令（如 quant, exp）的打印格式
3. **算法集成**: 将算法实现直接集成到扩展中
4. **测试验证**: 增强了测试用例，包括不同 LMUL 配置下的验证
5. **目录结构**: 改进了目录结构，将算法实现与扩展实现分离
6. **Mailbox 稳定性**: 修复了 Mailbox 通信中的问题
7. **构建系统**: 统一使用 build_all.sh 管理所有构建
8. **固件类型**: 分离 insn/mailbox/fuse/csr 固件类型
9. **Mailbox 接口**: 更新为使用 DATA0-3 新接口
10. **CSR 地址**: 更新 CSR 地址映射为 0xBC0-0xBC8

### 使用建议

- 对于生产环境，推荐使用 `build_all.sh` 统一构建系统
- 对于开发和测试，可以使用 `bash build_all.sh --run-tests` 快速验证
- 参考 `src/top/Makefile` 了解如何集成新的扩展
- 扩展开发时，确保指令编码不与现有指令冲突（使用 CUSTOM0-CUSTOM3 操作码空间）
- AI 指令参考 `src/top/custom/riscv/` 中的算法实现
- Mailbox 通信测试使用 `firmware/fuse/` 目录
- CSR 测试使用 `firmware/csr/` 目录
- ZMQ 通信测试使用 `build/zmq/` 目录
- 自定义 CSR 开发参考 `docs/rvv-custom-csr.md`
- CSR 地址定义参考 `src/top/common/csr_addr.h`

### 已知限制

- 当前测试固件使用固定内存地址，可能不适用于所有内存布局
- SystemC 集成需要额外的 SystemC 库安装
- Mailbox 通信目前仅支持特定的命令集
- CSR 功能已实现，但测试覆盖仍需扩展
- ZMQ 通信框架功能仍在完善中

## 贡献指南

1. 遵循现有代码风格和目录结构
2. 添加新扩展时，提供对应的测试程序
3. 更新文档（包括本文件）以反映变更
4. 确保构建系统向后兼容
5. 提交前运行现有测试：`bash build_all.sh --run-tests`
6. AI 扩展需同时更新 `src/top/custom/` 和 `src/top/extensions/` 中的实现
7. 确保新的扩展指令正确记录到 commit log 中
8. 添加 Mailbox 功能时，更新 `spike_mailbox.README.md` 文档
9. 添加新固件类型时，更新 `build_all.sh` 中的构建逻辑
10. 更新 TODO 列表以跟踪待完成工作
11. 添加自定义 CSR 功能时，更新 `docs/rvv-custom-csr.md` 文档
12. CSR 地址定义必须添加到 `src/top/common/csr_addr.h`
13. 添加 ZMQ 功能时，更新相关文档和测试用例

## 许可证

本项目基于 Spike 的许可证（BSD 3-Clause）和自定义扩展的 MIT 许可证。详见各目录中的 LICENSE 文件。