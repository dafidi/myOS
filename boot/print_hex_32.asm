; Prints a 32-bit value passed in by edx.
print_hex_32:
	pushad
	call clear_template_32

	mov ebx, HEX_OUT_32
	add ebx, 9

to_ascii_loop_32:
	mov eax, edx
	and eax, 0xf

	cmp eax, 10
	jge to_alpha_32
	add eax, 48
	jmp end_to_ascii_loop_32

to_alpha_32:
	add eax, 87

end_to_ascii_loop_32:
	mov [ebx], al
	add ebx, -1
	shr edx, 4
	cmp edx, 0
	je exit_print_hex_32
	jmp to_ascii_loop_32

exit_print_hex_32:
	mov ebx, HEX_OUT_32
	call print_string
	popad
	ret

HEX_OUT_32:
	db "0x00000000", 0 

clear_template_32:
	push ebx
	mov bl, '0'
	mov [HEX_OUT_32+2], bl
	mov [HEX_OUT_32+3], bl
	mov [HEX_OUT_32+4], bl
	mov [HEX_OUT_32+5], bl
	mov [HEX_OUT_32+6], bl
	mov [HEX_OUT_32+7], bl
	mov [HEX_OUT_32+8], bl
	mov [HEX_OUT_32+9], bl
	pop ebx
	ret

