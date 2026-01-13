#!/usr/bin/env python3
"""
从日志文件中提取 Input_BF16 和 Output_BF16 数组，并按要求分组
"""

import re
import os
from typing import List, Tuple, Dict


def extract_softmax_arrays(file_path: str) -> Tuple[List[str], List[str]]:
    """从 softmax 日志文件中提取 Input_BF16 和 Output_BF16 数组"""
    with open(file_path, 'r') as f:
        content = f.read()
    
    # 使用正则表达式匹配 Softmax Input/Output Table 部分的数据
    table_pattern = r"Softmax Input/Output Table:[\s\S]*?-{5,}\n([\s\S]*?)\n\s*Sum check:"
    match = re.search(table_pattern, content)
    
    if not match:
        print(f"未在 {file_path} 中找到 Softmax Input/Output Table")
        return [], []
    
    table_content = match.group(1)
    
    input_bf16_values = []
    output_bf16_values = []
    
    # 匹配表格中的数据行
    lines = table_content.split('\n')
    for line in lines:
        # 匹配格式: "  0 |   0xbf2a    |  -6.640625e-01  |   0x3d09    | 0.03344727"
        pattern = r'\s*\d+\s*\|\s*(0x[0-9a-fA-F]+)\s*\|.*?\|\s*(0x[0-9a-fA-F]+)\s*\|'
        match = re.search(pattern, line)
        if match:
            input_bf16 = match.group(1)
            output_bf16 = match.group(2)
            input_bf16_values.append(input_bf16)
            output_bf16_values.append(output_bf16)
    
    return input_bf16_values, output_bf16_values


def extract_expp_arrays(file_path: str) -> Tuple[List[str], List[str]]:
    """从 expp 日志文件中提取 Input_BF16 和 Output_BF16 数组"""
    with open(file_path, 'r') as f:
        content = f.read()
    
    input_bf16_values = []
    output_bf16_values = []
    
    # 匹配 expp.log 中的格式: "Index    Input                  HW_Result              Expected(F32)"
    lines = content.split('\n')
    for line in lines:
        # 匹配格式: "0        0x0000(+0.000e+00)      0x3f80(+1.000e+00)      +1.00000000e+00"
        pattern = r'\s*(\d+)\s+(0x[0-9a-fA-F]+)\([^)]*\)\s+(0x[0-9a-fA-F]+)\([^)]*\)\s+'
        match = re.search(pattern, line)
        if match:
            input_bf16 = match.group(2)
            output_bf16 = match.group(3)
            input_bf16_values.append(input_bf16)
            output_bf16_values.append(output_bf16)
    
    return input_bf16_values, output_bf16_values


def extract_quant_arrays(file_path: str) -> Tuple[List[str], List[str]]:
    """从 quant 日志文件中提取 Input_BF16 和 Output_BF16 数组"""
    with open(file_path, 'r') as f:
        content = f.read()
    
    input_bf16_values = []
    output_bf16_values = []
    
    # 匹配 quant_output.log 中的格式
    lines = content.split('\n')
    for line in lines:
        # 匹配格式: "    0 | 0x0000(+0.000e+00) |   0x00   | +0.000000e+00   |     0.00% |   0x00    | +0.000000e+00   |    0.00%"
        pattern = r'\s*\d+\s*\|\s*(0x[0-9a-fA-F]+)\([^)]*\)\s*\|.*?\|.*?\|.*?\|\s*(0x[0-9a-fA-F]+)\s*\|'
        match = re.search(pattern, line)
        if match:
            input_bf16 = match.group(1)
            output_bf16 = match.group(2)
            input_bf16_values.append(input_bf16)
            output_bf16_values.append(output_bf16)
    
    return input_bf16_values, output_bf16_values


def calculate_group_size(file_path: str) -> int:
    """根据文件名计算分组大小"""
    filename = os.path.basename(file_path)
    
    # 检查是否包含 m1, m2, m4, m8, m16 等模式
    m_pattern = r'm(\d+)'
    match = re.search(m_pattern, filename)
    
    if match:
        m_value = int(match.group(1))
        group_size = 32 << (m_value - 1)  # 32 * 2^(m_value-1)
    else:
        # 默认为 m1，即 32 个元素一组
        group_size = 32
    
    return group_size


def group_arrays(input_bf16: List[str], output_bf16: List[str], group_size: int) -> Tuple[List[List[str]], List[List[str]]]:
    """按指定大小分组数组"""
    input_groups = []
    output_groups = []
    
    for i in range(0, len(input_bf16), group_size):
        input_group = input_bf16[i:i + group_size]
        output_group = output_bf16[i:i + group_size]
        
        if input_group or output_group:  # 确保至少有一个非空
            input_groups.append(input_group)
            output_groups.append(output_group)
    
    return input_groups, output_groups


