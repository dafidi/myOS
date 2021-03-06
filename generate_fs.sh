#!/bin/bash
#
# Script to pretentiously generate filesystem described by fs.asm.
# It also relies on apps/app.bin being present.
#

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
APP_SIZE=$(stat apps/app.bin | grep Size | cut -d' ' -f4)

cp fs.asm tmp_fs.asm
echo "
dd ${APP_SIZE}
times 1536 - ($ - \$$) db 0" >> tmp_fs.asm

nasm -f bin -o fs.fs tmp_fs.asm
cat fs.fs apps/app.bin > full_fs.fs

fallocate -l 16G disk.hdd
dd if=full_fs.fs of=disk.hdd obs=512 seek=262144 conv=notrunc

hexdump -s 134217728 -n 2048 -C disk.hdd

rm -rf tmp_fs.asm
