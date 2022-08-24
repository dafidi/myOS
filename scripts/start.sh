#!/bin/bash
#
# File: start.sh
# 
# This ought to be run in a Linux shell.

# Convenience script to start the VM.
qemu-system-i386 \
	-D /tmp/qemu.log \
	-boot c,menu=off \
	-drive file=myOS.img,format=raw,if=ide,media=disk,index=0 \
	-drive file=disk.hdd,format=raw,if=ide,media=disk,index=1 \
	-m 4G -curses -gdb tcp::1236 \
	-S
