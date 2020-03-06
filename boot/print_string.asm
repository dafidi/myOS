;*******************************************************************************
; Simple function to print a string given that the address of the string is 
; passed in bx.
[bits 16]
print_string:
	pusha			;ensure to save all registers
	mov ah, 0x0e		;set BIOS to scrolling teletype mode
	mov al, '['
	int 0x10
print_loop:
	mov al, [bx]
	cmp al, 0
	je print_loop_end
	int 0x10
	add bx, 1
	jmp print_loop
print_loop_end: 
	mov al, ']'
	int 0x10
	popa
	ret

print_endl:
	pusha
	mov ah, 0x0e
	mov al, 10
	int 0x10
	mov al, 13
	int 0x10
	popa
	ret
; END of print_string
;*******************************************************************************
;*******************************************************************************
; A simple boot sector program to print hexadecimal value of a given 2-byte value
; passed in by dx

[bits 16]
print_hex:
	pusha

	call clear_template

	mov bx, HEX_OUT
	add bx, 5

to_ascii_loop:
	mov ax, dx
	and ax, 0xf

	cmp ax, 10
	jge to_alpha
	add ax, 48
	jmp end_to_ascii_loop

to_alpha:
	add ax, 87

end_to_ascii_loop:
	mov [bx], al
	add bx, -1
	shr dx, 4
	cmp dx, 0
	je exit_print_hex
	jmp to_ascii_loop

exit_print_hex:
	mov bx, HEX_OUT
	call print_string
	popa
	ret

HEX_OUT:
	db "0x0000", 0 
; END of print_hex. 
;********************************************************************************

clear_template:
	push bx
	mov bl, '0'
	mov [HEX_OUT+2], bl
	mov [HEX_OUT+3], bl
	mov [HEX_OUT+4], bl
	mov [HEX_OUT+5], bl
	pop bx
	ret
