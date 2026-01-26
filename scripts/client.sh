#!/bin/bash

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
ZMQ_CLIENT="$BUILD_DIR/zmq/spike_csr_client"
LOG_DIR="$PROJECT_ROOT/log"

# 创建日志目录
mkdir -p "$LOG_DIR"

# 检查文件是否存在
if [ ! -f "$ZMQ_CLIENT" ]; then
    echo "ERROR: ZMQ client not found at $ZMQ_CLIENT"
    echo "Please build first: bash build_all.sh -t zmq"
    exit 1
fi

echo "========================================"
echo "ZMQ Client"
echo "========================================"
echo "Client: $ZMQ_CLIENT"
echo "Endpoint: tcp://localhost:5555"
echo "========================================"
echo ""

# 运行客户端
cd "$PROJECT_ROOT"
exec "$ZMQ_CLIENT" all 2>&1 | tee "$LOG_DIR/zmq_client_manual.log"

sleep 5
exec "$ZMQ_CLIENT" quit >> "$LOG_DIR/zmq_client_manual.log"

sleep 2
pkill -f "spike_csr_server" 2>/dev/null || true
