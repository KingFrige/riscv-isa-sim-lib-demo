/* Linker script for RISC-V firmware */
OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY {
    /* Spike 的 ROM 通常从 0x1000 开始 */
    rom (rx) : ORIGIN = 0x1000, LENGTH = 64K
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 512K
}

SECTIONS {
    /* 代码段 */
    .text : {
        *(.text._start)
        *(.text.firmware_main)
        *(.text.handle_mailbox_command)
        *(.text.handle_hello_command)
        *(.text.handle_hi_command)
        *(.text.handle_vector_load)
        *(.text.handle_vector_store)
        *(.text.handle_vector_compute)
        *(.text)
        *(.text.*)
    } > rom

    /* 只读数据段 */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        *(.srodata)
        *(.srodata.*)
    } > rom

    /* 构造函数 */
    .init_array : {
        __init_array_start = .;
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    } > rom

    /* 析构函数 */
    .fini_array : {
        __fini_array_start = .;
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP (*(.fini_array))
        __fini_array_end = .;
    } > rom

    /* 数据段（加载到 ROM，运行时复制到 RAM） */
    .data : AT(ADDR(.rodata) + SIZEOF(.rodata)) {
        __data_start = .;
        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
        __data_end = .;
    } > ram

    /* BSS 段（未初始化数据） */
    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
        __bss_end = .;
    } > ram

    /* 确保 BSS 段不会占用所有 RAM */
    . = ALIGN(16);
    __bss_limit = .;
    
    /* 堆栈区域（在 BSS 之后） */
    .stack : {
        . = ALIGN(16);
        __stack_bottom = .;
        . += 4K; /* 4KB 栈空间 */
        __stack_top = .;
    } > ram

    /* 堆区域（在栈之后） */
    .heap : {
        . = ALIGN(16);
        __heap_start = .;
        . = ORIGIN(ram) + LENGTH(ram) - 1;
        __heap_end = .;
    } > ram

    /* 符号定义 */
    __rom_start = ORIGIN(rom);
    __rom_end = ORIGIN(rom) + LENGTH(rom);
    __ram_start = ORIGIN(ram);
    __ram_end = ORIGIN(ram) + LENGTH(ram);

    /* 数据加载地址（在 ROM 中的位置） */
    __data_load_start = LOADADDR(.data);
    __data_load_end = __data_load_start + SIZEOF(.data);

    /* 丢弃其他段 */
    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}

/* 全局符号 */
PROVIDE(__global_pointer$ = __ram_start + 0x800);
PROVIDE(_end = __heap_end);
PROVIDE(__heap_size = __heap_end - __heap_start);
PROVIDE(__stack_size = __stack_top - __stack_bottom);