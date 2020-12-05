
SECTIONS
{
	. = 0x1000;
	.text : {
		_text_start = ( ADDR(.text) );
		*(.text);
		_text_end = ( ADDR(.text) + SIZEOF(.text) );
	}
	.data : { 
		_data_start = ( ADDR(.data) );
		*(.data) }
		_data_end = ( ADDR(.data) + SIZEOF(.data) );
	.bss 0x100000 : {
		_bss_start = ( ADDR(.bss) ) ;
		*(.bss)
		*(.COMMON)
	 _bss_end = ( ADDR(.bss ) + SIZEOF(.bss) ) ;
	}
}

ENTRY(kernel_entry)
