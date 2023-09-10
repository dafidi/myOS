
SECTIONS
{
    .text 0x100000 : {
        PROVIDE(_begin_text = ( ADDR(.text) ));
        *(.text);
        PROVIDE(_end_text = ( ADDR(.text) + SIZEOF(.text) ));
    }
    .data : { 
        PROVIDE(_begin_data = ( ADDR(.data) ));
        *(.data) }
        PROVIDE(_end_data = ( ADDR(.data) + SIZEOF(.data) ));
    .bss : {
        PROVIDE(_begin_bss = ( ADDR(.bss) ));
        *(.bss)
        *(.COMMON)
        PROVIDE(_end_bss = ( ADDR(.bss ) + SIZEOF(.bss) ));
    }
}
ENTRY(kernel_entry)
