    .globl main
    .intel_syntax noprefix

main:
    #jmp $
    jmp 0x1b: exit
exit:
    iret
