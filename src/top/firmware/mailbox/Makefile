# Makefile for RISC-V firmware

# 工具链配置
RISCV_PREFIX ?= riscv64-unknown-elf-
CC = $(RISCV_PREFIX)gcc
LD = $(RISCV_PREFIX)ld
OBJCOPY = $(RISCV_PREFIX)objcopy
OBJDUMP = $(RISCV_PREFIX)objdump
SIZE = $(RISCV_PREFIX)size

# 项目根目录（支持从不同目录运行）
PROJECT_ROOT ?= ..
FIRMWARE_DIR = $(PROJECT_ROOT)/mailbox

# 编译选项
ARCH = rv64imafdc
ABI = lp64d
CFLAGS = -march=$(ARCH) -mabi=$(ABI) -O2 -g -Wall -Wextra
CFLAGS += -ffreestanding -nostdlib -fno-builtin
CFLAGS += -I$(FIRMWARE_DIR)/include -I$(FIRMWARE_DIR)/src -I../util

# 链接选项
LDFLAGS = -T $(FIRMWARE_DIR)/linker/firmware.ld -nostdlib -static
LDFLAGS += --gc-sections --print-memory-usage

# 源文件和目标文件
SRC_DIR = $(FIRMWARE_DIR)/src
INC_DIR = $(FIRMWARE_DIR)/include
LD_DIR = $(FIRMWARE_DIR)/linker
BUILD_DIR ?= build

FIRMWARE_SRCS = $(wildcard $(SRC_DIR)/*.c)
UTIL_SRCS = ../util/printf.c
SRCS = $(FIRMWARE_SRCS) $(UTIL_SRCS)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(FIRMWARE_SRCS)) $(BUILD_DIR)/printf.o  # 添加对应的obj

# 目标文件
TARGET = firmware
ELF = $(BUILD_DIR)/$(TARGET).elf
BIN = $(BUILD_DIR)/$(TARGET).bin
HEX = $(BUILD_DIR)/$(TARGET).hex
DUMP = $(BUILD_DIR)/$(TARGET).dump

# 默认目标
all: $(BUILD_DIR) $(ELF) $(BIN) $(HEX) $(DUMP) size

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译 C 文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 编译 util/printf.c
$(BUILD_DIR)/printf.o: ../util/printf.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 链接 ELF 文件
$(ELF): $(OBJS) $(LD_DIR)/firmware.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# 生成二进制文件
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# 生成十六进制文件（用于 Spike）
$(HEX): $(ELF)
	$(OBJCOPY) -O verilog $< $@

# 生成反汇编文件
$(DUMP): $(ELF)
	$(OBJDUMP) -D $< > $@

# 显示大小信息
size: $(ELF)
	@echo "=== Firmware Size Information ==="
	$(SIZE) $<
	@echo "================================="

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 重新构建
rebuild: clean all

# 安装到 Spike 目录
SPIKE_FIRMWARE_DIR ?= ../riscv-isa-sim/debug_rom
install: $(HEX)
	cp $(HEX) $(SPIKE_FIRMWARE_DIR)/firmware.hex
	@echo "Firmware installed to $(SPIKE_FIRMWARE_DIR)/firmware.hex"

# 调试信息
debug: $(DUMP)
	@echo "=== Firmware Disassembly ==="
	head -100 $(DUMP)
	@echo "... (see $(DUMP) for full disassembly)"

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all       - Build firmware (default)"
	@echo "  clean     - Remove build files"
	@echo "  rebuild   - Clean and rebuild"
	@echo "  install   - Install firmware to Spike directory"
	@echo "  size      - Show firmware size"
	@echo "  debug     - Show disassembly (first 100 lines)"
	@echo "  help      - Show this help message"

.PHONY: all clean rebuild install size debug help
