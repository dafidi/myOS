[org 0x7c00]
the_secret:
	db "X"

mov ah, 0x0e	;indicates scrolling teletype mode for screen-related BIOS interrupt.

;First attempt
mov al, the_secret
int 0x10

;second attempt
mov al, [the_secret]
int 0x10

;third attempt
mov bx, the_secret
add bx, 0x00
mov al, [bx]
int 0x10

;fourth attempt
mov al, [0x7c00]
int 0x10

jmp $

times 510 - ($ - $$) db 0

dw 0xaa55

