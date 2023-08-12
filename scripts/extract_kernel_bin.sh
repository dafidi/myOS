#!/bin/bash
#
# File: extract_kernel_bin.sh
#

# Stop on any error.
set -e
set -x

# This should be "mode=32" or "mode=64"
mode=$1

if [[ $mode != "mode=64" && $mode != "mode=32" ]];
then
    echo "mode arg must be mode=64 or mode=32"
    exit 0;
fi

elf_file=""
bin_file=""

if [[ $mode == "mode=64" ]];
then
    elf_file="kernel64.elf"
    bin_file="kernel64.bin"
else
    elf_file="kernel32.elf"
    bin_file="kernel32.bin"
fi

uname=`uname -a`

READELF="/usr/local/opt/llvm/bin/llvm-readelf"

extract_linux() {
    objcopy -O binary $elf_file $bin_file
}

# These are the sections that typicallye exist:
# __TEXT,__text WANT
# __TEXT,__eh_frame DON'T WANT
# __TEXT,__const WANT
# __TEXT,__cstring WANT
# __DATA,__const WANT
# __DATA,__bss DON'T WANT
# __DATA,__common DON'T WANT
# __DATA,__data WANT
# __BSS,__bss DON'T WANT (empty anyways)

max_kernel_occupied_address=0

# NOTE: mac only.
add_section() {
    local name=$1
    local address=$2
    local offset=$3
    local size=$4

    if [[ $((address + size)) -gt max_kernel_occupied_address ]];
    then
        max_kernel_occupied_address=$((address + size));
    fi

    if [ "${name}" = '__bss' ]; then
        echo "(bss) returning for $name"
        return;
    fi
    if [[ "$name" = "__eh_frame" ]]
    then
        echo "(eh_frame) returning for $name"
        return;
    fi
    if [[ "$name" = "__common" ]]
    then
        echo " (common) returning for $name"
        return;
    fi

    iseek_blocks=$((offset))
    # iseek_blocks=$((iseek_bloxks / 512))
    # oseek_blocks=$((address - 0x9000))
    oseek_blocks=$((address - 0x100000))
    # oseek_blocks=$((oseek_blocks / 512))
    echo iseek=${iseek_blocks} oseek=${oseek_blocks}
    dd iseek=${iseek_blocks} oseek=${oseek_blocks} if=${elf_file} of=${bin_file} bs=1 count=$((size)) conv=notrunc
}

# Based on Manual/Anectdotal inspection of the mahco elf these are the sections 
# we want to copy. Skip everything else.
extract_mac() {
    section_names=`${READELF} --section-headers ${elf_file} | grep -E "( Name)" | awk -F'[:(]' '{ print $2 }'`
    section_segments=`${READELF} --section-headers ${elf_file} | grep -E "( Segment)" | awk -F'[:(]' '{ print $2 }'`
    section_addresses=`${READELF} --section-headers ${elf_file} | grep -E "( Address)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`
    section_offsets=`${READELF} --section-headers ${elf_file} | grep -E "( Offset)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`
    section_sizes=`${READELF} --section-headers ${elf_file} | grep -E "( Size)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`

    #convert to arrays.
    section_names=($section_names)
    section_segments=($section_segments)
    section_addresses=($section_addresses)
    section_offsets=($section_offsets)
    section_sizes=($section_sizes)

    num_sections=${#section_names[@]}
    count=0

    while [ $count -lt $num_sections ]
    do
        echo ${section_names[$count]};
        name=${section_names[$count]}
        address=${section_addresses[$count]}
        offset=${section_offsets[$count]}
        size=${section_sizes[$count]}

        add_section $name $address $offset $size

        count=$((count + 1))
    done
}

# extract_windows() {
#     #TODO: may never do.
# }

extract_mac
