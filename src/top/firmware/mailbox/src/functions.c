#include "firmware.h"
#include <stdint.h>

// 初始化函数
void firmware_init(void) {
    printf("========================================\n");
    printf("RISC-V Firmware Initializing...\n");
    printf("Mailbox Base: 0x%lx\n", (uint64_t)MAILBOX_BASE);
    printf("========================================\n");
}

// 状态报告函数
void report_status(void) {
    // 简化版本，移除静态变量以避免重定位问题
    printf("[STATUS] Commands processed: N/A\n"); // 简化，不统计数量
}

// 性能测量函数（简单的周期计数）
void start_timer(void) {
    // 在实际系统中，这里会读取时间计数器
    // 例如：读取 mcycle CSR
}

uint64_t stop_timer(void) {
    // 在实际系统中，这里会计算经过的周期数
    return 0;
}

void log_performance(const char* operation, uint64_t cycles) {
    printf("[PERF] %s took %u cycles\n", operation, (uint32_t)cycles);
}

// ============================================================================
// Softmax 计算函数
// ============================================================================

/**
 * @brief 计算浮点数的指数函数（简化版本）
 * @param x 输入值
 * @return e^x 的近似值
 */
static float exp_approx(float x) {
    // 使用泰勒级数展开的简化版本
    // 注意：这是一个简化实现，实际应用中可能需要更精确的算法
    if (x > 10.0f) return 100000.0f;  // 防止溢出
    if (x < -10.0f) return 0.00001f;  // 防止下溢
    
    float result = 1.0f;
    float term = 1.0f;
    
    // 使用前5项泰勒级数展开
    for (int i = 1; i <= 5; i++) {
        term *= x / i;
        result += term;
    }
    
    return result;
}

/**
 * @brief 计算softmax函数
 * @param input 输入数组指针
 * @param output 输出数组指针
 * @param size 数组大小
 * @return 成功返回0，失败返回错误码
 */
int softmax_compute(const float* input, float* output, uint32_t size) {
    if (input == 0 || output == 0 || size == 0) {
        return MAILBOX_ERR_INVALID_PARAM;
    }
    
    // 查找最大值（用于数值稳定性）
    float max_val = input[0];
    for (uint32_t i = 1; i < size; i++) {
        if (input[i] > max_val) {
            max_val = input[i];
        }
    }
    
    // 计算指数和
    float sum = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        float exp_val = exp_approx(input[i] - max_val);  // 减去最大值提高数值稳定性
        output[i] = exp_val;
        sum += exp_val;
    }
    
    // 归一化
    if (sum == 0.0f) {
        return MAILBOX_ERR_INVALID_PARAM;  // 防止除以零
    }
    
    for (uint32_t i = 0; i < size; i++) {
        output[i] /= sum;
    }
    
    return MAILBOX_SUCCESS;
}

/**
 * @brief 打印浮点数组
 * @param name 数组名称
 * @param array 数组指针
 * @param size 数组大小
 */
void print_float_array(const char* name, const float* array, uint32_t size) {
    printf("[%s] ", name);
    
    for (uint32_t i = 0; i < size; i++) {
        // 简化打印：只打印整数部分
        int int_part = (int)array[i];
        printf("%d", int_part);
        
        if (i < size - 1) {
            printf(", ");
        }
    }
    printf("\n");
}

/**
 * @brief 验证softmax结果
 * @param output softmax输出数组
 * @param size 数组大小
 * @return 验证通过返回0，否则返回错误码
 */
int verify_softmax_result(const float* output, uint32_t size) {
    if (output == 0 || size == 0) {
        return MAILBOX_ERR_INVALID_PARAM;
    }
    
    // 检查所有值是否在[0,1]范围内
    for (uint32_t i = 0; i < size; i++) {
        if (output[i] < 0.0f || output[i] > 1.0f) {
            printf("[ERROR] Softmax output out of range: %u = %d%%\n", i, (int)(output[i] * 100));
            return MAILBOX_ERR_INVALID_PARAM;
        }
    }
    
    // 检查总和是否接近1.0
    float sum = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        sum += output[i];
    }
    
    if (sum < 0.99f || sum > 1.01f) {
        printf("[ERROR] Softmax sum not equal to 1.0: %d%%\n", (int)(sum * 100));
        return MAILBOX_ERR_INVALID_PARAM;
    }
    
    return MAILBOX_SUCCESS;
}