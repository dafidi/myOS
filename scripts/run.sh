#!/bin/bash
#
#  File: run.sh
#
# This ought to be run in a linux shell.

# Stop on any error.
set -e

cd ${HOME}/dev/myOS

noclean=0
nobuild=0
norun=0
build32=0

mode_arg="mode=64"

# Flag Parsing
for arg in "$@"
do
    if [[ $arg == "--no-clean" ]]; then
        noclean=1
    elif [[ $arg == "--no-build" ]]; then
        nobuild=1
    elif [[ $arg == "--no-run" ]]; then
        norun=1
    elif [[ $arg == "--build32" ]]; then
        build32=1
        mode_arg="mode=32"
    else
        echo " I don't know what to do about [$arg]."
    fi
done

echo "*************************Building image******************************"
bash scripts/build.sh ${mode_arg}
echo "*************************Done building Image*************************"

echo "**************************Running VM*********************************"
if [[ $norun == 0 ]]; then
    version=$(cat /proc/version)
    if [[ $version == *"Microsoft"* ]]; then
        echo "Launching Windows shell from which VM (qemu) will be launched..."
        bash -x scripts/wsl-start.sh ${mode_arg}
    else
        echo "Launching Linux start script (start.sh)."
        bash -x scripts/start.sh ${mode_arg}
    fi
else 
    echo "Flag \"--no-run\" passed - not running VM."
fi
echo "****************************Done Running VM**************************"

if [[ $noclean == 1 ]]; then
    echo "Flag \"--no-clean\" passed - not cleaning up build files."
else
    echo "Cleaning up build files."
    make clean
fi

echo "Done."
