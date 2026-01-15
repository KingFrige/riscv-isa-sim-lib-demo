/* Linker script for RISC-V firmware */
OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY {
    /* 将所有内容都放在 RAM 段中，避免跨段重定位问题 */
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 640K  /* 从DRAM起始地址开始，640KB总内存 */
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
    } > ram

    /* 只读数据段 */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        *(.srodata)
        *(.srodata.*)
    } > ram

    /* 构造函数 */
    .init_array : {
        __init_array_start = .;
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    } > ram

    /* 析构函数 */
    .fini_array : {
        __fini_array_start = .;
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP (*(.fini_array))
        __fini_array_end = .;
    } > ram

    /* 数据段 */
    .data : {
        __data_start = .;
        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
        __data_end = .;
    } > ram

    /* HTIF通信符号 (for Spike) - 需要KEEP保护防止被gc-sections删除 */
    .tohost : {
        KEEP(*(.tohost))
    } > ram
    .fromhost : {
        KEEP(*(.fromhost))
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

    /* 堆区域（在栈之后） - 只定义符号，不分配实际空间 */
    .heap (NOLOAD) : {
        . = ALIGN(16);
        __heap_start = .;
        /* 不分配实际空间，只定义符号 */
    } > ram
    
    /* 堆结束地址符号 */
    __heap_end = ORIGIN(ram) + LENGTH(ram);

    /* 符号定义 */
    __ram_start = ORIGIN(ram);
    __ram_end = ORIGIN(ram) + LENGTH(ram);

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
