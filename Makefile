
# Remember:
#	$< = first argument for target
#	$^ = all arguments for target
#	$@ = name of target

BINDIR=bin

# Generate list of sources automatically.
C_SOURCES=$(wildcard kernel/*.c kernel/**/*.c drivers/**/*.c fs/*.c)
HEADERS=$(wildcard kernel/*.h kernel/**/*.h drivers/*.h fs/*.h)

# Generate object file names to build based on *.c filenames.
OBJ = $(patsubst %.c, %.o, ${C_SOURCES})

boot_sector_deps=boot_sect_1.bin boot_sect_2.bin

GNU_CC=/Users/davidawogbemila/Downloads/Software_Installation/gcc-13.1.0/bin/gcc
OBJCOPY=/usr/local/opt/llvm/bin/llvm-objcopy

MAC_BUILD_C_FLAGS = -DBUILDING_ON_MAC

%.o: %.c
	${GNU_CC} ${C_FLAGS} -g -c $< -o $@

# %.o: %.asm
# 	nasm $< ${ASM_FLAGS} -o $@

%.o: %.s
	${GNU_CC} ${C_FLAGS} -g -c $< -o $@

%.bin: %.asm
	nasm $< -f bin -o $@
	ndisasm $@ > $@.dis

# Default build target, 64 bit.
default: myOS.img

myOS.img: boot_sectors kernel64.bin
	cat boot_sect_1.bin boot_sect_2.bin kernel64.bin > myOS.img

myOS32.img: boot_sectors kernel32.bin
	cat boot_sect_1.bin boot_sect_2.bin kernel32.bin > myOS32.img

boot_sectors: boot/boot_sect_1.bin boot/boot_sect_2.bin
	cp boot/*.bin ./

PLATFORM_FLAGS=${MAC_BUILD_C_FLAGS}
kernel64.bin-Mac: kernel64.bin

kernel64.bin: C_FLAGS = -Wall -O0 -m64 -fno-pie -fno-pic -no-pie \
-fno-stack-protector -ffreestanding -nolibc -fno-leading-underscore \
-nostdlib  ${PLATFORM_FLAGS}\
-I./
kernel64.bin: ASM_FLAGS = -f macho64 -g
kernel64.bin: kernel64.elf
	./scripts/extract_kernel_bin.sh mode=64

kernel32.bin: C_FLAGS = -Wall -O0 -m32 -fno-pie -fno-pic -no-pie \
-fno-stack-protector -ffreestanding -nolibc -fno-leading-underscore \
-nostdlib -DCONFIG32 \
-I./
kernel32.bin: ASM_FLAGS = -f elf -g -DCONFIG32
kernel32.bin: kernel32.elf
	objcopy -O binary kernel32.elf kernel32.bin

kernel64.elf: kernel/asm/kernel_entry.o ${OBJ}
	# ld -o $@ -arch x86_64 $^ -static -order_file kernel.ld
	${GNU_CC} ${C_FLAGS} -static -v -g $^ -o $@ -Wl,-e,kernel_entry \
	-Wl,-order_file_statistics -Wl,-exported_symbols_list,order_file.txt \
	-Wl,-pagezero_size,0  -Wl,-segaddr,__TEXT 0x100000 -Wl,-preload
	objdump -d kernel64.elf > kernel.asm.dis

kernel32.elf: kernel/asm/kernel_entry.o ${OBJ}
	ld -o kernel32.elf -m elf_i386 $^ --oformat elf32-i386 -T kernel.ld
	objdump -d kernel32.elf > kernel32.asm.dis

# Not sure why but using kernel_entry.s results in a bad error where 
# eip inexplicably jumps to 0xfbxxxxxx.
# kernel.elf:
# 	gcc -o kernel.elf -T kernel.ld -fuse-ld=gold ${C_FLAGS} kernel/kernel_entry.s ${C_SOURCES}
# 	objdump -d kernel.elf > kernel.s.dis

clean:
	rm -rf *.bin *.o *.map *.img *.elf *.dis *.hdd
	rm -rf kernel/*.o kernel/**/*.o boot/*.bin drivers/**/*.o fs/*.o

app.bin:
	make -C apps/ s
