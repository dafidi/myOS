#!/bin/bash
#
# Script to pretentiously generate filesystem described by fs.asm.
# It also relies on apps/app.bin being present.
#

rm -rf disk.hdd
rm -rf fs.fs

fallocate -l 16G disk.hdd

nasm -f bin -o fs.fs fs.asm
if [ ! -f apps/app.bin ];
then
    make -C apps s
fi
cat fs.fs apps/app.bin > full_fs.fs

dd if=full_fs.fs of=disk.hdd obs=512 seek=262144 conv=notrunc

hexdump -s 134217728 -n 2048 -C disk.hdd
