; Simple function to print a string given that the address of the string is passed in bx.
print_string:
	pusha			;ensure to save all registers
	mov ah, 0x0e		;set BIOS to scrolling teletype mode
print_loop:
	mov al, [bx]
	cmp al, 0
	je print_loop_end
	int 0x10
	add bx, 1
	jmp print_loop
print_loop_end: 
	popa
	ret

