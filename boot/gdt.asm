; A simple program to switch from 16-bit real mode to 32-bit protected mode.

gdt_start:

gdt_null: ; mandatory null descriptor as first entry in GDT
	dd 0x0
	dd 0x0

gdt_code:
	; base = 0x0, limit = 0xfffff
	; 1st flags: (present)1 (privilege)00 (descriptor type)1 -> 1001b
	; type flags: (code)1 (conforming)0 (readable)1 (accessed)0 -> 1010b
	; 2nd flags: (granularity)1 (32-bit default)1 (64-bit seg)0 (AVL)0 -> 1100b
	dw 0xffff	; Limit bits 0-15
	dw 0x0		; Base bits 0-15
	db 0x0		; Base bits 16-23
	db 10011010b	; 1st flags, type flags
	db 11001111b	; 2nd flags, Limit bits 16-19
	db 0x0		; Base bits 24-31

gdt_data:
	; Same as code section except for the type flags
	; type flags (code)0 (expand down)0 (writeable)1 (accessed)0 -> 0010b
	dw 0xffff	; Limit (bits 0-15)
	dw 0x0		; Base (bits 0-15_
	db 0x0		;Base (bits 16-23)
	db 10010010b	; 1st flags, type flags
	db 11001111b	; 2nd flags, Limit (bits 16-19)
	db 0x0		; Base (bits 24-31)

gdt_end:		; This label helps us calculate the size of the GDT.

gdt_descriptor:
	dw gdt_end - gdt_start - 1
	dd gdt_start

; Variables to mark offset (in bytes) to from the start if the GDT to the code and data segments.
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

