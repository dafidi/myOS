
# Remember:
#	$< = first argument for target
#	$^ = all arguments for target
#	$@ = name of target

# Generate list of sources automatically.
C_SOURCES = $(wildcard kernel/*.c kernel/**/*.c drivers/**/*.c fs/*.c)
HEADERS = $(wildcard kernel/*.h kernel/**/*.h drivers/*.h fs/*.h)

C_FLAGS = -Wall -O0 -m32 -fno-pie -fno-pic -no-pie \
-fno-stack-protector -ffreestanding -fno-hosted -nolibc \
-nostdlib \
-I./

# Generate object file names to build based on *.c filenames.
#OBJ = ${C_SOURCES: .c=.o}
OBJ = $(patsubst %.c, %.o, ${C_SOURCES})

# Default build target.
all: os-image

boot_sector_deps=boot_sect_1.bin boot_sect_2.bin
kernel_deps=kernel.bin
os_image_deps=$(boot_sector_deps) $(kernel_deps) 

os-image: $(os_image_deps) storage_disk.img
	cat $(os_image_deps) > os-image

boot_sect_1.bin: boot/boot_sect_1.bin
	cp boot/boot_sect_1.bin boot_sect_1.bin

boot_sect_2.bin: boot/boot_sect_2.bin
	cp boot/boot_sect_2.bin boot_sect_2.bin

kernel.elf: kernel/kernel_entry.o ${OBJ}
	ld -o kernel.elf -m elf_i386 $^ --oformat elf32-i386 -T kernel.ld
	objdump -d kernel.elf > kernel.asm.dis

# Not sure why but using kernel_entry.s results in  a bad error where 
# eip inexplicably jumps to 0xfbxxxxxx.
# kernel.elf:
# 	gcc -o kernel.elf -T kernel.ld -fuse-ld=gold ${C_FLAGS} kernel/kernel_entry.s ${C_SOURCES}
# 	objdump -d kernel.elf > kernel.s.dis

kernel.bin: kernel.elf
	objcopy -O binary kernel.elf kernel.bin

app.bin:
	make -C apps/ s

storage_disk.img: app.bin
	nasm -f bin -o storage_disk.img storage_disk.asm
	# Append app.bin
	cat apps/app.bin >> storage_disk.img

%.o: %.c
	gcc ${C_FLAGS} -c $< -o $@

%.o: %.asm
	nasm $< -f elf -g -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.o os-image *.map *.img *.elf #*.dis
	rm -rf kernel/*.o kernel/**/*.o boot/*.bin drivers/**/*.o fs/*.o

