ifeq ($(RISCV_PATH),)
$(error RISCV_PATH is not set)
endif
LD_LIB_DIR = $(RISCV_PATH)/lib

O ?= .

CC = $(RISCV_PATH)/bin/riscv64-unknown-elf-gcc
OC = $(RISCV_PATH)/bin/riscv64-unknown-elf-objcopy
OD = $(RISCV_PATH)/bin/riscv64-unknown-elf-objdump
AS = $(RISCV_PATH)/bin/riscv64-unknown-elf-as

# C compilation directives
CFLAGS += -c
CFLAGS += -std=c11
CFLAGS += -Wall
CFLAGS += -Wno-unused-label
CFLAGS += -Os
CFLAGS += -march=rv64imafdcv_zvl512b
CFLAGS += --no-integrated-cpp
CFLAGS += -mcmodel=medany

# Assembler
ASFLAGS += -c
ASFLAGS += -std=c11
ASFLAGS += -Os
ASFLAGS += -Wall
ASFLAGS += -march=rv64imafdcv_zvl512b
ASFLAGS += -mcmodel=medany

# Linker directives.
LFLAGS += -Wall
LFLAGS += -Os
LFLAGS += -Wl,--no-relax
LFLAGS += -nostdlib
LFLAGS += -lgcc -lc
LFLAGS += -Tscript.ld
LFLAGS += -mcmodel=medany

INCLUDE = -I.

C_SRC = ./main.c
S_SRC = ./start.S

OBJS += $(O)/$(S_SRC:.S=.o)
OBJS += $(O)/$(C_SRC:.c=.o)

$(O)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

$(O)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) -x assembler-with-cpp $(ASFLAGS) $(INCLUDE) $< -o $@

$(O)/%.elf: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $^ $(LFLAGS) -o $@

$(O)/%.lst: $(O)/%.elf
	@mkdir -p $(dir $@)
	$(OD) --source --all-headers --demangle --line-numbers --wide $< > $@

.PHONY: all
all: $(O)/main.lst $(O)/main.elf

.PHONY: clean
clean:
	rm -f $(OBJS) $(O)/*.o $(O)/*.elf $(O)/*.lst

