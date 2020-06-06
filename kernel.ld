
SECTIONS
{
  . = 0x1000;
  .text : { *(.text) }
  .data : { *(.data) }
  .bss 0x100000 : {
   _bss_start = ( ADDR(.bss) ) ;
    *(.bss)
    *(.COMMON)
   _bss_end = ( ADDR(.bss ) + SIZEOF(.bss) ) ;
  }
}

ENTRY(kernel_entry)
