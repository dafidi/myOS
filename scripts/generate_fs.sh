#!/bin/bash
#
# File: generate_fs.sh
#
# Script to pretentiously generate filesystem described by fs.asm.
# It also relies on apps/app.bin being present.

# Stop on any error.
set -e

# Make sure accidentally left over files do not interfere.
rm -rf disk.hdd
rm -rf fs.fs
rm -rf apps/app.bin

# Build binary which will live in filesystem.
make -C apps s
if [ ! -f apps/app.bin ];
then
    echo No app.bin found. Exiting
    exit
fi

# Get the size of the one file that will be in the filesystem. The 
# filesystem needs this information.
APP_SIZE=$(stat apps/app.bin | grep Size | cut -d' ' -f4)

# Build the filesystem. The assembly file (fs.asm) has been crafted to put the right
# bytes in the right places so that the resulting binary is a valid fs, as defined
# by fnode.h.
cp fs.asm tmp_fs.asm
echo "
dd ${APP_SIZE}
times 1536 - ($ - \$$) db 0" >> tmp_fs.asm
nasm -f bin -o fs.fs tmp_fs.asm
cat fs.fs apps/app.bin > full_fs.fs

# Create the disk and put the file system at the correct location in the disk.
# 262144 bytes are skipped in the write to the disk because that is how many bytes
# are needed to hold the metadata for the fs on a 16G disk. If we change the disk
# to be a different size, we should recalculate this value.
fallocate -l 16G disk.hdd
dd if=full_fs.fs of=disk.hdd obs=512 seek=262144 conv=notrunc

echo "Displaying fs content:"
hexdump -s 134217728 -n 2048 -C disk.hdd

# Clean up
rm -rf tmp_fs.asm full_fs.fs fs.fs
