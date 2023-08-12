#!/bin/bash
#
#  File: run.sh
#
# This ought to be run in a linux shell.

# Stop on any error.
set -e

platform="Linux"
uname=`uname -a`
if [[ $uname == *"Darwin"* ]];
then
	platform="Mac"
elif [[ $uname == *"Microsoft"* ]];
then
	platform="Windows"
fi
platform_arg=platform=${platform}

noclean=0
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
bash scripts/build.sh ${mode_arg} ${platform_arg}
echo "*************************Done building Image*************************"

echo "**************************Running VM*********************************"
if [[ $norun == 1 ]]; then
	echo "Flag \"--no-run\" passed - not running VM."
else
	./scripts/start.sh ${mode_arg} ${platform_arg}
fi
echo "****************************Done Running VM**************************"

if [[ $noclean == 1 ]]; then
    echo "Flag \"--no-clean\" passed - not cleaning up build files."
else
    echo "Cleaning up build files."
    make clean
fi

echo "Done."
