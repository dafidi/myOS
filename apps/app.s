    .globl main
    .intel_syntax noprefix

main:
    #jmp $
    #;jmp 0xbaba
    add eax, ebx
    iret
loop:
    mov eax, 0x1
    cmp eax, 0x1
    jmp 0x8: 0x119f
exit:
    iret
    jmp 0x0:0x119d
