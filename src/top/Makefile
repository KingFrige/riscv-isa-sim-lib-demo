# Static Spike Integration Makefile
# Builds a statically linked Spike executable with custom extensions

# Root directory of this project
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PROJECT_ROOT := $(realpath $(ROOT_DIR)/../..)

# Spike directories
SPIKE_SOURCE_DIR ?= $(PROJECT_ROOT)/riscv-isa-sim
SPIKE_DIR = $(patsubst %/,%,$(SPIKE_SOURCE_DIR))
BUILD_DIR = $(ROOT_DIR)/build
SPIKE_BUILD_DIR = $(SPIKE_DIR)/build
SPIKE_INSTALL_DIR = $(SPIKE_DIR)/install
SPIKE_INC_DIR = $(SPIKE_INSTALL_DIR)/include
SPIKE_LIB_DIR = $(SPIKE_INSTALL_DIR)/lib
SPIKE_CONFIGURE_STAMP = $(SPIKE_BUILD_DIR)/.configure_stamp

# Extension directories
EXTENSIONS_DIR = $(ROOT_DIR)/extensions
CUSTOM_RISCV_DIR = $(ROOT_DIR)/custom/riscv
EXTENSIONS_SRC = $(wildcard $(EXTENSIONS_DIR)/*.cc) $(wildcard $(EXTENSIONS_DIR)/*.cpp)
CUSTOM_RISCV_SRC = $(wildcard $(CUSTOM_RISCV_DIR)/*.cc) $(wildcard $(CUSTOM_RISCV_DIR)/*.cpp)
ALL_EXTENSIONS_SRC = $(EXTENSIONS_SRC) $(CUSTOM_RISCV_SRC)
EXTENSIONS_OBJ = $(patsubst $(EXTENSIONS_DIR)/%.cc,$(BUILD_DIR)/extensions/%.o,$(filter %.cc,$(EXTENSIONS_SRC))) \
                 $(patsubst $(EXTENSIONS_DIR)/%.cpp,$(BUILD_DIR)/extensions/%.o,$(filter %.cpp,$(EXTENSIONS_SRC))) \
                 $(patsubst $(CUSTOM_RISCV_DIR)/%.cc,$(BUILD_DIR)/custom/riscv/%.o,$(filter %.cc,$(CUSTOM_RISCV_SRC))) \
                 $(patsubst $(CUSTOM_RISCV_DIR)/%.cpp,$(BUILD_DIR)/custom/riscv/%.o,$(filter %.cpp,$(CUSTOM_RISCV_SRC)))
EXTENSIONS_DEP = $(EXTENSIONS_OBJ:.o=.d)

# RISC-V toolchain path (can be overridden)
RISCV_PATH ?= /home/korben/tools/riscv/master-v20251230

# Source files for our custom spike
CUSTOM_SPIKE_SRC = $(ROOT_DIR)/spike_main.cc
CUSTOM_SPIKE_OBJ = $(BUILD_DIR)/spike_main.o
CUSTOM_SPIKE_DEP = $(CUSTOM_SPIKE_OBJ:.o=.d)

# Target executable
TARGET = $(BUILD_DIR)/top-main

# Compiler flags
CXX ?= g++
CXXFLAGS = -std=c++17 -fPIC -O2 -Wall -MMD -MP -D_GNU_SOURCE
INCLUDES = \
	-I$(SPIKE_INC_DIR)/riscv \
	-I$(SPIKE_INC_DIR)/fesvr \
	-I$(SPIKE_INC_DIR)/disasm \
	-I$(SPIKE_INC_DIR)/softfloat \
	-I$(SPIKE_BUILD_DIR) \
	-I$(SPIKE_DIR)/riscv \
	-I$(SPIKE_DIR) \
	-I$(ROOT_DIR) \
	-I$(EXTENSIONS_DIR) \
	-I$(CUSTOM_RISCV_DIR)

# Linker flags
LDFLAGS = -Wl,-rpath,$(SPIKE_LIB_DIR) -Wl,--no-as-needed
LDLIBS = -lriscv -lsoftfloat -ldisasm -lfesvr -ldl -lpthread

# Default target
all: $(TARGET)
	cd firmware/insn && $(MAKE) O=$(BUILD_DIR)/firmware

# Include dependencies
-include $(EXTENSIONS_DEP)
-include $(CUSTOM_SPIKE_DEP)

# Build extensions (.cc files)
$(BUILD_DIR)/extensions/%.o: $(EXTENSIONS_DIR)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build extensions (.cpp files)
$(BUILD_DIR)/extensions/%.o: $(EXTENSIONS_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build custom RISC-V extensions (.cpp files)
$(BUILD_DIR)/custom/riscv/%.o: $(CUSTOM_RISCV_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build custom spike main
$(BUILD_DIR)/%.o: $(ROOT_DIR)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Link everything together
$(TARGET): $(EXTENSIONS_OBJ) $(CUSTOM_SPIKE_OBJ)
	$(CXX) $(LDFLAGS) -L$(SPIKE_LIB_DIR) -o $@ $^ $(LDLIBS)

# Spike build targets
$(SPIKE_CONFIGURE_STAMP):
	@if [ ! -f "$(SPIKE_BUILD_DIR)/config.status" ]; then \
		mkdir -p $(SPIKE_BUILD_DIR) && \
		cd $(SPIKE_BUILD_DIR) && \
		$(SPIKE_DIR)/configure \
			--prefix=$(SPIKE_INSTALL_DIR) \
			CFLAGS="-fPIC -O2" \
			CXXFLAGS="-fPIC -O2 -std=c++17"; \
	fi
	touch $(SPIKE_CONFIGURE_STAMP)

.PHONY: spike_build
spike_build: $(SPIKE_CONFIGURE_STAMP)
	$(MAKE) -C $(SPIKE_BUILD_DIR)
	$(MAKE) -C $(SPIKE_BUILD_DIR) install

.PHONY: reconfigure_spike
reconfigure_spike:
	$(MAKE) clean_spike
	$(MAKE) $(SPIKE_CONFIGURE_STAMP)

# Test program compilation
TEST_SW_DIR = $(ROOT_DIR)/firmware/insn
TEST_ELF = $(BUILD_DIR)/firmware/insn/main.elf

.PHONY: build_test
build_test: $(TARGET)
	cd $(TEST_SW_DIR) && RISCV_PATH=$(RISCV_PATH) make O=$(BUILD_DIR)/firmware

# Run with build directory paths
.PHONY: run-build
run-build: 
	$(BUILD_DIR)/top-main --isa=rv64imafdcv_zvl512b_zicsr_xperia_xperiv -l --log=$(BUILD_DIR)/spike.log --log-commits --instructions=80000 $(BUILD_DIR)/firmware/insn/main.elf

# Rule to ensure test program is built
$(TEST_ELF): build_test

.PHONY: clean_spike
clean_spike:
	$(RM) -r $(SPIKE_BUILD_DIR) $(SPIKE_INSTALL_DIR)
	$(RM) $(SPIKE_CONFIGURE_STAMP)

.PHONY: clean_test
clean_test:
	cd $(TEST_SW_DIR) && make clean

.PHONY: clean
clean: clean_test
	$(RM) $(EXTENSIONS_OBJ) $(EXTENSIONS_DEP) $(CUSTOM_SPIKE_OBJ) $(CUSTOM_SPIKE_DEP) $(TARGET) spike.log

.PHONY: distclean
distclean: clean clean_spike

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all           - Build static spike with extensions (default)"
	@echo "  spike_build   - Build and install spike libraries"
	@echo "  clean         - Remove built objects and executable"
	@echo "  clean_spike   - Remove spike build and installation"
	@echo "  distclean     - Clean everything"
	@echo "  reconfigure_spike - Reconfigure spike build"
	@echo "  run - spike test"
