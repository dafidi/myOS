# myOS
This is a hobby operating systems project.

## Prerequisites

* gcc: I think any version will do ... maybe.
* qemu: qemu-system-i386 specifically (this is currently the only targeted platform).
* gdb: well, this isn't really needed but is expected by default. If you don't want to install gdb for some reason, you can simply remove the "`-s -S`" flags from scripts/start.sh

## Executing the kernel

To run, simply run the script:

`$ ./scripts/run.sh`

from the root folder.

## Typical Workflow

When you run `scripts/run.sh`, Qemu will launch a [curses](https://en.wikipedia.org/wiki/Curses_(programming_library)) window in your terminal. By default, Qemu will not immediately launch the VM but wait for a connection on tcp port 1234 (this is theeffect of the "`-s -S`" flags in `scripts/start.sh`). In another terminal, you can cd into the project's root directory and run:

`$ gdb kernel.elf`

You can then connect to the Qemu instance by running:

`$ gdb> target remote :1234`

Then let the VM continue execution:

`$ gdb> c`

After this you can use `ctrl+c` to pause the VM.

You can then try a couple of things from gdb:

### inspect registers

`$ gdb> info reg [<register_name>]*`

Note: Not supplying a specific register will cause all the registers to be displayed.

### step through instructions

`$ gdb> stepi`

This will cause the VM to execute one instruction: the next instruction.

### Print register value

For example, to print the current value in the eax register 

`$ gdb> print $eax`

### View content at memory address

To view the content at 0xb8000 for example:

`$ gdb> x/x 0xb8000`

### Add a breakpoint to the kernel

Gdb lets us add breakpoints in multiple ways:

#### By function name:

To set a breakpoint at `main` in `kernel/kernel.c` for example:

`$ gdb> b main`

#### By line number:

To set a break point at `line 50` in `kernel/kernel.c`: 

`$ gdb> b kernel/kernel.c:50`

#### By address:

To set a breakpoint at `0x1000`:

`b *0x1000`

# Cheers
