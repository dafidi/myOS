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

;*******************************************************************************
; A simple boot sector program to print hexadecimal value of a given 2-byte value
; passed in by dx

print_hex_pm:
	pushad

	call clear_template_pm

	mov ebx, HEX_OUT_pm
	add ebx, 9

to_ascii_loop_pm:
	mov eax, edx
	and eax, 0xf

	cmp eax, 10
	jge to_alpha_pm
	add eax, 48
	jmp end_to_ascii_loop_pm

to_alpha_pm:
	add eax, 87

end_to_ascii_loop_pm:
	mov [ebx], al
	add ebx, -1
	shr edx, 4
	cmp edx, 0
	je exit_print_hex_pm
	jmp to_ascii_loop_pm

exit_print_hex_pm:
	mov ebx, HEX_OUT_pm
	call print_string_pm
	popad
	ret

HEX_OUT_pm:
	db "0x00000000", 0
; END of print_hex_pm.

clear_template_pm:
	push ebx
	mov bl, '0'
	mov [HEX_OUT_pm+2], bl
	mov [HEX_OUT_pm+3], bl
	mov [HEX_OUT_pm+4], bl
	mov [HEX_OUT_pm+5], bl
	mov [HEX_OUT_pm+6], bl
	mov [HEX_OUT_pm+7], bl
	mov [HEX_OUT_pm+8], bl
	mov [HEX_OUT_pm+9], bl
	pop ebx
	ret
;********************************************************************************
