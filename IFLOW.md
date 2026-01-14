# RISC-V ISA 模拟器库集成演示项目

## 项目概述

本项目演示如何将 [RISC-V ISA 模拟器 (Spike)](https://github.com/riscv-software-src/riscv-isa-sim) 作为库使用，并与外部模拟器环境集成。项目已进一步扩展，集成了针对 Llama.cpp 的 RISC-V 向量扩展，专门用于 AI 推理加速的自定义指令集扩展。

目前包含四个主要用例：
1. **C++ 内存模拟器集成** - 将 Spike 与自定义 C++ 内存模拟器连接
2. **SystemC 包装器** - 将 Spike 嵌入 SystemC 环境，创建完整的系统级仿真平台
3. **静态链接 Spike** - 将自定义扩展与 Spike 静态链接，创建独立的可执行文件（已实现）
4. **Llama.cpp RISC-V 向量扩展** - 为 AI 推理实现自定义的 exp、softmax、quant 指令

此外，项目还包含一个实验性扩展示例 (`src/xperimental`)，展示了如何为 Spike 添加自定义指令扩展。

## 技术栈

- **核心模拟器**: RISC-V ISA Simulator (Spike)
- **编程语言**: C++ (主程序), C (测试软件), SystemC (系统级建模), RISC-V 汇编
- **构建系统**: GNU Make, Autotools (Spike 子模块), Consolidated build script (`build_all.sh`)
- **仿真环境**: SystemC 2.3+ (可选)
- **工具链**: RISC-V GNU 工具链 (需支持 rv64imafdcv 架构)
- **AI 推理**: 集成 BFloat16、MxFP8 等 AI 精度处理

## 目录结构

```
.
├── build/                  # 统一构建输出目录
│   ├── firmware/           # 固件构建输出
│   ├── spike/              # Spike 构建输出
│   ├── spike-install/      # Spike 安装目录
│   └── top/                # top wrapper 构建输出
├── C_src/                  # Llama.cpp RISC-V 向量扩展实现
│   ├── config.h            # 配置文件
│   ├── Makefile            # 构建配置
│   ├── util.c/h            # 工具函数
│   ├── riscv/              # RISC-V 向量扩展实现
│   │   ├── BF16.cpp/hpp    # BFloat16 处理单元
│   │   ├── custom_expp.cpp/hpp # BF16 e^x 近似计算单元
│   │   ├── SoftmaxCore.cpp/hpp # Softmax 核心计算单元
│   │   ├── MxFp8ActQuant.cpp/hpp # MXFP8 激活量化单元
│   │   ├── main_expp.cpp   # exp 单元测试主程序
│   │   ├── main_quant.cpp  # quant 单元测试主程序
│   │   ├── main_softmax.cpp # softmax 单元测试主程序
│   │   └── single_expp.cpp # 单值 exp 调试程序
│   ├── gemm/               # GEMM 运算实现
│   ├── log/                # 测试日志输出
│   └── script/             # 数据处理脚本
├── docs/                   # 项目文档
│   ├── insn-decode.jpg     # 指令解码图示
│   └── insn.jpg            # 指令图示
├── riscv-isa-sim/          # Spike 子模块 (RISC-V ISA 模拟器)
├── src/                    # 源代码目录
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
│   │   ├── build/          # 构建输出目录
│   │   ├── custom/         # 自定义 RISC-V 扩展实现 (copied from C_src)
│   │   │   ├── config.h    # 配置文件
│   │   │   ├── util.c/h    # 工具函数
│   │   │   ├── riscv/      # RISC-V 向量扩展实现
│   │   │   │   ├── BF16.cpp/hpp    # BFloat16 处理单元
│   │   │   │   ├── custom_expp.cpp/hpp # BF16 e^x 近似计算
│   │   │   │   ├── MxFp8ActQuant.cpp/hpp # MXFP8 量化核心
│   │   │   │   ├── SoftmaxCore.cpp/hpp # Softmax 计算核心
│   │   │   │   ├── main_expp.cpp   # exp 单元测试主程序
│   │   │   │   ├── main_quant.cpp  # quant 单元测试主程序
│   │   │   │   └── main_softmax.cpp # softmax 单元测试主程序
│   │   │   │   └── single_expp.cpp # 单值 exp 调试程序
│   │   │   ├── gemm/       # GEMM 运算实现
│   │   │   ├── log/        # 测试日志输出
│   │   │   └── script/     # 数据处理脚本
│   │   ├── extensions/     # 自定义扩展实现
│   │   │   ├── BF16.cpp/hpp # BFloat16 处理单元
│   │   │   ├── custom_expp.cpp/hpp # BF16 e^x 近似计算
│   │   │   ├── MxFp8ActQuant.cpp/hpp # MXFP8 量化核心
│   │   │   ├── SoftmaxCore.cpp/hpp # Softmax 计算核心
│   │   │   ├── decode_macros.h
│   │   │   ├── insn_macros.h
│   │   │   ├── primitiveTypes.h
│   │   │   ├── specialize.h
│   │   │   ├── util.h
│   │   │   ├── v_ext_macros.h
│   │   │   ├── xperia.cc   # 标量扩展（加法）
│   │   │   └── xperiv.cc   # 向量扩展（加法、乘法及 AI 指令）
│   │   ├── firmware/       # 测试固件
│   │   │   ├── main.c      # 测试程序（包含 exp/softmax/quant 测试）
│   │   │   ├── start.S     # 启动代码
│   │   │   ├── script.ld   # 链接脚本
│   │   │   ├── util.c/h    # 工具函数
│   │   │   └── Makefile    # 固件构建配置
│   │   ├── spike_main.cc   # 自定义 Spike 主程序
│   │   └── Makefile        # 静态链接构建配置
├── src/xperimental/        # 自定义扩展实验
│   ├── xperimental_ext/    # 扩展实现 (.so 动态库)
│   ├── xperimental_sw/     # 测试软件
│   └── README.md           # 扩展使用指南
├── build_all.sh            # 统一构建脚本
├── exp_analysis.md         # 实验分析文档
├── IFLOW.md                # 项目文档
├── LICENSE                 # 许可证文件
├── README.md               # 项目总览
├── require.txt             # 项目需求说明
├── run_log_after_simplify.txt # 运行日志
├── set-env.sh              # 环境变量设置脚本
├── tags                    # 代码标签文件
├── test_error.c            # 测试错误文件
└── test_vlen.log           # 测试向量长度日志
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
# 若不可用，请手动设置 RISCV_PATH 等环境变量
```

### 3. 构建项目

```bash
# 使用统一构建脚本（推荐）
bash build_all.sh

# 或者只构建和运行测试
bash build_all.sh --run-tests

# 构建特定组件
bash build_all.sh --spike      # 仅构建 Spike
bash build_all.sh --firmware   # 仅构建固件
bash build_all.sh --top        # 仅构建顶层包装器
bash build_all.sh --clean      # 清理构建目录
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

#### Llama.cpp RISC-V 向量扩展单元测试

```bash
cd C_src
make all       # 编译所有单元测试
make expp      # 运行 BFloat16 exp 计算单元测试
make softmax   # 运行 Softmax 计算单元测试
make quant     # 运行 MxFP8 量化单元测试
make single ARGS=0x3F80  # 运行单值 exp 测试 (输入 1.0)
```

#### 静态链接 Spike 演示（包含 AI 指令扩展）

```bash
# 使用统一构建脚本
bash build_all.sh --run-tests

# 或者手动构建
cd src/top
make spike_build  # 构建 Spike 库
make              # 构建静态链接的 top-main
make run          # 运行包含 exp/softmax/quant 指令的测试程序（生成 log.txt 日志）

# 或使用统一构建目录中的文件运行测试
make run-build    # 使用 build/ 目录中的文件运行测试
```

#### 静态链接 Spike 演示（包含 AI 指令扩展）

```bash
# 使用统一构建脚本
bash build_all.sh --run-tests

# 或者手动构建
cd src/top
make spike_build  # 构建 Spike 库
make              # 构建静态链接的 top-main
make run          # 运行包含 exp/softmax/quant 指令的测试程序（生成 log.txt 日志）
```

### 5. 运行特定扩展测试

```bash
# 运行完整的 AI 指令测试
cd src/top
make clean
make spike_build
make
./build/top-main --isa=rv64imafdcv_zvl512b_zicsr_xperia_xperiv -l --log=log.txt --log-commits --instructions=2000 build/firmware/main.elf

# 使用 RISC-V 工具链编译固件
cd src/top/firmware
make clean
make all
```

## 详细构建说明

### 统一构建系统

项目现在使用 `build_all.sh` 作为统一的构建脚本，将所有构建输出集中到 `build/` 目录下。该脚本支持以下功能：

- 构建 Spike 模拟器
- 构建固件
- 构建顶层包装器
- 运行测试
- 管理构建依赖

**主要命令：**
- `bash build_all.sh` - 构建所有组件并运行测试
- `bash build_all.sh --all` - 构建所有组件并运行测试
- `bash build_all.sh --run-tests` - 构建并运行测试
- `bash build_all.sh --clean` - 清理构建目录
- `bash build_all.sh --spike` - 仅构建 Spike
- `bash build_all.sh --firmware` - 仅构建固件
- `bash build_all.sh --top` - 仅构建顶层包装器
- `bash build_all.sh --help` - 显示帮助信息
- `bash build_all.sh --riscv PATH` - 设置 RISC-V 工具链路径
- `bash build_all.sh --spike-src PATH` - 设置 Spike 源码路径

### Spike 构建配置

Spike 作为子模块位于 `riscv-isa-sim/` 目录。构建过程会自动配置和安装到 `build/spike-install`。

关键环境变量（通过 `set-env.sh` 设置）：
- `SPIKE_INSTALL_DIR`: Spike 安装目录
- `PATH`: 添加 Spike 二进制路径
- `LD_LIBRARY_PATH`: 添加 Spike 库路径
- 包含路径和库路径设置
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
- `make all` 或 `make`: 构建静态链接的 `top-main` 可执行文件
- `make spike_build`: 构建并安装 Spike 库
- `make reconfigure_spike`: 重新配置 Spike 构建
- `make run`: 运行测试程序（使用完整指令集，包含 AI 指令）
- `make run-build`: 使用构建目录中的文件运行测试
- `make clean`: 清理生成文件
- `make clean_spike`: 清理 Spike 构建
- `make distclean`: 清理所有生成文件

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

### 扩展开发

添加自定义扩展时：
1. 在 `src/top/extensions/` 或 `src/xperimental/xperimental_ext/` 中创建新文件
2. 参考 `xperia.cc` 和 `xperiv.cc` 实现扩展（需实现指令解码、执行和反汇编）
3. 通过静态链接方式加载
4. 提供对应的测试软件（参考 `src/top/firmware/main.c`）
5. 将算法实现从 `C_src/riscv/` 复制到 `src/top/custom/riscv/` 目录中，避免外部依赖

### 自定义扩展目录结构

项目采用双目录结构来 maintain 算法实现：
- `C_src/riscv/`: 独立的算法实现和单元测试
- `src/top/custom/riscv/`: 为 Spike 静态链接复制的算法实现

这种结构允许：
- 独立的算法开发和测试（在 C_src 中）
- 与 Spike 静态链接时的自包含实现（在 src/top/custom 中）
- 避免构建时的外部依赖问题

## 开发约定

### 代码组织

1. **模块分离**: 每个用例有独立目录，包含完整的构建和测试设施
2. **头文件管理**: 公共头文件放置在对应目录的根级别
3. **测试软件**: 每个演示都有对应的测试软件目录 (`sw/`)
4. **AI 扩展**: 算法实现在 `C_src/riscv/` 中，Spike 扩展在 `src/top/extensions/` 中

### 构建系统

- 使用 `build_all.sh` 统一管理构建过程
- 输出集中到 `build/` 目录
- 支持环境变量覆盖配置
- 提供 `clean` 目标确保可重复构建
- Spike 构建通过子模块和自动化脚本管理

### 扩展开发

添加自定义扩展时：
1. 在 `src/xperimental/` 中创建新目录
2. 参考 `xperia.cc` 和 `xperiv.cc` 实现扩展
3. 通过静态链接方式加载
4. 提供对应的测试软件
5. 将 C_src 中的算法实现复制到扩展目录，避免外部依赖

## 测试和验证

### 测试软件

每个演示都包含测试软件：
- `src/cpp/sw/`: C++ 演示的测试程序
- `src/systemc/sw/`: SystemC 演示的测试程序
- `src/top/firmware/`: 静态链接演示的测试固件（包含 AI 指令测试）
- `src/xperimental/xperimental_sw/`: 自定义扩展测试程序
- `C_src/riscv/`: Llama.cpp AI 扩展单元测试

### 运行验证

1. **基本功能验证**: 运行演示程序检查是否正确执行
2. **扩展验证**: 使用 Spike 的 `--extlib` 参数加载自定义扩展
3. **静态链接验证**: 使用 `top-main` 运行包含自定义扩展的程序
4. **AI 指令验证**: 使用 `top-main` 运行包含 exp/softmax/quant 指令的测试程序
5. **日志分析**: 通过 `-l` 和 `--log` 参数生成执行日志（默认生成 `log.txt`）

### 调试支持

- **SystemC 调试**: 使用 `--debug` 或 `-d` 参数启用调试输出
- **远程调试**: 支持远程 bitbang 调试 (`--rbb-port`)
- **JTAG 接口**: 集成 Spike 的 JTAG DTM 模块
- **自定义扩展调试**: 扩展实现中包含大量 fprintf 输出用于调试

## 测试框架和预期结果校验

项目实现了完整的测试框架，用于验证自定义扩展的功能正确性，特别针对 AI 推理中的数学运算进行了优化。

### 测试框架特性

1. **预期结果校验**: 每个自定义指令都有对应的预期结果验证
2. **错误报告机制**: 通过 `tohost` 机制报告测试失败和错误代码
3. **全面覆盖**: 支持标量、向量及 AI 指令的测试
4. **自动化验证**: 测试程序自动验证指令执行结果
5. **AI 精度测试**: 针对 BF16、MXFP8 等 AI 精度进行专门测试
6. **LMUL 测试**: 针对不同向量长度乘数（1/2/4/8）进行测试

### 测试错误代码

测试框架定义了以下错误代码：
- `ERR_XPERIA_ADD (0x10)`: XPERIA 标量加法扩展测试失败
- `ERR_XPERIV_ADD (0x30)`: XPERIV 向量加法扩展测试失败
- `ERR_XPERIV_MUL (0x40)`: XPERIVMUL 向量乘法扩展测试失败
- `ERR_EXP (0x50)`: EXP 向量指数运算扩展测试失败
- `ERR_SOFTMAX (0x60)`: SOFTMAX 向量 Softmax 运算扩展测试失败
- `ERR_QUANT (0x70)`: QUANT 向量量化扩展测试失败

### 当前测试状态

✅ **已通过验证的功能**:
- XPERIA 标量加法扩展 (`peri.a.add`)
- XPERIV 向量加法扩展 (`peri.v.add`)
- XPERIVMUL 向量乘法扩展 (`peri.v.mul`)
- EXP 向量指数运算扩展 (`exp`)
- SOFTMAX 向量 Softmax 运算扩展 (`softmax`)
- QUANT 向量量化扩展 (`quant`)

🔧 **已修复的问题**:
- EXP/softmax/quant 指令的 commit log 显示问题 - 已通过改进扩展实现修复
- 提升了 LMUL (1/2/4/8) 不同配置下的测试覆盖

### 运行测试

```bash
# 运行完整测试（包含 AI 指令）
bash build_all.sh --run-tests

# 或者手动运行
cd src/top
make run

# 查看测试日志
tail -f log.txt
cat build/log.txt  # 当使用 build_all.sh 时

# 运行 C_src 中的单元测试
cd C_src
make expp          # 运行 exp 单元测试
make softmax       # 运行 softmax 单元测试
make quant         # 运行 quant 单元测试
make single ARGS=0x3F80  # 运行单值 exp 测试 (输入 1.0)
```

### 测试程序结构

测试程序 (`src/top/firmware/main.c`) 包含：
1. **测试设置**: 初始化测试环境，设置向量扩展
2. **指令执行**: 执行所有自定义指令（包括 AI 指令）
3. **LMUL 配置**: 针对不同长度乘数 (1/2/4/8) 的测试
4. **结果验证**: 验证每个指令的执行结果
5. **错误处理**: 报告测试失败并传递错误代码
6. **成功报告**: 通过 `tohost` 机制报告测试通过

### 新增 LMUL 测试覆盖

最新的测试程序 (`src/top/firmware/main.c`) includes comprehensive tests for different LMUL (Vector Length Multiplier) configurations:
- **Test 7-10**: EXP instruction with LMUL=1, 2, 4, 8
- **Test 11-14**: SOFTMAX instruction with LMUL=1, 2, 4, 8
- **Test 15-18**: QUANT instruction with LMUL=1, 2, 4, 8

Each test verifies the instruction with different vector lengths, ensuring correct operation across all supported vector configurations. The tests use actual log data from the C_src unit tests to validate the hardware implementation against expected outputs.

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
   - 参考 C_src 中的单元测试验证算法正确性

6. **module load 命令不可用**
   - 手动设置 RISC-V 工具链路径：`export RISCV_PATH=/path/to/riscv/toolchain`
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
```

## 后续开发

根据 `require.txt` 的需求，已完成以下开发重点：
1. ✅ 理解现有代码架构
2. ✅ 实现新的顶层设计，将外部库与 Spike 静态链接（已在 `src/top/` 中实现）
3. ✅ 添加新的指令扩展（已添加 exp、softmax、quant 指令）
4. ✅ 测试和验证新实现（已实现测试框架，AI 指令验证通过）
5. ✅ 修复新增加指令的 commit_log 问题（已修复）
6. ✅ 整合构建系统到 `build_all.sh`（已实现）
7. ✅ 输出文件到 `build/` 目录（已实现）
8. ✅ 修复一元操作的打印问题（已修复）
9. ✅ 增加更多测试用例（已添加 LMUL 测试）

**注意**: 避免直接修改 `riscv-isa-sim/` 子模块中的代码，应通过外部层级和编译系统扩展功能。`src/top/` 目录展示了如何在不修改 Spike 源代码的情况下实现静态链接集成。

## 版本更新说明

### 新增功能
1. **统一构建系统** (`build_all.sh`): 集中化构建流程，输出到 `build/` 目录
2. **静态链接 Spike 集成** (`src/top/`): 实现了将自定义扩展与 Spike 静态链接的功能
3. **AI 指令扩展**: 添加了 exp、softmax、quant 指令用于 AI 推理加速
4. **C_src 目录**: 包含 Llama.cpp RISC-V 向量扩展的算法实现
5. **改进的构建系统**: 支持静态链接方式，避免动态库依赖问题
6. **完整测试框架**: 包含预期结果校验和错误报告机制
7. **LMUL 测试覆盖**: 增加了对不同向量长度乘数 (1/2/4/8) 的测试
8. **指令编码参考**: 根据 docs/insn-decode.jpg, docs/insn.jpg 实现扩展
9. **算法集成**: 将 C_src 中的代码复制到 `src/top/custom/riscv/` 目录，不再依赖外部目录
10. **扩展测试**: 提取 C_src 中对应 main 函数中的测试方法到测试函数中
11. **Firmware 复杂测试**: 增加更多测试到 firmware 中来测试新增的指令 quant/exp/softmax
12. **Custom 目录**: 创建 `src/top/custom/` 目录，包含完整的 RISC-V 扩展算法实现

### 修复改进
1. **Commit Log 问题**: 修复了 EXP/softmax/quant 指令在日志中不显示的问题
2. **指令打印格式**: 修复了一元操作指令（如 quant, exp）的打印格式
3. **算法集成**: 将 C_src 中的算法实现直接集成到扩展中
4. **测试验证**: 增强了测试用例，包括不同 LMUL 配置下的验证
5. **目录结构**: 改进了目录结构，将算法实现与扩展实现分离

### 使用建议
- 对于生产环境，推荐使用 `build_all.sh` 统一构建系统
- 对于开发和测试，可以使用 `bash build_all.sh --run-tests` 快速验证
- 参考 `src/top/Makefile` 了解如何集成新的扩展
- 扩展开发时，确保指令编码不与现有指令冲突（使用 CUSTOM0-CUSTOM3 操作码空间）
- AI 指令参考 `C_src/riscv/` 中的算法实现

### 已知限制
- 当前测试固件使用固定内存地址，可能不适用于所有内存布局
- SystemC 集成需要额外的 SystemC 库安装

## 贡献指南

1. 遵循现有代码风格和目录结构
2. 添加新扩展时，提供对应的测试程序
3. 更新文档（包括本文件）以反映变更
4. 确保构建系统向后兼容
5. 提交前运行现有测试：`bash build_all.sh --run-tests`
6. AI 扩展需同时更新 `C_src/` 和 `src/top/extensions/` 中的实现
7. 确保新的扩展指令正确记录到 commit log 中

## 许可证

本项目基于 Spike 的许可证（BSD 3-Clause）和自定义扩展的 MIT 许可证。详见各目录中的 LICENSE 文件。
