

1. Why does it seem like changing the start of the .bss section in kernel.ld sometimes triggers General Page Faults, Invalid Opcode, etc? I'm guessing misaligned addresses but that hand-wavy, I want details.
2. It appears I cannot reorder the code and segment descriptors in the GDT.
Specifically, doing the following:
    Fill the GDT in this order:
        0: null descriptor,
        8: data segment descriptor,
        16: code segment descriptor,
    load the gdtr,
    then, conducting a far jump to update CS to 16
    and finally, updating DS, SS, ES, FS, and GS to 8
causes issues as soon as interrupts are enabled.

It seems there is sometimes(?) a problem writing to and reading form zones MAX_ORDER and MAX_ORDER-1 (see memory_object_cache_init). This came up while allocating a block to support memory object allocation. MAX_ORDER-2 is fine. Might be worth investigating what's going on there.
