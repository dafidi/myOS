
SECTIONS
{
  . = 0xa00000;
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : {
   _bss_start = ( ADDR(.bss) ) ;
    *(.bss)
    *(.COMMON)
   _bss_end = ( ADDR(.bss ) + SIZEOF(.bss) ) ;
  }
}

ENTRY(main)
