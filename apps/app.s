    .globl main
    .intel_syntax noprefix

main:
    # Checking with objdump, 0x30000007 is the next line with the add instruction.
    jmp 0x8: 0x30000007
    add eax, ebx
exit:
    iret
