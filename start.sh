#!/bin/bash

# Convenience script to start the VM.
# qemu-system-i386 -boot c \
# -drive file=os-image,\
#        format=raw,\
#        if=ide,\
#        media=disk\
# -m 4G
qemu-system-i386 -boot c,menu=off -drive file=os-image,format=raw,if=ide,media=disk,index=0 -m 4G
