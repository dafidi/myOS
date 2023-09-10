#!/bin/bash
#
# File: start.sh
#

PLATFORM=`uname -a | cut -d" " -f1`
if [[ $PLATFORM == "Linux" ]]
then
	bash -x scripts/linux-start.sh $mode
elif [[ $PLATFORM == "Darwin" ]]
then
	./scripts/mac-start.sh $mode
else
	bash -x scripts/wsl-start.sh $mode
fi

