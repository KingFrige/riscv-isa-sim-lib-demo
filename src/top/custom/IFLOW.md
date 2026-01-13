# 3DDRAM C/C++ Models 项目

## 项目概述

这是一个用于 3DDRAM 项目的 C/C++ 模型实现，专注于 BF16（Brain Floating Point 16）精度的计算单元测试。该项目包含对关键数学函数的测试模型，特别是针对神经网络推理中常用的函数进行的测试。

该项目主要测试以下组件的 BF16 精度实现：

1. **Softmax 函数单元测试**: 测试 BF16 精度下的 softmax 函数实现，支持通过命令行参数指定不同的 ROW_LEN 值（32, 64, 128, 256, 512），并生成相应的日志文件（softmax_output_m1.log 到 softmax_output_m16.log）。

2. **量化单元测试** (`quant_output.log`): 测试 BF16 精度下的量化算法，包含对 65536 个 BF16 值的全面测试，比较硬件量化与参考量化实现的误差。

3. **指数函数单元测试** (`expp.log`): 测试 BF16 精度下的指数函数（exp）实现，对所有 65536 个 BF16 值进行穷举测试，验证精度和误差。

4. **GEMM (通用矩阵乘法) 测试**: 测试矩阵乘法的模型实现。

## 架构

### 目录结构
- `build/`: 编译输出目录，包含所有 .o 和 .d 文件
- `gemm/`: GEMM 相关的模型实现
- `log/`: 日志文件存储目录
- `riscv/`: RISC-V 相关的核心模型和实现
- `script/`: 辅助脚本目录
- `test/`: 测试文件目录，包含所有 main_*.cpp 测试入口文件
- `util.c/h`: 通用工具函数

### 构建系统
- Makefile 支持自动依赖跟踪（.d 文件）
- 所有 .o 和 .d 文件输出到 build 目录下
- 支持多种构建目标（gemm, quant, expp, softmax, single）

## 测试内容

### Softmax 测试
- 支持通过命令行参数指定 ROW_LEN 值
- 生成多种日志文件：softmax_output_m1.log, softmax_output_m2.log, softmax_output_m4.log, softmax_output_m8.log, softmax_output_m16.log
- 验证 BF16 精度下 softmax 函数的正确性
- 检查输出总和是否接近 1.0（归一化性质）
- 包含不同输入分布的测试用例

### 量化测试
- 比较硬件量化实现与参考实现的精度差异
- 记录量化误差百分比
- 针对所有可能的 BF16 值进行测试

### 指数函数测试
- 验证 BF16 精度下的指数函数实现
- 设定误差阈值（相对误差 > 500000.00% 或绝对误差 > 0.800）
- 测试所有 65536 个 BF16 值

## 命令

### 构建命令
- `make`: 构建所有目标
- `make gemm`: 构建并运行 GEMM 测试
- `make quant`: 构建并运行 MxFP8 量化测试
- `make expp`: 构建并运行 BF16 e^x 近似测试
- `make single`: 构建并运行单值 expp 测试 (使用 ARGS=0x8000)
- `make softmax`: 构建并运行 SoftmaxCore 黄金模型测试，遍历 ROW_LEN=32,64,128,256,512
- `make clean`: 清理构建产物
- `make run`: 运行所有测试

### Softmax 命令行参数
- `./build/test_softmax <ROW_LEN>`: 运行 Softmax 测试，指定 ROW_LEN 值
- ROW_LEN 必须是 32 的倍数
- 输出日志文件名格式为 softmax_output_mN.log，其中 N = ROW_LEN >> 5

## 工具与脚本

### Python 提取工具 (`extract_arrays.py`)
- 从日志文件中提取 Input_BF16 和 Output_BF16 数组
- 按照索引对应关系组织数据
- 根据文件后缀（m1, m2, m4, m8, m16）自动计算分组大小
- 输出格式为 C 语言的 uint16_t 数组，便于集成到 C/C++ 项目中
- 分组规则：m值对应的组大小为 32 << (m-1)，例如 m1=32, m2=64, m4=256, m8=4096, m16=1048576

### 构建系统特性
- 启用自动依赖跟踪（-MMD -MP 编译选项）
- 所有 .o 和 .d 文件输出到 build 目录下
- 支持并行构建
- 清理时删除所有 .d 文件

## 文件说明

- `config.h`: 项目配置文件
- `Makefile`: 构建脚本，支持多种目标和自动依赖跟踪
- `util.c/h`: 通用工具函数实现
- `test_quant.cpp`: 量化测试代码
- `gemm/*`: GEMM 相关实现
- `riscv/*`: RISC-V 相关核心实现，包含 BF16、SoftmaxCore、自定义 expp 等
- `test/*`: 测试文件目录，包含所有 main_*.cpp 和 single_*.cpp 测试入口文件
- `log/`: 日志目录，包含所有测试输出
- `build/`: 构建输出目录，包含所有 .o 和 .d 文件
- `softmax_output_m*.log`: Softmax 函数测试输出，不同后缀表示不同的 ROW_LEN 设置
- `quant_output.log`: 量化算法测试输出
- `expp.log`: 指数函数测试输出
- `extract_arrays.py`: 从日志文件提取数组并转换为 C 语言格式的脚本
- `IFLOW.md`: 项目说明文件
- `TODO`: 项目待办事项

## 开发惯例

该项目主要关注数值精度和算法验证，测试覆盖了所有可能的 BF16 输入值，以确保在 3DDRAM 系统中使用 BF16 精度时的正确性和可靠性。新增了 Python 脚本来自动化数据提取和格式转换过程，便于将测试结果集成到 C/C++ 项目中。

构建系统采用现代 Makefile 最佳实践，包括自动依赖跟踪、构建产物分离和清理功能，确保项目结构的整洁性和构建的可靠性。