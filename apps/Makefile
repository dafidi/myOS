
#!/bin/bash

PROJECT_PATH=/Users/davidawogbemila/Develop/myOS

C_FLAGS = -Wall -O0 -m32 -nostdlib -fno-pic -fno-pie -fno-stack-protector -fno-hosted -ffreestanding
COMPILE_FLAGS = -Wall -O0 -m32 -nostdlib -fno-pic -fno-pie -fno-stack-protector -ffreestanding

GNU_CC=gcc
OBJCOPY=/usr/local/opt/llvm/bin/llvm-objcopy

MAC_PLATFORM_ARGS=-Wl,-pagezero_size,0  -Wl,-segaddr,__TEXT 0x30000000
LINUX_PLATFORM_ARGS=-tTtext=0x30000000

S_TARGET=UNCONFIGURED_S_TARGET

s-Mac: PLATFORM_ARGS=${MAC_PLATFORM_ARGS}
s-Mac: s

s-Linux: PLATFORM_ARGS=${LINUX_PLATFORM_ARGS}
s-Linux: s

s: S_FLAGS=${COMPILE_FLAGS} ${PLATFORM_ARGS}
s:	clean
	${GNU_CC} ${S_FLAGS} -static -v -g app.s -o app.s.elf -Wl,-e,main
	objdump -d app.s.elf > app.s.dis
	${PROJECT_PATH}/scripts/extract_app_bin.sh
	

c:	clean
	gcc -o app.c.elf ${C_FLAGS} -Ttext=0x4000 -Wl,-emain app.c
	objdump -d app.c.elf > app.c.dis
	objcopy -O binary app.c.elf app.bin

clean:
	rm -rf *.o *.bin
