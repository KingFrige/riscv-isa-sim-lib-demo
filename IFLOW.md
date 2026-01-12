# RISC-V ISA 模拟器库集成演示项目

## 项目概述

本项目演示如何将 [RISC-V ISA 模拟器 (Spike)](https://github.com/riscv-software-src/riscv-isa-sim) 作为库使用，并与外部模拟器环境集成。

目前包含三个主要用例：
1. **C++ 内存模拟器集成** - 将 Spike 与自定义 C++ 内存模拟器连接
2. **SystemC 包装器** - 将 Spike 嵌入 SystemC 环境，创建完整的系统级仿真平台
3. **静态链接 Spike** - 将自定义扩展与 Spike 静态链接，创建独立的可执行文件（已实现）

此外，项目还包含一个实验性扩展示例 (`src/xperimental`)，展示了如何为 Spike 添加自定义指令扩展。

## 技术栈

- **核心模拟器**: RISC-V ISA Simulator (Spike)
- **编程语言**: C++ (主程序), C (测试软件), SystemC (系统级建模)
- **构建系统**: GNU Make, Autotools (Spike 子模块)
- **仿真环境**: SystemC 2.3+ (可选)
- **工具链**: RISC-V GNU 工具链 (需支持 rv64imafdcv 架构)

## 目录结构

```
.
├── riscv-isa-sim/          # Spike 子模块 (RISC-V ISA 模拟器)
├── src/                    # 源代码目录
│   ├── cpp/                # C++ 内存模拟器集成
│   │   ├── sw/             # 测试软件
│   │   ├── util/           # 工具函数
│   │   ├── demo_core.cc    # 核心演示逻辑
│   │   ├── memory_simulator.cc # 内存模拟器实现
│   │   ├── main.cc         # 主程序入口
│   │   └── Makefile        # 构建配置
│   ├── top/       # 静态链接 Spike 集成
│   │   ├── extensions/     # 自定义扩展实现
│   │   │   ├── decode_macros.h
│   │   │   ├── insn_macros.h
│   │   │   ├── primitiveTypes.h
│   │   │   ├── specialize.h
│   │   │   ├── v_ext_macros.h
│   │   │   ├── xperia.cc   # 标量扩展（加法）
│   │   │   ├── xperib.cc   # 新增标量扩展（乘法）
│   │   │   ├── xperiv.cc   # 向量扩展（加法）
│   │   │   └── xperiv_mul.cc # 新增向量乘法扩展
│   │   ├── firmware/       # 测试固件
│   │   │   ├── main.c      # 测试程序
│   │   │   ├── start.S     # 启动代码
│   │   │   ├── script.ld   # 链接脚本
│   │   │   └── Makefile    # 固件构建配置
│   │   ├── spike_main.cc   # 自定义 Spike 主程序
│   │   ├── Makefile        # 静态链接构建配置
│   │   └── spike-static    # 生成的静态链接可执行文件
│   ├── systemc/            # SystemC 集成
│   │   ├── memory/         # 内存模型
│   │   ├── turbo/          # 处理器核心包装器
│   │   ├── uncore/         # 非核心逻辑
│   │   ├── util/           # 工具函数
│   │   ├── sw/             # 测试软件
│   │   ├── sc_main.cpp     # SystemC 主程序
│   │   ├── Makefile        # 构建配置
│   │   └── README.md       # 详细使用说明
│   └── xperimental/        # 自定义扩展实验
│       ├── xperimental_ext/ # 扩展实现 (.so 动态库)
│       ├── xperimental_sw/  # 测试软件
│       └── README.md        # 扩展使用指南
├── build-spike.sh          # 构建 Spike 脚本
├── set-env.sh              # 环境变量设置脚本
├── require.txt             # 项目需求说明
├── README.md               # 项目总览
└── .gitmodules             # Git 子模块配置
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

### 2. 构建 Spike

```bash
# 运行构建脚本
bash build-spike.sh
```

### 3. 设置环境变量

```bash
# 设置 Spike 路径和环境变量
source set-env.sh

# 注意：set-env.sh 中包含 module load 命令，仅在支持 module 的环境中使用
# 若不可用，请手动设置 RISCV_PATH 等环境变量
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

#### 静态链接 Spike 演示

```bash
cd src/top
make spike_build  # 构建 Spike 库
make              # 构建静态链接的 spike-static
make run          # 运行测试程序（生成 log.txt 日志）
```

## 详细构建说明

### Spike 构建配置

Spike 作为子模块位于 `riscv-isa-sim/` 目录。构建过程会自动配置和安装到 `riscv-isa-sim/install/`。

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

### 静态链接 Spike 构建

`src/top/Makefile` 提供了完整的静态链接构建流程：

**主要目标：**
- `make all` 或 `make`: 构建静态链接的 `spike-static` 可执行文件
- `make spike_build`: 构建并安装 Spike 库
- `make reconfigure_spike`: 重新配置 Spike 构建
- `make run`: 运行测试程序（使用完整指令集）
- `make clean`: 清理生成文件
- `make clean_spike`: 清理 Spike 构建
- `make distclean`: 清理所有生成文件

**构建特点：**
- 将自定义扩展（xperia, xperib, xperiv, xperiv_mul）直接编译到可执行文件中
- 支持 `--isa=rv64imafdcv_zicsr_xperia_xperiv_xperib_xperivmul` 指令集
- 无需动态加载扩展库
- 测试固件自动编译并链接

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

3. **XPERIB** - 新增标量扩展，包含 1 条指令 `peri.b.mul`
   - 操作码：0x4b (CUSTOM2)
   - 功能：标量乘法，`rd = rs1 * rs2`

4. **XPERIV_MUL** - 新增向量乘法扩展，包含 1 条指令 `peri.v.mul`
   - 操作码：0x5b (CUSTOM3)
   - 功能：向量乘法，`vd[i] = vs1[i] * vs2[i]`

### 扩展开发

添加自定义扩展时：
1. 在 `src/top/extensions/` 或 `src/xperimental/xperimental_ext/` 中创建新文件
2. 参考 `xperia.cc` 和 `xperiv.cc` 实现扩展（需实现指令解码、执行和反汇编）
3. 通过动态库 (`*.so`) 或静态链接方式加载
4. 提供对应的测试软件（参考 `src/top/firmware/main.c`）

## 开发约定

### 代码组织

1. **模块分离**: 每个用例有独立目录，包含完整的构建和测试设施
2. **头文件管理**: 公共头文件放置在对应目录的根级别
3. **测试软件**: 每个演示都有对应的测试软件目录 (`sw/`)

### 构建系统

- 使用 Makefile 管理构建过程
- 支持环境变量覆盖配置
- 提供 `clean` 目标确保可重复构建
- Spike 构建通过子模块和自动化脚本管理

### 扩展开发

添加自定义扩展时：
1. 在 `src/xperimental/` 中创建新目录
2. 参考 `xperia.cc` 和 `xperiv.cc` 实现扩展
3. 通过动态库 (`*.so`) 方式加载
4. 提供对应的测试软件

## 测试和验证

### 测试软件

每个演示都包含测试软件：
- `src/cpp/sw/`: C++ 演示的测试程序
- `src/systemc/sw/`: SystemC 演示的测试程序
- `src/top/firmware/`: 静态链接演示的测试固件
- `src/xperimental/xperimental_sw/`: 自定义扩展测试程序

### 运行验证

1. **基本功能验证**: 运行演示程序检查是否正确执行
2. **扩展验证**: 使用 Spike 的 `--extlib` 参数加载自定义扩展
3. **静态链接验证**: 使用 `spike-static` 运行包含自定义扩展的程序
4. **日志分析**: 通过 `-l` 和 `--log` 参数生成执行日志（默认生成 `log.txt`）

### 调试支持

- **SystemC 调试**: 使用 `--debug` 或 `-d` 参数启用调试输出
- **远程调试**: 支持远程 bitbang 调试 (`--rbb-port`)
- **JTAG 接口**: 集成 Spike 的 JTAG DTM 模块

## 测试框架和预期结果校验

项目实现了完整的测试框架，用于验证自定义扩展的功能正确性。

### 测试框架特性

1. **预期结果校验**: 每个自定义指令都有对应的预期结果验证
2. **错误报告机制**: 通过 `tohost` 机制报告测试失败和错误代码
3. **全面覆盖**: 支持标量和向量指令的测试
4. **自动化验证**: 测试程序自动验证指令执行结果

### 测试错误代码

测试框架定义了以下错误代码：
- `ERR_XPERIA_ADD (0x10)`: XPERIA 标量加法扩展测试失败
- `ERR_XPERIB_MUL (0x20)`: XPERIB 标量乘法扩展测试失败  
- `ERR_XPERIV_ADD (0x30)`: XPERIV 向量加法扩展测试失败
- `ERR_XPERIV_MUL (0x40)`: XPERIVMUL 向量乘法扩展测试失败

### 当前测试状态

✅ **已通过验证的功能**:
- XPERIA 标量加法扩展 (`peri.a.add`)
- XPERIB 标量乘法扩展 (`peri.b.mul`)

🔧 **需要进一步调试的功能**:
- XPERIV 向量加法扩展 (`peri.v.add`) - 向量扩展可能需额外配置
- XPERIVMUL 向量乘法扩展 (`peri.v.mul`) - 向量扩展可能需额外配置

### 运行测试

```bash
# 运行完整测试
cd src/top
make run

# 查看测试日志
tail -f log.txt

# 仅运行标量指令测试（简化版）
cd src/top/firmware
RISCV_PATH=/path/to/riscv/toolchain make -f Makefile.simple  # 如存在
cd ..
./spike-static --isa=rv64imafdc_zicsr_xperia_xperib firmware/simple.elf
```

### 测试程序结构

测试程序 (`src/top/firmware/main.c`) 包含：
1. **测试设置**: 初始化测试环境，设置向量扩展
2. **指令执行**: 执行所有自定义指令
3. **结果验证**: 验证每个指令的执行结果
4. **错误处理**: 报告测试失败并传递错误代码
5. **成功报告**: 通过 `tohost` 机制报告测试通过

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
   - 确保 Spike 库已正确构建和安装：`make spike_build`
   - 检查扩展文件路径和编译选项：`ls src/top/extensions/`
   - 验证依赖库路径设置：`echo $LIBRARY_PATH`
   - 检查编译器版本：`g++ --version`（需要支持 C++17）

5. **module load 命令不可用**
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

# 检查 SystemC 安装
ls $SYSTEMC_INCLUDE/systemc.h 2>/dev/null || echo "SystemC not found"

# 检查 RISC-V 工具链
which riscv64-unknown-elf-gcc

# 检查 Spike 安装
ls riscv-isa-sim/install/bin/spike 2>/dev/null || echo "Spike not installed"
```

## 后续开发

根据 `require.txt` 的需求，后续开发重点包括：
1. 理解现有代码架构（已完成）
2. 实现新的顶层设计，将外部库与 Spike 静态链接（已在 `src/top/` 中实现）
3. 添加新的指令扩展（已添加 xperib 和 xperiv_mul）
4. 测试和验证新实现（已实现测试框架，标量指令验证通过）

**注意**: 避免直接修改 `riscv-isa-sim/` 子模块中的代码，应通过外部层级和编译系统扩展功能。`src/top/` 目录展示了如何在不修改 Spike 源代码的情况下实现静态链接集成。

## 版本更新说明

### 新增功能
1. **静态链接 Spike 集成** (`src/top/`): 实现了将自定义扩展与 Spike 静态链接的功能
2. **新增扩展**: 添加了 xperib 和 xperiv_mul 扩展
3. **改进的构建系统**: 支持静态和动态两种扩展加载方式
4. **完整测试框架**: 包含预期结果校验和错误报告机制

### 使用建议
- 对于生产环境，推荐使用静态链接方式，避免动态库依赖问题
- 对于开发和测试，可以使用动态加载方式快速迭代
- 参考 `src/top/Makefile` 了解如何集成新的扩展
- 扩展开发时，确保指令编码不与现有指令冲突（使用 CUSTOM0-CUSTOM3 操作码空间）

### 已知限制
- 向量扩展测试可能需要额外的向量长度和配置设置
- 当前测试固件使用固定内存地址，可能不适用于所有内存布局
- SystemC 集成需要额外的 SystemC 库安装

## 贡献指南

1. 遵循现有代码风格和目录结构
2. 添加新扩展时，提供对应的测试程序
3. 更新文档（包括本文件）以反映变更
4. 确保构建系统向后兼容
5. 提交前运行现有测试：`cd src/top && make run`

## 许可证

本项目基于 Spike 的许可证（BSD 3-Clause）和自定义扩展的 MIT 许可证。详见各目录中的 LICENSE 文件。
