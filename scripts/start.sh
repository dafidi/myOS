#!/bin/bash
#
# File: start.sh
# 
# This ought to be run in a Linux shell.

qemu_cmd=qemu-system-x86_64
# qemu_cmd=/home/david/Downloads/qemu-7.2.0/build/qemu-system-x86_64

image=myOS.img

for arg in "$@"
do
    if [[ $arg == "mode=32" ]]; then
       qemu_cmd=qemu-system-i386
       image=myOS32.img
    else
        echo " I don't know what to do about [$arg]."
    fi
done


# Convenience script to start the VM.
${qemu_cmd} \
    -D /tmp/qemu.log \
    -boot c,menu=off \
    -drive file=${image},format=raw,if=ide,media=disk,index=0 \
    -drive file=disk.hdd,format=raw,if=ide,media=disk,index=1 \
    -m 8G -display curses -gdb tcp::1236 \
    -d int -no-reboot\
    -S
