# RISC-V Firmware for Spike Mailbox

This firmware provides mailbox communication support for the Spike RISC-V simulator.

## Overview

The firmware implements a mailbox communication mechanism that allows the host to send commands to the Spike simulator and receive responses. It supports basic commands (hello/hi) and vector operations.

## Features

- **Mailbox Communication**: Memory-mapped I/O based communication
- **Command Support**:
  - `HELLO`: Simple greeting command
  - `HI`: Alternative greeting command
  - `VECTOR_LOAD`: Load vector data from memory
  - `VECTOR_STORE`: Store vector data to memory
  - `VECTOR_COMPUTE`: Perform vector computation
- **Error Handling**: Comprehensive error codes and status reporting
- **Debug Output**: UART-based debug messages

## Building

### Prerequisites

- RISC-V toolchain (riscv64-unknown-elf-gcc)
- Make

### Build Steps

```bash
make PROJECT_ROOT="." FIRMWARE_DIR="." BUILD_DIR="build"

# Install to Spike directory
make install
```

### Build Outputs

- `build/firmware.elf`: ELF executable
- `build/firmware.bin`: Raw binary
- `build/mailbox.hex`: Verilog hex format (for Spike)
- `build/firmware.dump`: Disassembly

## Memory Map

### Mailbox Registers (Base: 0x60000000)

| Offset | Register | Width | Description |
|--------|----------|-------|-------------|
| 0x0000 | STATUS | 32-bit | Mailbox status |
| 0x0004 | COMMAND | 32-bit | Command register |
| 0x0008 | DATA_ADDR | 64-bit | Data address pointer |
| 0x0010 | DATA_SIZE | 32-bit | Data size in bytes |
| 0x0014 | RESPONSE | 32-bit | Command response |
| 0x0018 | VECTOR_CONFIG | 64-bit | Vector configuration |

### Status Register Bits

| Bit | Name | Description |
|-----|------|-------------|
| 0 | READY | Mailbox is ready for new command |
| 1 | BUSY | Mailbox is processing command |
| 2 | ERROR | Error occurred during processing |
| 3 | VECTOR_MODE | Vector mode enabled |

## Command Protocol

### Command Execution Flow

1. **Host sends command**:
   - Wait for `STATUS.READY = 1`
   - Write command to `COMMAND` register
   - Write parameters to other registers
   - Write to `STATUS` register to trigger execution

2. **Firmware processes command**:
   - Detects `STATUS.BUSY = 1`
   - Reads command and parameters
   - Executes corresponding function
   - Writes result to `RESPONSE`
   - Clears `BUSY` bit, sets `READY` bit

3. **Host receives response**:
   - Poll `STATUS.BUSY` until `0`
   - Read `RESPONSE` register

### Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00000000 | SUCCESS | Command executed successfully |
| 0x00000001 | INVALID_CMD | Invalid command code |
| 0x00000002 | INVALID_PARAM | Invalid parameter |
| 0x00000003 | MEM_ACCESS | Memory access error |
| 0x00000004 | VECTOR_CONFIG | Vector configuration error |
| 0x00000005 | NOT_IMPLEMENTED | Command not implemented |

## Integration with Spike

### Installation

```bash
# Build and install firmware
make install
```

This will copy `mailbox.hex` to the Spike debug_rom directory.

### Spike Configuration

The firmware is designed to replace Spike's default debug ROM. When Spike starts with this firmware:

1. The firmware initializes and enters main loop
2. It continuously polls the mailbox for commands
3. When a command is received, it processes it and sends response
4. Debug output is sent to UART (address 0x10000000)

## Testing

### Simple Test

```c
// Host-side test code
void test_hello(void) {
    // Wait for mailbox ready
    while (!(mailbox->status & MAILBOX_READY));
    
    // Send HELLO command
    mailbox->command = MAILBOX_CMD_HELLO;
    mailbox->status = 0;  // Trigger execution
    
    // Wait for completion
    while (mailbox->status & MAILBOX_BUSY);
    
    // Check response
    if (mailbox->response == MAILBOX_SUCCESS) {
        printf("HELLO command executed successfully\n");
    }
}
```

### Expected Output

When the HELLO command is executed, the firmware will output:
```
[INFO]: Firmware starting...
[COMMAND]: HELLO (0x00000001)
[RESPONSE]: 0x00000000 (SUCCESS)
Hello from firmware!
```

## Directory Structure

```
firmware/
├── include/
│   └── mailbox.h          # Header file
├── src/
│   ├── main.c             # Main firmware code
│   └── mailbox.c          # Mailbox handling
├── linker/
│   └── firmware.ld        # Linker script
├── build/                 # Build output directory
├── Makefile              # Build system
└── README.md             # This file
```

## Customization

### Adding New Commands

1. Add command code to `mailbox.h`:
   ```c
   #define MAILBOX_CMD_NEW_COMMAND 0x00000020
   ```

2. Add handler function in `main.c`:
   ```c
   uint32_t handle_new_command(void) {
       print_string("New command executed\n");
       return MAILBOX_SUCCESS;
   }
   ```

3. Add to command switch statement:
   ```c
   case MAILBOX_CMD_NEW_COMMAND:
       response = handle_new_command();
       break;
   ```

### Modifying Memory Layout

Edit `linker/firmware.ld` to change:
- ROM/RAM addresses and sizes
- Section ordering
- Stack and heap sizes

## Troubleshooting

### Build Issues

- **Toolchain not found**: Set `RISCV_PREFIX` environment variable
- **Linker errors**: Check linker script paths and memory regions
- **Size too large**: Reduce code size or increase memory in linker script

### Runtime Issues

- **Commands not working**: Verify mailbox address matches Spike configuration
- **Hang on startup**: Check stack/heap sizes in linker script

## License

This firmware is part of the Spike Wrapper project. See the main project LICENSE file for details.

## References

- [RISC-V Instruction Set Manual](https://riscv.org/technical/specifications/)
- [Spike RISC-V Simulator](https://github.com/riscv-software-src/riscv-isa-sim)
- [RVV Mailbox Design Document](../rvv_mailbox.md)
