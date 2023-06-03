#!/bin/bash
#
# File: start.sh
# 
# This ought to be run in a Mac shell.

# Convenience script to start the VM.
qemu_cmd=qemu-system-x86_64

image=myOS.img
ram=4G

for arg in "$@"
do
    if [[ $arg == "mode=32" ]]; then
       qemu_cmd=qemu-system-i386
       image=myOS32.img
       ram=4G
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
    -m ${ram} -display curses -gdb tcp::1236 \
    -d int -no-reboot\
    -S