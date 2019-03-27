; A simple boot sector program to demoinstrate segment offsetting.

mov ah, 0x0e

mov dx, ds
call print_hex
call print_endl

mov bx, 0x7c0
mov ds, bx
mov al, [the_secret]
int 0x10
call print_endl

mov dx, ds
call print_hex
call print_endl

jmp $

the_secret:
	db "X"

%include "print_string.asm"

times 510 - ($ - $$) db 0

dw 0xaa55

