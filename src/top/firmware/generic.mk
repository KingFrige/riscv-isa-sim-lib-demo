# Generic Makefile for RISC-V firmware
# 使用方法：在子目录的 Makefile 中定义 ARCH, FIRMWARE_SRCS 等，然后包含此文件

# 工具链配置（如果在包含此文件前未定义，则使用默认值）
RISCV_PREFIX ?= riscv64-unknown-elf-
CC = $(RISCV_PREFIX)gcc
OBJCOPY = $(RISCV_PREFIX)objcopy
OBJDUMP = $(RISCV_PREFIX)objdump
SIZE = $(RISCV_PREFIX)size
AS = $(RISCV_PREFIX)as

# 架构配置（必须在包含此文件前定义）
ARCH ?= rv64gcv_zvl512b
ABI ?= lp64d

# 编译选项（可在包含此文件前扩展）
CFLAGS ?= -march=$(ARCH) -mabi=$(ABI) -O2 -Wall -Wextra
CFLAGS += -ffreestanding -nostdlib -fno-builtin
CFLAGS += -I../common
CFLAGS += -mcmodel=medany
CFLAGS += -fdata-sections -ffunction-sections

# 汇编选项
ASFLAGS = -march=$(ARCH) -mcmodel=medany

# 链接选项
LDFLAGS = -T ../common/memmap.ld -nostdlib -static

# 目录配置
COMMON_DIR = ../common
BUILD_DIR ?= build

# 源文件配置（必须在包含此文件前定义 FIRMWARE_SRCS）
FIRMWARE_SRCS ?= $(wildcard *.c)
UTIL_SRCS = $(COMMON_DIR)/printf.c $(COMMON_DIR)/mailbox.c
SRCS = $(FIRMWARE_SRCS) $(UTIL_SRCS)

# 目标文件
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(FIRMWARE_SRCS))
OBJS += $(BUILD_DIR)/printf.o
OBJS += $(BUILD_DIR)/start.o
OBJS += $(BUILD_DIR)/mailbox.o

# 构建目标
ELF = $(BUILD_DIR)/firmware.elf
BIN = $(BUILD_DIR)/firmware.bin
HEX = $(BUILD_DIR)/firmware.hex
DUMP = $(BUILD_DIR)/firmware.dump

# 默认目标
all: $(BUILD_DIR) $(ELF) $(BIN) $(HEX) $(DUMP) size

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译 C 文件
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 编译 common/printf.c
$(BUILD_DIR)/printf.o: $(COMMON_DIR)/printf.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译 common/mailbox.c
$(BUILD_DIR)/mailbox.o: $(COMMON_DIR)/mailbox.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译 common/start.S
$(BUILD_DIR)/start.o: $(COMMON_DIR)/start.S
	$(CC) $(ASFLAGS) -c $< -o $@

# 链接
$(ELF): $(OBJS)
	$(CC) -Wl,--gc-sections $(OBJS) $(LDFLAGS) -o $@

# 生成二进制
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# 生成十六进制
$(HEX): $(ELF)
	$(OBJCOPY) -O verilog $< $@

# 生成反汇编
$(DUMP): $(ELF)
	$(OBJDUMP) -D $< > $@

# 显示大小
size: $(ELF)
	@echo "=== Firmware Size Information ==="
	$(SIZE) $<
	@echo "================================="

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 重新构建
rebuild: clean all

.PHONY: all clean rebuild size
