SECTIONS {
  . = 0x80000000;
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
  .srodata : { *(.srodata) }
  
  /* Vector test data sections */
  . = 0x80001000;
  .vector_data_v0 : {
    *(.vector_data_v0)
  }
  
  . = 0x80001100;
  .vector_data_v2 : {
    *(.vector_data_v2)
  }
  
  . = 0x80001200;
  .vector_data_v4 : {
    *(.vector_data_v4)
  }
  
  . = 0x80001300;
  .vector_data_v6 : {
    *(.vector_data_v6)
  }
  
  . = ALIGN(0x1000);
  .tohost : { *(.tohost) }
}
