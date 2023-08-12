
SECTIONS
{
    . = 0x9000;
    .text : {
        PROVIDE(begin_text = ( ADDR(.text) ));
        *(.text);
        PROVIDE(end_text = ( ADDR(.text) + SIZEOF(.text) ));
    }
    .data : { 
        PROVIDE(begin_data = ( ADDR(.data) ));
        *(.data) }
        PROVIDE(end_data = ( ADDR(.data) + SIZEOF(.data) ));
    .bss 0x100000 : {
        PROVIDE(begin_bss = ( ADDR(.bss) ));
        *(.bss)
        *(.COMMON)
        PROVIDE(end_bss = ( ADDR(.bss ) + SIZEOF(.bss) ));
    }
}
ENTRY(kernel_entry)
