#!/bin/bash

# Spike Wrapper Build Script
# Builds the complete system: Spike modifications, firmware, top wrapper, and tests

set -e

module load riscv-toolchain/master-v20251230
RISCV_TOOLCHAIN=${RISCV}


# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/build/spike-install"  # Changed to avoid conflicts

# Default paths (override with environment variables)
RISCV_TOOLCHAIN="${RISCV_TOOLCHAIN:-/opt/riscv}"
SPIKE_SRC_DIR="${SPIKE_SRC_DIR:-$PROJECT_ROOT/riscv-isa-sim}"
SPIKE_BUILD_DIR="${SPIKE_BUILD_DIR:-$BUILD_DIR/spike}"
FIRMWARE_BUILD_DIR="${FIRMWARE_BUILD_DIR:-$BUILD_DIR/firmware}"
TOP_BUILD_DIR="${TOP_BUILD_DIR:-$BUILD_DIR/top}"
MAILBOX_BUILD_DIR="${MAILBOX_BUILD_DIR:-$BUILD_DIR/mailbox}"
TESTS_BUILD_DIR="${TESTS_BUILD_DIR:-$BUILD_DIR/tests}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check RISC-V toolchain
    if [ ! -d "$RISCV_TOOLCHAIN" ]; then
        log_error "RISC-V toolchain not found at $RISCV_TOOLCHAIN"
        log_error "Please set RISCV_TOOLCHAIN environment variable"
        return 1
    fi
    
    # Check Spike source
    if [ ! -d "$SPIKE_SRC_DIR" ]; then
        log_error "Spike source not found at $SPIKE_SRC_DIR"
        log_error "Please set SPIKE_SRC_DIR environment variable"
        return 1
    fi
    
    # Check required tools
    local missing_tools=()
    for tool in make cmake gcc g++ python3; do
        if ! command -v $tool >/dev/null 2>&1; then
            missing_tools+=($tool)
        fi
    done
    
    if [ ${#missing_tools[@]} -gt 0 ]; then
        log_error "Missing required tools: ${missing_tools[*]}"
        return 1
    fi
    
    log_success "All prerequisites satisfied"
    return 0
}

# Build Spike with mailbox support
build_spike() {
    log_info "Building Spike with mailbox support..."
    
    # Create build directory
    mkdir -p "$SPIKE_BUILD_DIR"
    cd "$SPIKE_BUILD_DIR"
    
    # Configure
    log_info "Configuring Spike..."
    "$SPIKE_SRC_DIR/configure" \
        --prefix="$INSTALL_DIR" \
        --enable-commitlog \
        --with-boost \
        --with-boost-asio \
        --with-boost-regex \
        CXXFLAGS="-O2 -g" \
        CFLAGS="-O2 -g"
    
    # Build
    log_info "Building Spike..."
    make -j$(nproc)
    
    # Install
    log_info "Installing Spike..."
    make install
    
    # Also install libspike_main.a which is needed by the top wrapper
    log_info "Installing libspike_main.a..."
    if [ -f "libspike_main.a" ]; then
        cp libspike_main.a "$INSTALL_DIR/lib/"
        log_success "libspike_main.a installed"
    else
        log_warning "libspike_main.a not found in build directory"
    fi
    
    cd "$PROJECT_ROOT"
    log_success "Spike built successfully"
}

# Build firmware
build_firmware() {
    log_info "Building firmware..."
    
    # Create build directory
    mkdir -p "$FIRMWARE_BUILD_DIR"
    
    # Change to top firmware directory and build
    cd "$PROJECT_ROOT/src/top/firmware/insn"
    
    # Set required environment variable
    export RISCV_PATH="$RISCV_TOOLCHAIN"
    
    # Build the firmware - Makefile will output to BUILD_DIR automatically
    make RISCV_PATH="$RISCV_TOOLCHAIN" BUILD_DIR="$BUILD_DIR/firmware/insn"

    # Verify firmware was built
    if [ -f "$BUILD_DIR/firmware/insn/firmware.elf" ]; then
        log_success "Firmware built to $BUILD_DIR/firmware/insn/firmware.elf"
    else
        log_error "Firmware not found at $BUILD_DIR/firmware/insn/firmware.elf"
        return 1
    fi
    
    # Return to project root
    cd "$PROJECT_ROOT"
    log_success "Firmware built successfully"
}

# Build mailbox firmware
build_mailbox_firmware() {
    log_info "Building mailbox firmware..."
    
    # Change to mailbox firmware directory and build
    cd "$PROJECT_ROOT/src/top/firmware/mailbox"
    
    # Set required environment variable
    export RISCV_PATH="$RISCV_TOOLCHAIN"
    
    # Build the firmware - Makefile will output to BUILD_DIR automatically
    make RISCV_PATH="$RISCV_TOOLCHAIN" BUILD_DIR="$BUILD_DIR/firmware/mailbox"

    # Verify firmware was built
    if [ -f "$BUILD_DIR/firmware/mailbox/firmware.elf" ]; then
        log_success "Mailbox firmware built to $BUILD_DIR/firmware/mailbox/firmware.elf"
    else
        log_error "Mailbox firmware not found at $BUILD_DIR/firmware/mailbox/firmware.elf"
        return 1
    fi
    
    # Return to project root
    cd "$PROJECT_ROOT"
    log_success "Mailbox firmware built successfully"
}

# Build top wrapper
build_top_wrapper() {
    log_info "Building top wrapper..."
    
    # Create build directory
    mkdir -p "$TOP_BUILD_DIR"
    
    # Set environment variables for the build
    # Use the existing Spike installation that's already in riscv-isa-sim/install
    export SPIKE_INSTALL_DIR="$PROJECT_ROOT/riscv-isa-sim/install"
    export SPIKE_SOURCE_DIR="$SPIKE_SRC_DIR"
    export RISCV_PATH="$RISCV_TOOLCHAIN"
    
    # Change to top directory and build the wrapper
    cd "$PROJECT_ROOT/src/top"
    
    # Define the proper build directory (in the project root)
    TOP_BUILD_DIR="$PROJECT_ROOT/build/top"
    
    # Check if Spike is already built by checking for required libraries
    if [ ! -f "$PROJECT_ROOT/riscv-isa-sim/install/lib/libriscv.so" ]; then
        log_info "Required Spike libraries not found, building Spike libraries first..."
        make spike_build BUILD_DIR="$TOP_BUILD_DIR"
    else
        log_info "Using existing Spike installation at $PROJECT_ROOT/riscv-isa-sim/install"
    fi
    
    log_info "Building top wrapper executable..."
    make BUILD_DIR="$TOP_BUILD_DIR"
    
    # Copy the built executable to the project build directory
    mkdir -p "$BUILD_DIR"
    if [ -f "$TOP_BUILD_DIR/top-main" ]; then
        cp "$TOP_BUILD_DIR/top-main" "$BUILD_DIR/"
        log_success "Top wrapper executable copied to $BUILD_DIR/"
    else
        log_error "Top wrapper executable not found"
        return 1
    fi
    
    # Copy firmware if built
    if [ -f "$TOP_BUILD_DIR/firmware/insn/firmware.elf" ]; then
        mkdir -p "$BUILD_DIR/firmware"
        cp "$TOP_BUILD_DIR/firmware/insn/firmware.elf" "$BUILD_DIR/firmware/"
        log_success "Firmware copied to $BUILD_DIR/firmware/"
    fi
    
    # Return to project root
    cd "$PROJECT_ROOT"
    log_success "Top wrapper built successfully"
}

# Run tests
run_tests() {
    log_info "Running tests..."
    
    # Set environment variables
    export SPIKE_INSTALL_DIR="$PROJECT_ROOT/riscv-isa-sim/install"
    export SPIKE_SOURCE_DIR="$SPIKE_SRC_DIR"
    export RISCV_PATH="$RISCV_TOOLCHAIN"
    
    # Ensure the executables we need are in the build directory
    if [ ! -f "$BUILD_DIR/top-main" ]; then
        log_error "top-main not found in $BUILD_DIR. Please build the top wrapper first."
        return 1
    fi
    
    if [ ! -f "$BUILD_DIR/firmware/insn/firmware.elf" ]; then
        log_error "firmware.elf not found in $BUILD_DIR/firmware/insn. Please build the firmware first."
        return 1
    fi
    
    # Run the test using the Makefile target which ensures consistent execution
    log_info "Running tests using files from $BUILD_DIR..."
    cd "$PROJECT_ROOT/src/top"
    make run-build BUILD_DIR="$PROJECT_ROOT/build"
    log_success "Tests completed and log saved to $BUILD_DIR/spike.log"
}

run_mailbox_tests() {
    log_info "Running mailbox tests..."

    # Set environment variables
    export SPIKE_INSTALL_DIR="$PROJECT_ROOT/riscv-isa-sim/install"
    export SPIKE_SOURCE_DIR="$SPIKE_SRC_DIR"
    export RISCV_PATH="$RISCV_TOOLCHAIN"

    # Run mailbox test using Makefile
    cd "$PROJECT_ROOT/src/top"
    make run-mailbox BUILD_DIR="$BUILD_DIR"

    cd "$PROJECT_ROOT"
    log_success "Mailbox tests completed"
}

# Clean build
clean_build() {
    log_info "Cleaning build directories..."
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        log_success "Build directory cleaned"
    fi
    
    if [ -d "$INSTALL_DIR" ]; then
        rm -rf "$INSTALL_DIR"
        log_success "Install directory cleaned"
    fi
}


# Build mailbox firmware
build_mailbox_firmware() {
    log_info "Building mailbox firmware..."
    
    # Create build directory
    mkdir -p "$BUILD_DIR/firmware/mailbox"
    
    # Set required environment variable
    export RISCV_PATH="$RISCV_TOOLCHAIN"
    
    # Change to mailbox firmware directory and build
    cd "$PROJECT_ROOT/src/top/firmware/mailbox"
    
    # Set RISCV_PREFIX for the toolchain
    export RISCV_PREFIX="$RISCV_TOOLCHAIN/bin/riscv64-unknown-elf-"
    
    # Build the mailbox firmware
    # Use the current directory as PROJECT_ROOT and set FIRMWARE_DIR to current directory
    make RISCV_PREFIX="$RISCV_PREFIX" PROJECT_ROOT="." FIRMWARE_DIR="." BUILD_DIR="build"
    
    # Copy the built firmware to the project build directory
    if [ -f "build/firmware.elf" ]; then
        cp "build/firmware.elf" "$BUILD_DIR/firmware/mailbox/"
        log_success "Mailbox firmware copied to $BUILD_DIR/firmware/mailbox/"
    else
        log_error "Mailbox firmware not found"
        return 1
    fi
    
    # Return to project root
    cd "$PROJECT_ROOT"
    log_success "Mailbox firmware built successfully"
}

# Build spike_mailbox wrapper
build_spike_mailbox() {
    log_info "Building spike mailbox wrapper using Makefile..."

    # Set environment variables for the build
    export SPIKE_INSTALL_DIR="$PROJECT_ROOT/riscv-isa-sim/install"
    export SPIKE_SOURCE_DIR="$SPIKE_SRC_DIR"
    export RISCV_PATH="$RISCV_TOOLCHAIN"

    # Build spike_mailbox using Makefile (includes all extensions)
    cd "$PROJECT_ROOT/src/top"
    make build_mailbox BUILD_DIR="$BUILD_DIR"

    # Copy the built executable to the project build directory
    if [ -f "$BUILD_DIR/mailbox/spike_mailbox" ]; then
        log_success "Spike mailbox wrapper built successfully"
    else
        log_error "Spike mailbox wrapper executable not found"
        return 1
    fi

    # Return to project root
    cd "$PROJECT_ROOT"
}

# Show help
show_help() {
    cat << EOF
Spike Wrapper Build Script

Usage: $0 [OPTIONS]

Options:
  -h, --help              Show this help message
  -c, --clean             Clean build directories before building
  -s, --spike             Build only Spike
  -f, --firmware [TYPE]   Build firmware (TYPE: insn|mailbox|all, default: all)
  -t, --top [TYPE]        Build top wrappers (TYPE: insn|mailbox|all, default: all)
  -r, --run-tests [TYPE]  Build and run tests (TYPE: insn|mailbox|all, default: all)
  --all                   Build everything and run tests
  --riscv PATH            Set RISC-V toolchain path (default: /opt/riscv)
  --spike-src PATH        Set Spike source path (default: ./riscv-isa-sim)

Environment variables:
  RISCV_TOOLCHAIN     Path to RISC-V toolchain
  SPIKE_SRC_DIR       Path to Spike source directory
  SPIKE_BUILD_DIR     Path to Spike build directory
  FIRMWARE_BUILD_DIR  Path to firmware build directory
  TOP_BUILD_DIR       Path to top wrapper build directory
  TESTS_BUILD_DIR     Path to tests build directory

Examples:
  $0                      # Build everything
  $0 --clean              # Clean and build everything
  $0 --spike --firmware   # Build only Spike and firmware
  $0 -t                   # Build all top wrappers (top-main + spike_mailbox)
  $0 -t insn              # Build only top-main
  $0 -t mailbox           # Build only spike_mailbox
  $0 -r                   # Build and run all tests (spike_main + mailbox)
  $0 -r insn              # Build and run spike_main tests only
  $0 -r mailbox           # Build and run mailbox tests only
  $0 --all                # Build everything and run tests

  # Custom paths
  RISCV_TOOLCHAIN=~/riscv SPIKE_SRC_DIR=~/spike $0
EOF
}

# Parse command line arguments
parse_args() {
    local clean_build_flag=0
    local build_spike_flag=0
    local build_firmware_flag="all"
    local build_top_flag=0
    local build_spike_mailbox_flag=0
    local run_tests_flag=0
    local run_mailbox_tests_flag=0

    if [ $# -lt 1 ]; then
        run_tests_flag=1
    fi
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                clean_build_flag=1
                shift
                ;;
            -s|--spike)
                build_spike_flag=1
                shift
                ;;
            -f|--firmware)
                if [[ -z "$2" || "$2" == -* ]]; then
                    build_firmware_flag="all"
                else
                    build_firmware_flag="$2"
                    shift
                fi
                shift
                ;;
            -t|--top)
                # Default: build all top wrappers (top-main + spike_mailbox)
                build_top_flag=1
                build_spike_mailbox_flag=1
                # Check for optional type argument (insn|mailbox|all)
                if [[ -n "$2" && "$2" != -* ]]; then
                    case "$2" in
                        insn)
                            build_spike_mailbox_flag=0
                            ;;
                        mailbox)
                            build_top_flag=0
                            ;;
                        all)
                            # Default behavior - both
                            ;;
                    esac
                    shift
                fi
                shift
                ;;
            -r|--run-tests|--run)
                build_spike_flag=1
                build_firmware_flag="all"
                build_top_flag=1
                build_spike_mailbox_flag=1
                run_tests_flag=1
                run_mailbox_tests_flag=1
                # Check for optional type argument (insn|mailbox|all)
                if [[ -n "$2" && "$2" != -* ]]; then
                    case "$2" in
                        insn)
                            run_mailbox_tests_flag=0
                            build_spike_mailbox_flag=0
                            ;;
                        mailbox)
                            run_tests_flag=0
                            ;;
                        all)
                            # Default behavior
                            ;;
                    esac
                    shift
                fi
                shift
                ;;
            --all)
                # Build everything and run tests
                build_spike_flag=1
                build_firmware_flag="all"
                build_top_flag=1
                build_spike_mailbox_flag=1
                run_tests_flag=1
                run_mailbox_tests_flag=1
                shift
                ;;
            --riscv)
                RISCV_TOOLCHAIN="$2"
                shift 2
                ;;
            --spike-src)
                SPIKE_SRC_DIR="$2"
                shift 2
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Apply clean if requested
    if [ $clean_build_flag -eq 1 ]; then
        clean_build
    fi
    
    # Create directories
    mkdir -p "$BUILD_DIR" "$INSTALL_DIR"
    
    # Build components as needed
    if [ $build_spike_flag -eq 1 ]; then
        build_spike
    fi
    
    # Build firmware based on type
    case $build_firmware_flag in
        all)
            build_firmware
            build_mailbox_firmware
            ;;
        insn)
            build_firmware
            ;;
        mailbox)
            build_mailbox_firmware
            ;;
    esac
    
    if [ $build_top_flag -eq 1 ]; then
        build_top_wrapper
    fi
    
    if [ $build_spike_mailbox_flag -eq 1 ]; then
        build_spike_mailbox
    fi
    
    # Run tests if requested
    if [ $run_tests_flag -eq 1 ]; then
        run_tests
    fi

    # Run tests if requested
    if [ $run_mailbox_tests_flag -eq 1 ]; then
        run_mailbox_tests
    fi
}

# Main function
main() {
    log_info "Starting Spike Wrapper build..."
    log_info "Project root: $PROJECT_ROOT"
    log_info "Build directory: $BUILD_DIR"
    log_info "Install directory: $INSTALL_DIR"
    log_info "RISC-V toolchain: $RISCV_TOOLCHAIN"
    log_info "Spike source: $SPIKE_SRC_DIR"
    
    # Check prerequisites
    if ! check_prerequisites; then
        exit 1
    fi
    
    # Parse arguments and build
    parse_args "$@"
    
    log_success "Build completed successfully!"
    log_info "Installation directory: $INSTALL_DIR"
}

# Run main function
main "$@"
