SECTIONS {
  . = 0x80000000;
  .text : { *(.text) }
  .rodata : { *(.rodata) }
  .data : { *(.data) }
  .bss : { *(.bss) }
  .srodata : { *(.srodata) }
  
  /* Ensure code section has enough space before vector test data */
  /* Based on objdump, code ends around 0x80004000, so place data after that */
  /* Reserve extra space to prevent accidental execution of data area */
  . = 0x8000a000;  /* Place data sections well after code to avoid overlap, with extra margin */
  
  /* Vector test data sections */
  .vector_data_v0 : {
    *(.vector_data_v0)
  }
  
  . = 0x8000a100;
  .vector_data_v2 : {
    *(.vector_data_v2)
  }
  
  . = 0x8000a200;
  .vector_data_v4 : {
    *(.vector_data_v4)
  }
  
  . = 0x8000a300;
  .vector_data_v6 : {
    *(.vector_data_v6)
  }
  
  /* Additional vector test data sections */
  . = 0x8000a400;
  .vector_data_v8 : {
    *(.vector_data_v8)
  }
  
  . = 0x8000aa00;
  .vector_data_v12 : {
    *(.vector_data_v12)
  }
  
  . = 0x8000ae00;
  .vector_data_v16 : {
    *(.vector_data_v16)
  }
  
  . = 0x8000b000;
  .vector_data_v20 : {
    *(.vector_data_v20)
  }
  
  . = 0x8000b200;
  .vector_data_v24 : {
    *(.vector_data_v24)
  }
  
  . = 0x8000b400;
  .vector_data_v28 : {
    *(.vector_data_v28)
  }
  
  . = 0x8000b600;
  .vector_data_v32 : {
    *(.vector_data_v32)
  }
  
  . = 0x8000b800;
  .vector_data_v36 : {
    *(.vector_data_v36)
  }
  
  . = 0x8000ba00;
  .vector_data_v40 : {
    *(.vector_data_v40)
  }
  
  . = 0x8000bc00;
  .vector_data_v44 : {
    *(.vector_data_v44)
  }
  
  . = 0x8000be00;
  .vector_data_v48 : {
    *(.vector_data_v48)
  }
  
  . = 0x8000c000;
  .vector_data_v52 : {
    *(.vector_data_v52)
  }
  
  . = 0x8000c200;
  .vector_data_v56 : {
    *(.vector_data_v56)
  }
  
  . = 0x8000c400;
  .vector_data_v60 : {
    *(.vector_data_v60)
  }
  
  . = 0x8000c600;
  .vector_data_v64 : {
    *(.vector_data_v64)
  }
  
  . = 0x8000c800;
  .vector_data_v68 : {
    *(.vector_data_v68)
  }
  
  . = 0x8000ca00;
  .vector_data_v72 : {
    *(.vector_data_v72)
  }
  
  . = 0x8000cc00;
  .vector_data_v76 : {
    *(.vector_data_v76)
  }
  
  . = 0x8000ce00;
  .vector_data_v80 : {
    *(.vector_data_v80)
  }
  
  /* Place tohost section after other sections to avoid overlap */
  . = 0x8000d000;
  .tohost : { *(.tohost) }
}
