# RISC-V AI 指令扩展 BF16 计算单元项目

## 项目概述

这是 RISC-V ISA 模拟器库集成演示项目中的一个子模块，专注于 RISC-V 自定义指令扩展的 BF16（Brain Floating Point 16）精度计算单元测试。该项目包含针对 AI 推理中常用数学函数的 BF16 精度实现，包括 exp、softmax、quant 等自定义指令的参考模型实现和测试验证。

该项目主要实现和测试以下 AI 推理相关的 BF16 精度功能：

1. **Softmax 函数单元测试**: 实现 BF16 精度下的 softmax 函数，支持通过命令行参数指定不同的 ROW_LEN 值（32, 64, 128, 256, 512），用于 AI 模型的输出归一化。

2. **量化单元测试** (`quant_output.log`): 实现 BF16 到 MxFP8 的量化算法，包含对 BF16 值的全面测试，比较硬件量化与参考量化实现的误差。

3. **指数函数单元测试** (`expp.log`): 实现 BF16 精度下的指数函数（exp）近似计算，用于激活函数和概率计算。

4. **GEMM (通用矩阵乘法) 实现**: 包含支持 MxFP8 精度的 GEMM 运算实现，用于 AI 推理中的核心计算。

## 架构

### 目录结构
- `build/`: 编译输出目录，包含所有 .o 和 .d 文件
- `gemm/`: GEMM 相关的 MxFP8 精度运算实现
- `log/`: 测试日志文件存储目录
- `riscv/`: RISC-V 相关的核心计算单元和 BF16 实现
- `script/`: 辅助脚本目录（包括日志数据提取、分析和错误过滤脚本）
- `test/`: 测试文件目录，包含所有 main_*.cpp 测试入口文件
- `util.c/h`: 通用工具函数

### 构建系统
- Makefile 支持自动依赖跟踪（.d 文件）
- 所有 .o 和 .d 文件输出到 build 目录下
- 支持多种构建目标（gemm, quant, expp, softmax, single）
- 集成 RISC-V 指令扩展测试和验证
- CFLAGS/CXXFLAGS 使用 -std=c11/-std=c++11 标准，并启用 -fPIC 位置无关代码编译
- 包含 config.h 配置文件，定义通用宏和参数

## 测试内容

### Softmax 测试
- 支持通过命令行参数指定 ROW_LEN 值（32, 64, 128, 256, 512）
- 生成多种日志文件：softmax_output_m1.log, softmax_output_m2.log, softmax_output_m4.log, softmax_output_m8.log, softmax_output_m16.log
- 验证 BF16 精度下 softmax 函数的正确性
- 检查输出总和是否接近 1.0（归一化性质）
- 包含不同输入分布的测试用例
- 验证 RISC-V 自定义 softmax 指令的参考实现

### 量化测试
- 实现 BF16 到 MxFP8 的量化算法
- 比较硬件量化实现与参考实现的精度差异
- 记录量化误差百分比
- 针对 BF16 值范围进行全面测试
- 使用 shell 脚本 (`filter_errors.sh`) 过滤和分析错误输出

### 指数函数测试
- 验证 BF16 精度下的指数函数实现
- 实现 e^x 近似计算算法
- 验证精度和误差范围
- 针对 RISC-V 自定义 exp 指令的参考实现

### GEMM 测试
- 验证 MxFP8 精度的 GEMM 运算实现
- 测试矩阵乘法的数值精度
- 验证累加器精度和舍入模式

## 命令

### 构建命令
- `make`: 构建所有目标
- `make gemm`: 构建并运行 GEMM 测试
- `make quant`: 构建并运行 MxFP8 量化测试（运行后执行错误过滤脚本）
- `make expp`: 构建并运行 BF16 e^x 近似测试
- `make single`: 构建并运行单值 expp 测试 (使用 ARGS=0x8000)
- `make softmax`: 构建并运行 SoftmaxCore 测试，遍历 ROW_LEN=32,64,128,256,512
- `make softmax_all`: 与 softmax 目标相同，运行所有 ROW_LEN 设置
- `make clean`: 清理构建产物
- `make run`: 运行所有测试
- `make extract_riscv_arrays`: 从日志中提取 RISC-V 指令参考数组
- `make help`: 显示可用的构建目标

### Softmax 命令行参数
- `./build/test_softmax <ROW_LEN>`: 运行 Softmax 测试，指定 ROW_LEN 值
- ROW_LEN 必须是 32 的倍数
- 输出日志文件名格式为 softmax_output_mN.log，其中 N = ROW_LEN >> 5

## 工具与脚本

### RISC-V 指令数组提取工具 (`extract_riscv_insn_ref_arrays.py`)
- 从日志文件中提取 Input_BF16 和 Output_BF16 数组
- 按照索引对应关系组织数据
- 根据文件后缀（m1, m2, m4, m8, m16）自动计算分组大小
- 输出格式为 C 语言的 uint16_t 数组，便于集成到 RISC-V 指令扩展中
- 分组规则：m值对应的组大小为 32 << (m-1)，例如 m1=32, m2=64, m4=256, m8=4096, m16=1048576

