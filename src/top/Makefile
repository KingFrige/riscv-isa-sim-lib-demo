# Static Spike Integration Makefile
# Builds statically linked Spike executables with custom extensions

# Root directory of this project
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PROJECT_ROOT := $(realpath $(ROOT_DIR)/../..)

# =============================================================================
# Directory Configuration (Consistent structure: <type>/)
# =============================================================================

# Build output directories (unified: insn/, mailbox/, csr/)
BUILD_DIR ?= $(ROOT_DIR)/build
INSN_BUILD_DIR = $(BUILD_DIR)/insn
MAILBOX_BUILD_DIR = $(BUILD_DIR)/mailbox
CSR_BUILD_DIR = $(BUILD_DIR)/csr

# Spike directories
SPIKE_SOURCE_DIR ?= $(PROJECT_ROOT)/riscv-isa-sim
SPIKE_DIR = $(patsubst %/,%,$(SPIKE_SOURCE_DIR))
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

# =============================================================================
# Source Files for Each Target
# =============================================================================

# Source files for spike_insn (INSN test)
SPIKE_INSN_SRC = $(ROOT_DIR)/spike_insn.cc
SPIKE_INSN_OBJ = $(INSN_BUILD_DIR)/spike_insn.o
SPIKE_INSN_DEP = $(SPIKE_INSN_OBJ:.o=.d)

# Source files for spike_mailbox (MAILBOX test)
SPIKE_MAILBOX_SRC = $(ROOT_DIR)/spike_mailbox.cc
SPIKE_MAILBOX_OBJ = $(MAILBOX_BUILD_DIR)/spike_mailbox.o
SPIKE_MAILBOX_DEP = $(SPIKE_MAILBOX_OBJ:.o=.d)

# Source files for spike_csr (CSR test)
SPIKE_CSR_SRC = $(ROOT_DIR)/spike_csr.cc
SPIKE_CSR_OBJ = $(CSR_BUILD_DIR)/spike_csr.o
SPIKE_CSR_DEP = $(SPIKE_CSR_OBJ:.o=.d)

# =============================================================================
# Target Executables (Consistent: <type>/<name>)
# =============================================================================

INSN_TARGET = $(INSN_BUILD_DIR)/spike_insn
MAILBOX_TARGET = $(MAILBOX_BUILD_DIR)/spike_mailbox
CSR_TARGET = $(CSR_BUILD_DIR)/spike_csr

# =============================================================================
# Compiler Configuration
# =============================================================================

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

# 静态链接 Spike 相关库，动态链接系统库
LDFLAGS = -Wl,--no-as-needed
LDLIBS_STATIC = -Wl,-Bstatic -lriscv -lsoftfloat -ldisasm -lfesvr -lfdt -Wl,-Bdynamic -ldl -lpthread -lboost_regex -lboost_system

# =============================================================================
# Default Target
# =============================================================================

all: build_insn_test

# Include dependencies
-include $(EXTENSIONS_DEP)
-include $(SPIKE_INSN_DEP)
-include $(SPIKE_MAILBOX_DEP)

# =============================================================================
# Common Build Rules
# =============================================================================

