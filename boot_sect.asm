; A simple boot sector program demonstrating conditionals.

mov bx, 30

cmp bx, 4
jg compare_with_40
mov al, 'A'
jmp end

compare_with_40:
	cmp bx, 40
	jge else
	mov al, 'B'
	jmp end

else:
	mov al, 'C'

end:
	mov ah, 0x0e
	int 0x10
	jmp $

times 510 - ($ - $$) db 0

dw 0xaa55

