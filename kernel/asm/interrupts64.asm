[bits 64]

global enable_interrupts64
enable_interrupts64:
    sti
    ret

global disable_interrupts64
disable_interrupts64:
    cli
    ret

extern fault_handler64
extern irq_handler64

global isr64_common
global isr64_0
global isr64_1
global isr64_2
global isr64_3
global isr64_4
global isr64_5
global isr64_6
global isr64_7
global isr64_8
global isr64_9
global isr64_10
global isr64_11
global isr64_12
global isr64_13
global isr64_14
global isr64_15
global isr64_16
global isr64_17
global isr64_18
global isr64_19
global isr64_20
global isr64_21
global isr64_22
global isr64_23
global isr64_24
global isr64_25
global isr64_26
global isr64_27
global isr64_28
global isr64_29
global isr64_30
global isr64_31

global irq64_0
global irq64_1
global irq64_2
global irq64_3
global irq64_4
global irq64_5
global irq64_6
global irq64_7
global irq64_8
global irq64_9
global irq64_10
global irq64_11
global irq64_12
global irq64_13
global irq64_14
global irq64_15

global isrs64_begin
global isrs64_end
global irqs64_begin
global irqs64_end

; ; In 64-bit mode, we can skip pushing dummy error codes (push byte 0) because
; the processor offers a uniform format for interrupt stack frame regardless
; of whether there is a changein privilege level or not. These are commented
; out. When the code is verified to work, they will be removed.
; See Intel IA-32 Developer's manual Vol. 3 Section 6.14, Figure 6-9 for more information.

isrs64_begin:
isr64_0:
    cli
    ; push byte 0
    push byte 0
    jmp isr64_common

isr64_1:
    cli
    ; push byte 0
    push byte 1
    jmp isr64_common

isr64_2:
    cli
    ; push byte 0
    push byte 2
    jmp isr64_common

isr64_3:
    cli
    ; push byte 0
    push byte 3
    jmp isr64_common

isr64_4:
    cli
    ; push byte 0
    push byte 4
    jmp isr64_common

isr64_5:
    cli
    ; push byte 0
    push byte 5
    jmp isr64_common

isr64_6:
    cli
    ; push byte 0
    push byte 6
    jmp isr64_common

isr64_7:
    cli
    ; push byte 0
    push byte 7
    jmp isr64_common

isr64_8:
    cli
    push byte 8
    jmp isr64_common

isr64_9:
    cli
    ; push byte 0
    push byte 9
    jmp isr64_common

isr64_10:
    cli
    push byte 10
    jmp isr64_common

isr64_11:
    cli
    push byte 11
    jmp isr64_common

isr64_12:
    cli
    push byte 12
    jmp isr64_common

isr64_13:
    cli
    push byte 13
    jmp isr64_common

isr64_14:
    cli
    push byte 14
    jmp isr64_common

isr64_15:
    cli
    ; push byte 0
    push byte 15
    jmp isr64_common

isr64_16:
    cli
    ; push byte 0
    push byte 16
    jmp isr64_common

isr64_17:
    cli
    ; push byte 0
    push byte 17
    jmp isr64_common

isr64_18:
    cli
    ; push byte 0
    push byte 18
    jmp isr64_common

isr64_19:
    cli
    ; push byte 0
    push byte 19
    jmp isr64_common

isr64_20:
    cli
    ; push byte 0
    push byte 20
    jmp isr64_common

isr64_21:
    cli
    ; push byte 0
    push byte 21
    jmp isr64_common

isr64_22:
    cli
    ; push byte 0
    push byte 22
    jmp isr64_common

isr64_23:
    cli
    ; push byte 0
    push byte 23
    jmp isr64_common

isr64_24:
    cli
    ; push byte 0
    push byte 24
    jmp isr64_common

isr64_25:
    cli
    ; push byte 0
    push byte 25
    jmp isr64_common

isr64_26:
    cli
    ; push byte 0
    push byte 26
    jmp isr64_common

isr64_27:
    cli
    ; push byte 0
    push byte 27
    jmp isr64_common

isr64_28:
    cli
    ; push byte 0
    push byte 28
    jmp isr64_common

isr64_29:
    cli
    ; push byte 0
    push byte 29
    jmp isr64_common

isr64_30:
    cli
    ; push byte 0
    push byte 30
    jmp isr64_common

isr64_31:
    cli
    ; push byte 0
    push byte 31
    jmp isr64_common

isr64_common:
    push rax
    push rcx
    push rdx
    push rbx
    push rsp
    push rbp
    push rsi
    push rdi
    mov rax, rsp
    mov rdi, rax
    ;push rax
    mov rax, fault_handler64
    call rax
    ;pop rax
    pop rdi
    pop rsi
    pop rbp
    pop rsp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    add rsp, 16
    iretq
isrs64_end:

; Error codes are not pushed for additional interrupt service handlers. Let's "push byte 0"
; just for consistency.
irqs64_begin:
irq64_0:
    cli
    push byte 0
    push byte 32
    jmp irq64_common

irq64_1:
    cli
    push byte 0
    push byte 33
    jmp irq64_common

irq64_2:
    cli
    push byte 0
    push byte 34
    jmp irq64_common

irq64_3:
    cli
    push byte 0
    push byte 35
    jmp irq64_common

irq64_4:
    cli
    push byte 0
    push byte 36
    jmp irq64_common

irq64_5:
    cli
    push byte 0
    push byte 37
    jmp irq64_common

irq64_6:
    cli
    push byte 0
    push byte 38
    jmp irq64_common

irq64_7:
    cli
    push byte 0
    push byte 39
    jmp irq64_common

irq64_8:
    cli
    push byte 0
    push byte 40
    jmp irq64_common

irq64_9:
    cli
    push byte 0
    push byte 41
    jmp irq64_common

irq64_10:
    cli
    push byte 0
    push byte 42
    jmp irq64_common

irq64_11:
    cli
    push byte 0
    push byte 43
    jmp irq64_common

irq64_12:
    cli
    push byte 0
    push byte 44
    jmp irq64_common

irq64_13:
    cli
    push byte 0
    push byte 45
    jmp irq64_common

irq64_14:
    cli
    push byte 0
    push byte 46
    jmp irq64_common

irq64_15:
    cli
    push byte 0
    push byte 47
    jmp irq64_common

irq64_common:
    push rax
    push rcx
    push rdx
    push rbx
    push rsp
    push rbp
    push rsi
    push rdi
    mov rax, rsp
    mov rdi, rax
    mov rax, irq_handler64
    call rax
    pop rdi
    pop rsi
    pop rbp
    pop rsp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    add rsp, 16
    iretq
irqs64_end:
