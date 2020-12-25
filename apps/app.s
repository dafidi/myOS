    .globl main
    .intel_syntax noprefix

main:
    jmp 0x8: exit
exit:
    iret
