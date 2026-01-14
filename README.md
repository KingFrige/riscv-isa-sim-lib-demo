# RISC-V ISA Simulator as a Library Demo

This project demonstrates how to use [RISC-V ISA Simulator](https://github.com/riscv-software-src/riscv-isa-sim) as a library, and integrate it with an external simulator environment. 
We currently have four main use-cases: 
1. connecting with C++ memory simulator
2. having a SystemC wrapper and embedding it within a SystemC environment
3. statically linking Spike with custom extensions (including AI instruction extensions)
4. mailbox communication framework for host-firmware interaction

Each use-case has its own folder in which you can find separate `README.md` file with more details.
1. `src/cpp` - C++ memory simulator
2. `src/systemc` - SystemC wrapper
3. `src/top` - Statically linked Spike with custom extensions and AI instructions
4. `src/xperimental` - Custom extension examples

## require


## init repo

```bash
git clone -b dev git@github.com:KingFrige/riscv-isa-sim-lib-demo.git

cd riscv-isa-sim-lib-demo
git submodule update --init --recursive
```

## run

```bash
./build_all.sh -r
```

## mailbox functionality

The project also includes a mailbox communication framework that allows host programs to communicate with firmware running in the Spike simulator:

```bash
# Build mailbox firmware
./build_all.sh --mailbox-firmware

# Build spike mailbox wrapper
./build_all.sh --spike-mailbox

# Build all components including mailbox
./build_all.sh --all
```

For more details about the mailbox functionality, see `src/top/spike_mailbox.README.md`.
