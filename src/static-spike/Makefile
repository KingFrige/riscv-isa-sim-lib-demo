# Static Spike Integration Makefile
# Builds a statically linked Spike executable with custom extensions

# Root directory of this project
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PROJECT_ROOT := $(realpath $(ROOT_DIR)/../..)

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
EXTENSIONS_SRC = $(wildcard $(EXTENSIONS_DIR)/*.cc) $(wildcard $(EXTENSIONS_DIR)/*.cpp)
EXTENSIONS_OBJ = $(patsubst $(EXTENSIONS_DIR)/%.cc,$(EXTENSIONS_DIR)/%.o,$(filter %.cc,$(EXTENSIONS_SRC))) \
                 $(patsubst $(EXTENSIONS_DIR)/%.cpp,$(EXTENSIONS_DIR)/%.o,$(filter %.cpp,$(EXTENSIONS_SRC)))
EXTENSIONS_DEP = $(EXTENSIONS_OBJ:.o=.d)

# RISC-V toolchain path (can be overridden)
RISCV_PATH ?= /home/korben/tools/riscv/master-v20251230

# Source files for our custom spike
CUSTOM_SPIKE_SRC = $(ROOT_DIR)/spike_main.cc
CUSTOM_SPIKE_OBJ = $(CUSTOM_SPIKE_SRC:.cc=.o)
CUSTOM_SPIKE_DEP = $(CUSTOM_SPIKE_OBJ:.o=.d)

# Target executable
TARGET = spike-static

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
	-I$(EXTENSIONS_DIR)

# Linker flags
LDFLAGS = -Wl,-rpath,$(SPIKE_LIB_DIR) -Wl,--no-as-needed
LDLIBS = -lriscv -lsoftfloat -ldisasm -lfesvr -ldl -lpthread

# Default target
all: $(TARGET)
	make -C firmware/

# Include dependencies
-include $(EXTENSIONS_DEP)
-include $(CUSTOM_SPIKE_DEP)

# Build extensions (.cc files)
$(EXTENSIONS_DIR)/%.o: $(EXTENSIONS_DIR)/%.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build extensions (.cpp files)
$(EXTENSIONS_DIR)/%.o: $(EXTENSIONS_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build custom spike main
$(ROOT_DIR)/%.o: $(ROOT_DIR)/%.cc
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
TEST_SW_DIR = $(ROOT_DIR)/firmware
TEST_ELF = $(TEST_SW_DIR)/main.elf

.PHONY: build_test
build_test:
	cd $(TEST_SW_DIR) && RISCV_PATH=$(RISCV_PATH) make

.PHONY: run
run: $(TEST_ELF)
	./spike-static --isa=rv64imafdcv_zicsr_xperia_xperiv_xperib_xperivmul -l --log=log.txt --log-commits --instructions=2000 $(TEST_ELF)

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
	$(RM) $(EXTENSIONS_OBJ) $(EXTENSIONS_DEP) $(CUSTOM_SPIKE_OBJ) $(CUSTOM_SPIKE_DEP) $(TARGET)

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
