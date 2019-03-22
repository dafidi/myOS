
mov ah, 0x0e	;indicates scrolling teletype mode for screen-related BIOS interrupt.

mov al, 'D'

int 0x10

times 510 - ($ - $$) db 0

dw 0xaa55

