#!/bin/bash
#
# File: start.sh
#

mode=$1
platform=$2

case $platform in
	"Mac")
		echo "Launching Mac start script."
		./scripts/mac-start.sh $mode
	;;
	"Windows")
		echo "Launching Windows (WSL) start script."
		bash -x scripts/wsl-start.sh $mode
	;;
	"Linux")
		echo "Launching Linux start script."
		bash -x scripts/linux-start.sh $mode
	;;
esac
