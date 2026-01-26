#!/bin/bash

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
ZMQ_SERVER="$BUILD_DIR/zmq/spike_csr_server"
FIRMWARE_ELF="$BUILD_DIR/csr/firmware/firmware.elf"
LOG_DIR="$PROJECT_ROOT/log"

# 创建日志目录
mkdir -p "$LOG_DIR"

# 设置库路径
export LD_LIBRARY_PATH=$PROJECT_ROOT/riscv-isa-sim/install/lib:$LD_LIBRARY_PATH

pkill -f "spike_csr_server" 2>/dev/null || true
sleep 2

# 检查文件是否存在
if [ ! -f "$ZMQ_SERVER" ]; then
    echo "ERROR: ZMQ server not found at $ZMQ_SERVER"
    echo "Please build first: bash build_all.sh -t zmq"
    exit 1
fi

if [ ! -f "$FIRMWARE_ELF" ]; then
    echo "ERROR: CSR firmware not found at $FIRMWARE_ELF"
    echo "Please build first: bash build_all.sh -f csr"
    exit 1
fi

echo "========================================"
echo "ZMQ Server"
echo "========================================"
echo "Server: $ZMQ_SERVER"
echo "Firmware: $FIRMWARE_ELF"
echo "Endpoint: tcp://*:5555"
echo "========================================"
echo ""

# 启动服务器
cd "$PROJECT_ROOT"
exec "$ZMQ_SERVER" "$FIRMWARE_ELF" "tcp://*:5555" 2>&1 | tee "$LOG_DIR/zmq_server_manual.log"
