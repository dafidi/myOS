; The issue with this function is that we always overwrite the
; the text at the top left of the screen.

[bits 32]
; Define some constants
VIDEO_MEMORY equ 0xb8000
WHITE_ON_BLACK equ 0x0f

; prints a null-terminated string pointed to by ebx
print_string_pm:
	pusha
	mov edx, VIDEO_MEMORY

print_string_pm_loop:
	mov al, [ebx]
	mov ah, WHITE_ON_BLACK

	cmp al, 0
	je print_string_pm_done

	mov [edx], ax

	add ebx, 1
	add edx, 2

	jmp print_string_pm_loop

print_string_pm_done:
	popa
	ret

print_hex:
	pusha
	mov ebx, HEX_OUT
	add ebx, 9

to_ascii_loop:
	mov eax, edx
	and eax, 0xf

	cmp eax, 10
	jge to_alpha
	add eax, 48
	jmp end_to_ascii_loop

to_alpha:
	add eax, 87

end_to_ascii_loop:
	mov [ebx], al
	add ebx, -1
	shr edx, 4
	cmp edx, 0
	je exit_print_hex
	jmp to_ascii_loop

exit_print_hex:
	mov ebx, HEX_OUT
	call print_string_pm
	popa
	ret

HEX_OUT:
	db "0x00000000", 0 
; END of print_hex. 
;********************************************************************************
