
# Remember:
#	$< = first argument for target
#	$^ = all arguments for target
#	$@ = name of target

# Generate list of sources automatically.
C_SOURCES = $(wildcard kernel/*.c drivers/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h)

# Generate object file names to build based on *.c filenames.
#OBJ = ${C_SOURCES: .c=.o}
OBJ = $(patsubst %.c, %.o, ${C_SOURCES})

# Default build target.
all: os-image

# I concatenate null.bin  (which is a binary with nothing but 0x0) because I 
# was experiencing errors which trying to load disc sectors for more data than
# is contained in the binary. Weirdly enough, it seemed to solve the problem 
# the first time, but then I tried running the image again without concatenating
# null.bin - this did not result in errors. I therefore concluded that what
# happened was:
# 	The host system would not let me read data from beyond the where the
#	kernel was stored in memory (probably because of some type of "accessed"
#	flag(s)). After I added null the first time, the flag was probably then
#	changed such that when I tried it again, the flag(s) was/were set so that
#	reading for that part of memory threw no errors :).
os-image: boot/boot_sect.bin  kernel.bin null.bin
	cat $^ > os-image

kernel.bin: kernel/kernel_entry.o ${OBJ}
	ld -o kernel.bin -m elf_i386 -Ttext 0x1000 $^ --oformat binary

%.o: %.c
	gcc -m32 -fno-pie -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

#boot_sect.bin: boot_sect.asm
#	nasm $< -f bin -o $@

#kernel_entry.o: kernel_entry.asm
#	nasm $< -f elf -o $@

# The -m32 flag lets the compiler know to compile the kernel for a 32-bit 
# machine. This has a side effect of throwing the error:
# 	"undefined reference to `_GLOBAL_OFFSET_TABLE_'"
# I fix this by adding the -fno-pie flag.
#kernel.o: kernel.c
#	gcc -m32 -fno-pie -c $< -o $@

#null.bin: null.asm
#	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os-image *.map
	rm -rf kernel/*.o boot/*.bin drivers/*.o