$(BUILD_DIR)/extensions/%.o: $(EXTENSIONS_DIR)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/extensions/%.o: $(EXTENSIONS_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/custom/riscv/%.o: $(CUSTOM_RISCV_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(ROOT_DIR)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Specific rules for spike_*.o files to build in their respective directories
$(INSN_BUILD_DIR)/spike_insn.o: $(ROOT_DIR)/spike_insn.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(MAILBOX_BUILD_DIR)/spike_mailbox.o: $(ROOT_DIR)/spike_mailbox.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(CSR_BUILD_DIR)/spike_csr.o: $(ROOT_DIR)/spike_csr.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# =============================================================================
# Spike Library Targets (verb_noun naming)
# =============================================================================

$(SPIKE_CONFIGURE_STAMP):
	@if [ ! -f "$(SPIKE_BUILD_DIR)/config.status" ]; then \
		mkdir -p $(SPIKE_BUILD_DIR) && \
		cd $(SPIKE_BUILD_DIR) && \
		$(SPIKE_DIR)/configure \
			--prefix=$(SPIKE_INSTALL_DIR) \
			--enable-static \
			CFLAGS="-fPIC -O2" \
			CXXFLAGS="-fPIC -O2 -std=c++17"; \
	fi
	touch $(SPIKE_CONFIGURE_STAMP)

.PHONY: build_spike
build_spike: $(SPIKE_CONFIGURE_STAMP)
	$(MAKE) -C $(SPIKE_BUILD_DIR)
	$(MAKE) -C $(SPIKE_BUILD_DIR) install
	@# Copy static libraries if they exist
	@for lib in libriscv.a libsoftfloat.a; do \
		if [ -f "$(SPIKE_BUILD_DIR)/$$lib" ]; then \
			cp "$(SPIKE_BUILD_DIR)/$$lib" "$(SPIKE_LIB_DIR)/" && \
			echo "Copied $$lib to $(SPIKE_LIB_DIR)/"; \
		else \
			echo "Warning: $$lib not found in $(SPIKE_BUILD_DIR)/"; \
		fi \
	done

.PHONY: reconfigure_spike
reconfigure_spike:
	$(MAKE) clean_spike
	$(MAKE) $(SPIKE_CONFIGURE_STAMP)

.PHONY: clean_spike
clean_spike:
	$(RM) -r $(SPIKE_BUILD_DIR) $(SPIKE_INSTALL_DIR)
	$(RM) $(SPIKE_CONFIGURE_STAMP)

# =============================================================================
# INSN Test Targets (insn_ prefix, outputs to build/insn/)
# =============================================================================

INSN_FIRMWARE_DIR = $(ROOT_DIR)/firmware/insn
INSN_FIRMWARE_ELF = $(INSN_BUILD_DIR)/firmware/firmware.elf

# Link spike_insn
$(INSN_TARGET): $(EXTENSIONS_OBJ) $(SPIKE_INSN_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -L$(SPIKE_LIB_DIR) -o $@ $^ $(LDLIBS_STATIC)

# Build insn firmware
.PHONY: build_insn_firmware
build_insn_firmware:
	$(MAKE) -C $(INSN_FIRMWARE_DIR) BUILD_DIR=$(abspath $(INSN_BUILD_DIR)/firmware)

# Build insn test (wrapper + firmware)
.PHONY: build_insn_test
build_insn_test: build_insn_firmware $(INSN_TARGET)

# Run insn test
.PHONY: run_insn_test
run_insn_test: $(INSN_TARGET) $(INSN_FIRMWARE_ELF)
	$(INSN_TARGET) --isa=rv64gcv_zvl512b_zicsr_xperia_xperiv \
		-l --log=$(LOG_DIR)/spike_insn.log --log-commits \
		--instructions=80000 $(INSN_FIRMWARE_ELF)

# Clean insn build
.PHONY: clean_insn_test
clean_insn_test:
	$(MAKE) -C $(INSN_FIRMWARE_DIR) BUILD_DIR=$(abspath $(INSN_BUILD_DIR)/firmware) clean
	$(RM) $(SPIKE_INSN_OBJ) $(SPIKE_INSN_DEP) $(INSN_TARGET)
	$(RM) -rf $(INSN_BUILD_DIR)

# =============================================================================
# MAILBOX Test Targets (mailbox_ prefix, outputs to build/mailbox/)
# =============================================================================

MAILBOX_FIRMWARE_DIR = $(ROOT_DIR)/firmware/mailbox
MAILBOX_FIRMWARE_ELF = $(MAILBOX_BUILD_DIR)/firmware/firmware.elf

CSR_FIRMWARE_DIR = $(ROOT_DIR)/firmware/csr
CSR_FIRMWARE_ELF = $(CSR_BUILD_DIR)/firmware/firmware.elf

# Link spike_mailbox
$(MAILBOX_TARGET): $(EXTENSIONS_OBJ) $(SPIKE_MAILBOX_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -L$(SPIKE_LIB_DIR) -o $@ $^ $(LDLIBS_STATIC)

# Link spike_csr
$(CSR_TARGET): $(EXTENSIONS_OBJ) $(SPIKE_CSR_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -L$(SPIKE_LIB_DIR) -o $@ $^ $(LDLIBS_STATIC)

# Build mailbox firmware
.PHONY: build_mailbox_firmware
build_mailbox_firmware:
	$(MAKE) -C $(MAILBOX_FIRMWARE_DIR) BUILD_DIR=$(abspath $(MAILBOX_BUILD_DIR)/firmware) \
		RISCV_PREFIX="$(RISCV_PATH)/bin/riscv64-unknown-elf-"

# Build CSR firmware
.PHONY: build_csr_firmware
build_csr_firmware:
	$(MAKE) -C $(CSR_FIRMWARE_DIR) BUILD_DIR=$(abspath $(CSR_BUILD_DIR)/firmware) \
		RISCV_PREFIX="$(RISCV_PATH)/bin/riscv64-unknown-elf-"

# Build mailbox test (wrapper + firmware)
.PHONY: build_mailbox_test
build_mailbox_test: build_mailbox_firmware $(MAILBOX_TARGET)

# Build CSR test (wrapper + firmware)
.PHONY: build_csr_test
build_csr_test: build_csr_firmware $(CSR_TARGET)

# Run mailbox test (optional FIRMWARE_ELF to use external firmware)
.PHONY: run_mailbox_test
run_mailbox_test: $(MAILBOX_TARGET)
ifeq ($(FIRMWARE_ELF),)
	$(MAILBOX_TARGET) -l --log=$(LOG_DIR)/spike_mailbox.log --log-commits $(MAILBOX_FIRMWARE_ELF)
else
	$(MAILBOX_TARGET) -l --log=$(LOG_DIR)/spike_fuse.log --log-commits $(FIRMWARE_ELF)
endif

# Run CSR test (optional FIRMWARE_ELF to use external firmware)
.PHONY: run_csr_test
run_csr_test: $(CSR_TARGET)
ifeq ($(FIRMWARE_ELF),)
	$(CSR_TARGET) -l --log=$(LOG_DIR)/spike_csr.log --log-commits $(FIRMWARE_ELF)
else
	$(CSR_TARGET) -l --log=$(LOG_DIR)/spike_csr.log --log-commits $(FIRMWARE_ELF)
endif

# Clean mailbox build
.PHONY: clean_mailbox_test
clean_mailbox_test:
	$(MAKE) -C $(MAILBOX_FIRMWARE_DIR) BUILD_DIR=$(abspath $(MAILBOX_BUILD_DIR)/firmware) clean
	$(RM) $(SPIKE_MAILBOX_OBJ) $(SPIKE_MAILBOX_DEP) $(MAILBOX_TARGET)
	$(RM) -rf $(MAILBOX_BUILD_DIR)

# Clean CSR build
.PHONY: clean_csr_test
clean_csr_test:
	$(MAKE) -C $(CSR_FIRMWARE_DIR) BUILD_DIR=$(abspath $(CSR_BUILD_DIR)/firmware) clean
	$(RM) $(SPIKE_CSR_OBJ) $(SPIKE_CSR_DEP) $(CSR_TARGET)
	$(RM) -rf $(CSR_BUILD_DIR)

# =============================================================================
# Unified Build Targets
# =============================================================================

# Build all test wrappers and firmwares
.PHONY: build_all_tests
build_all_tests: build_insn_test build_mailbox_test build_csr_test

# Run all tests
.PHONY: run_all_tests
run_all_tests: run_insn_test run_mailbox_test

# Clean all test builds
.PHONY: clean_all_tests
clean_all_tests: clean_insn_test clean_mailbox_test

# =============================================================================
# Clean Targets
# =============================================================================

.PHONY: clean
clean: clean_insn_test clean_mailbox_test
	@rm -rf ./build

.PHONY: distclean
distclean: clean clean_spike

# =============================================================================
# Help Target
# =============================================================================

.PHONY: help
help:
	@echo "Available targets:"
	@echo ""
	@echo "=== Spike Library ==="
	@echo "  build_spike        - Build and install spike libraries"
	@echo "  reconfigure_spike  - Reconfigure spike build"
	@echo "  clean_spike        - Remove spike build and installation"
	@echo ""
	@echo "=== INSN Test (insn_*, outputs: build/insn/) ==="
	@echo "  build_insn_firmware - Build insn firmware"
	@echo "  build_insn_test    - Build insn wrapper and firmware"
	@echo "  run_insn_test      - Run insn test"
	@echo "  clean_insn_test    - Clean insn build"
	@echo ""
	@echo "=== MAILBOX Test (mailbox_*, outputs: build/mailbox/) ==="
	@echo "  build_mailbox_firmware - Build mailbox firmware"
	@echo "  build_mailbox_test     - Build mailbox wrapper and firmware"
	@echo "  run_mailbox_test       - Run mailbox test"
	@echo "  clean_mailbox_test     - Clean mailbox build"
	@echo ""
	@echo "=== Unified Targets ==="
	@echo "  build_all_tests    - Build all test wrappers and firmwares"
	@echo "  run_all_tests      - Run all tests"
	@echo "  clean_all_tests    - Clean all test builds"
	@echo ""
	@echo "=== Utility ==="
	@echo "  all                - Build insn test (default)"
	@echo "  clean              - Remove all test builds"
	@echo "  distclean          - Clean everything"
	@echo "  help               - Show this help message"