def process_softmax_files_c_format():
    """处理所有 softmax 文件，输出 C 语言格式"""
    print("处理 softmax 文件...")
    
    # 获取所有 softmax 文件
    softmax_files = []
    for file in os.listdir('.'):
        if file.startswith('softmax_output_') and file.endswith('.log'):
            softmax_files.append(file)
    
    for file in sorted(softmax_files):
        print(f"\n处理文件: {file}")
        
        # 提取数组
        input_bf16, output_bf16 = extract_softmax_arrays(file)
        print(f"提取到 {len(input_bf16)} 个输入-输出对")
        
        # 计算分组大小
        group_size = calculate_group_size(file)
        print(f"分组大小: {group_size}")
        
        # 分组
        input_groups, output_groups = group_arrays(input_bf16, output_bf16, group_size)
        print(f"分成了 {len(input_groups)} 组")
        
        # 提取文件名基础部分（去掉路径和扩展名）
        base_filename = os.path.splitext(os.path.basename(file))[0]
        
        # 输出 C 语言格式的数组
        print_c_array_for_all_groups(input_groups, output_groups, base_filename, 2)
        print()


def process_expp_file_c_format():
    """处理 expp.log 文件，输出 C 语言格式"""
    print("处理 expp.log 文件...")
    
    if os.path.exists('expp.log'):
        # 提取数组
        input_bf16, output_bf16 = extract_expp_arrays('expp.log')
        print(f"提取到 {len(input_bf16)} 个输入-输出对")
        
        # 对于 expp.log，我们使用默认的 32 个元素一组
        group_size = 32
        print(f"分组大小: {group_size}")
        
        # 分组
        input_groups, output_groups = group_arrays(input_bf16, output_bf16, group_size)
        print(f"分成了 {len(input_groups)} 组")
        
        # 输出 C 语言格式的数组
        print_c_array_for_all_groups(input_groups, output_groups, 'expp', 2)
        print()


def process_quant_file_c_format():
    """处理 quant_output.log 文件，输出 C 语言格式"""
    print("处理 quant_output.log 文件...")
    
    if os.path.exists('quant_output.log'):
        # 提取数组
        input_bf16, output_bf16 = extract_quant_arrays('quant_output.log')
        print(f"提取到 {len(input_bf16)} 个输入-输出对")
        
        # 对于 quant_output.log，我们使用默认的 32 个元素一组
        group_size = 32
        print(f"分组大小: {group_size}")
        
        # 分组
        input_groups, output_groups = group_arrays(input_bf16, output_bf16, group_size)
        print(f"分成了 {len(input_groups)} 组")
        
        # 输出 C 语言格式的数组
        print_c_array_for_all_groups(input_groups, output_groups, 'quant', 2)
        print()


def print_c_array(input_groups: List[List[str]], output_groups: List[List[str]], base_filename: str, group_idx: int):
    """打印 C 语言格式的 uint16_t 数组"""
    input_group = input_groups[group_idx]
    output_group = output_groups[group_idx]
    
    # 转换为 C 语言格式的 uint16_t 数组
    print(f"// Group {group_idx+1} - {base_filename}")
    print(f"uint16_t input_{base_filename}_group_{group_idx+1}[{len(input_group)}] = {{")
    for i, val in enumerate(input_group):
        if i % 8 == 0:  # 每行8个元素，便于阅读
            print(f"    {val}", end="")
        else:
            print(f", {val}", end="")
        if (i + 1) % 8 == 0:
            print()  # 换行
    if len(input_group) % 8 != 0:
        print()  # 如果最后一行不足8个元素，换行
    print("};")
    
    print(f"uint16_t output_{base_filename}_group_{group_idx+1}[{len(output_group)}] = {{")
    for i, val in enumerate(output_group):
        if i % 8 == 0:  # 每行8个元素，便于阅读
            print(f"    {val}", end="")
        else:
            print(f", {val}", end="")
        if (i + 1) % 8 == 0:
            print()  # 换行
    if len(output_group) % 8 != 0:
        print()  # 如果最后一行不足8个元素，换行
    print("};")
    print()  # 额外换行，分隔不同数组


def print_c_array_for_all_groups(input_groups: List[List[str]], output_groups: List[List[str]], base_filename: str, num_groups_to_print: int = 2):
    """为所有组打印 C 语言格式的 uint16_t 数组"""
    print(f"处理 {base_filename} 文件，共 {len(input_groups)} 组数据")
    print(f"打印前 {min(num_groups_to_print, len(input_groups))} 组数据：")
    
    for i in range(min(num_groups_to_print, len(input_groups))):
        print_c_array(input_groups, output_groups, base_filename, i)


def main():
    """主函数"""
    print("开始处理日志文件...")
    
    # 处理 softmax 文件
    process_softmax_files_c_format()
    
    # 处理 expp 文件
    process_expp_file_c_format()
    
    # 处理 quant 文件
    process_quant_file_c_format()
    
    print("处理完成！")


if __name__ == "__main__":
    main()
