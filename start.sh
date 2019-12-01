#!/bin/bash

# Convenience script to start the VM.
qemu-system-i386 \
	-boot c,menu=off \
	-drive file=os-image,format=raw,if=ide,media=disk,index=0 \
	-drive file=null.bin,format=raw,if=ide,media=disk,index=1 \
	-m 4G