### 量化错误过滤脚本 (`filter_errors.sh`)
- 对量化测试输出进行错误分析和过滤
- 用于分析测试结果中的错误模式

### 误差分析脚本 (`expp_error_plot.py`, `plot_errors.py`)
- 分析和可视化指数函数计算中的误差
- 生成误差图表和统计信息

### 构建系统特性
- 启用自动依赖跟踪（-MMD -MP 编译选项）
- 所有 .o 和 .d 文件输出到 build 目录下
- 支持并行构建
- 清理时删除所有 .d 文件
- 集成 RISC-V 指令扩展构建流程
- 包含配置头文件，支持批量大小等参数配置

## 文件说明

- `config.h`: 项目配置文件，定义通用宏和参数（EXP_WIDTH, MAT_WIDTH, EXT_WIDTH, ROW_SIZE, COL_SIZE, BATCH_SIZE）
- `Makefile`: 构建脚本，支持多种目标和自动依赖跟踪
- `util.c/h`: 通用工具函数实现
- `test_quant.cpp`: 量化测试代码（独立量化测试）
- `gemm/*`: GEMM 相关实现（MxFP8 精度矩阵运算）
  - `Fp32PsumAcc.cpp/hpp`: FP32 累加器实现
  - `GemmTop.cpp/hpp`: GEMM 顶层模块
  - `MxFp8ActDeNorm.cpp/hpp`: MxFP8 激活去归一化实现
  - `MxFp8Product.cpp/hpp`: MxFP8 乘积运算实现
  - `Readme.md`: GEMM 模块说明文档
- `riscv/*`: RISC-V 相关核心实现，包含 BF16、SoftmaxCore、自定义 expp 等
  - `BF16.cpp/hpp`: BF16 精度处理单元
  - `custom_expp.cpp/hpp`: BF16 e^x 近似计算单元
  - `MxFp8ActQuant.cpp/hpp`: MxFP8 激活量化单元
  - `SoftmaxCore.cpp/hpp`: Softmax 核心计算单元
  - `Readme.md`: RISC-V 模块说明文档
- `test/*`: 测试文件目录，包含所有 main_*.cpp 和 single_*.cpp 测试入口文件
  - `main_expp.cpp`: exp 单元测试主程序
  - `main_gemm.cpp`: GEMM 单元测试主程序
  - `main_quant.cpp`: quant 单元测试主程序
  - `main_softmax.cpp`: softmax 单元测试主程序
  - `single_expp.cpp`: 单值 exp 调试程序
- `log/`: 日志目录，包含所有测试输出
- `build/`: 构建输出目录，包含所有 .o 和 .d 文件
- `softmax_output_m*.log`: Softmax 函数测试输出，不同后缀表示不同的 ROW_LEN 设置
- `quant_output.log`: 量化算法测试输出
- `expp.log`: 指数函数测试输出
- `extract_riscv_insn_ref_arrays.py`: 从日志文件提取 RISC-V 指令参考数组并转换为 C 语言格式的脚本
- `filter_errors.sh`: 量化测试错误过滤和分析脚本
- `expp_error_plot.py`: 指数函数误差绘图脚本
- `plot_errors.py`: 误差分析绘图脚本
- `IFLOW.md`: 项目说明文件
- `TODO`: 项目待办事项

## 配置参数

- `EXP_WIDTH`: 指数位宽度（默认 8）
- `MAT_WIDTH`: 尾数位宽度（默认 24）
- `EXT_WIDTH`: 累加扩展位宽度（默认 4）
- `ROW_SIZE`: 行大小（默认 16）
- `COL_SIZE`: 列大小（默认 16）
- `BATCH_SIZE`: 批处理大小（默认 1）

## 开发惯例

该项目主要关注 AI 推理中数学函数的 BF16 精度实现和验证，为 RISC-V 自定义指令扩展提供参考模型。项目实现了 exp、softmax、quant 等 AI 推理核心函数的 BF16 精度算法，并通过全面的测试验证算法正确性。

构建系统采用现代 Makefile 最佳实践，包括自动依赖跟踪、构建产物分离和清理功能，确保项目结构的整洁性和构建的可靠性。测试框架覆盖了不同输入规模和精度要求，确保在 RISC-V 指令扩展中使用 BF16 精度时的正确性和可靠性。

Python 脚本用于自动化数据提取和格式转换过程，便于将测试结果集成到 RISC-V 指令扩展的硬件实现中。

项目通过 `TODO` 文件跟踪开发任务，当前已完成对 softmax 命令行参数的支持，允许通过 make 命令遍历不同 ROW_LEN 设置（32/64/128/256/512）。