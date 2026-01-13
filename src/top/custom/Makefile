# 3DDRAM C/C++ Models Makefile

CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O2 -std=c11 -I. -I$(GEMM_DIR) -I$(RISCV_DIR) -fPIC -DBATCH_SIZE=1
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 -I. -I$(GEMM_DIR) -I$(RISCV_DIR) -fPIC -DBATCH_SIZE=1
LDFLAGS = -lm

BUILD_DIR = build
LOG_DIR = log
GEMM_DIR = gemm
RISCV_DIR = riscv

GEMM_SRCS = $(GEMM_DIR)/MxFp8ActDeNorm.cpp $(GEMM_DIR)/MxFp8Product.cpp \
            $(GEMM_DIR)/Fp32PsumAcc.cpp $(GEMM_DIR)/GemmTop.cpp
GEMM_OBJS = $(addprefix $(BUILD_DIR)/, $(GEMM_SRCS:.cpp=.o))

RISCV_SRCS = $(RISCV_DIR)/MxFp8ActQuant.cpp
RISCV_OBJS = $(addprefix $(BUILD_DIR)/, $(RISCV_SRCS:.cpp=.o))

EXPP_SRCS = $(RISCV_DIR)/BF16.cpp $(RISCV_DIR)/custom_expp.cpp
EXPP_OBJS = $(addprefix $(BUILD_DIR)/, $(EXPP_SRCS:.cpp=.o))

SOFTMAX_SRCS = $(RISCV_DIR)/BF16.cpp $(RISCV_DIR)/SoftmaxCore.cpp $(RISCV_DIR)/custom_expp.cpp
SOFTMAX_OBJS = $(addprefix $(BUILD_DIR)/, $(SOFTMAX_SRCS:.cpp=.o))

TEST_GEMM = $(BUILD_DIR)/test_gemm_4k
TEST_QUANT = $(BUILD_DIR)/test_quant
TEST_EXPP = $(BUILD_DIR)/test_custom_expp
SINGLE_EXPP = $(BUILD_DIR)/single_expp
TEST_SOFTMAX = $(BUILD_DIR)/test_softmax

.PHONY: all gemm quant expp single softmax run clean help

all: $(TEST_GEMM) $(TEST_QUANT) $(TEST_EXPP) $(SINGLE_EXPP) $(TEST_SOFTMAX)
	@echo "All targets built successfully."

gemm: $(TEST_GEMM)
	@./$(TEST_GEMM)

quant: $(TEST_QUANT)
	@./$(TEST_QUANT)
	@bash ./script/filter_errors.sh "" 13 all

expp: $(TEST_EXPP)
	@./$(TEST_EXPP)

single: $(SINGLE_EXPP)
	@./$(SINGLE_EXPP) $(ARGS)

softmax: $(TEST_SOFTMAX)
	@echo "Running softmax tests for different ROW_LEN values (32, 64, 128, 256, 512)..."
	@echo "Running with ROW_LEN=32 (m1)..."
	@$(TEST_SOFTMAX) 32
	@echo "Running with ROW_LEN=64 (m2)..."
	@$(TEST_SOFTMAX) 64
	@echo "Running with ROW_LEN=128 (m4)..."
	@$(TEST_SOFTMAX) 128
	@echo "Running with ROW_LEN=256 (m8)..."
	@$(TEST_SOFTMAX) 256
	@echo "Running with ROW_LEN=512 (m16)..."
	@$(TEST_SOFTMAX) 512
	@echo "All softmax tests completed."

softmax_all: $(TEST_SOFTMAX)
	@echo "Running softmax tests for different ROW_LEN values..."
	@echo "Running with ROW_LEN=32 (m1)..."
	@$(TEST_SOFTMAX) 32
	@echo "Running with ROW_LEN=64 (m2)..."
	@$(TEST_SOFTMAX) 64
	@echo "Running with ROW_LEN=128 (m4)..."
	@$(TEST_SOFTMAX) 128
	@echo "Running with ROW_LEN=256 (m8)..."
	@$(TEST_SOFTMAX) 256
	@echo "Running with ROW_LEN=512 (m16)..."
	@$(TEST_SOFTMAX) 512
	@echo "All softmax tests completed."

$(TEST_GEMM): $(BUILD_DIR)/util.o $(GEMM_OBJS) test/main_gemm.cpp | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_QUANT): $(BUILD_DIR)/util.o $(BUILD_DIR)/riscv/BF16.o $(BUILD_DIR)/riscv/custom_expp.o $(RISCV_OBJS) test/main_quant.cpp | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_EXPP): $(BUILD_DIR)/util.o $(EXPP_OBJS) test/main_expp.cpp | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(SINGLE_EXPP): $(BUILD_DIR)/util.o $(BUILD_DIR)/riscv/BF16.o $(BUILD_DIR)/riscv/custom_expp.o test/single_expp.cpp | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_SOFTMAX): $(BUILD_DIR)/util.o $(SOFTMAX_OBJS) test/main_softmax.cpp | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Include dependency files
-include $(GEMM_OBJS:.o=.d)
-include $(RISCV_OBJS:.o=.d)
-include $(EXPP_OBJS:.o=.d)
-include $(SOFTMAX_OBJS:.o=.d)
-include $(BUILD_DIR)/util.d
-include $(BUILD_DIR)/riscv/BF16.d
-include $(BUILD_DIR)/riscv/custom_expp.d

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(LOG_DIR):
	@mkdir -p $(LOG_DIR)

run: $(LOG_DIR) $(TEST_GEMM) expp single softmax quant
	@./$(TEST_GEMM)

extract_riscv_arrays: run
	cd log && python3 ../script/extract_riscv_insn_ref_arrays.py > riscv_insn_array.log

clean:
	@rm -rf $(BUILD_DIR) $(LOG_DIR) log/* *.log *.o *.out *.d

help:
	@echo "3DDRAM C Models - Targets:"
	@echo "  make / make gemm  - Build and run GEMM test"
	@echo "  make quant        - Build and run MxFP8 quantization test"
	@echo "  make expp         - Build and run BF16 e^x approximation test"
	@echo "  make single       - Build and run single value expp test (use ARGS=0x8000)"
	@echo "  make softmax      - Build and run SoftmaxCore golden model test"
	@echo "  make clean        - Remove build artifacts"
