#!/bin/sh

HAVE_GIT=`dpkg -s git 2>/dev/null | grep -c "Status: install"`

set -e

if [ $HAVE_GIT -eq 0 ]; then
	sudo apt-get install git
fi

# download RPI cross-compiler
if [ ! -d rpi-tools ]; then
	git clone --depth 1 https://github.com/raspberrypi/tools/ rpi-tools
else
	echo "Cross compiler already installed"
fi


PATH_SETUP=`grep -c "arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin" ~/.bashrc`

if [ $PATH_SETUP -eq 0 ]; then
	echo "" >> ~/.bashrc
	echo "#Add Raspberry PI cross-compiler to \$PATH" >> ~/.bashrc
	echo "PATH=\$PATH:$PWD/rpi-tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin"  >> ~/.bashrc
else
	echo "\$PATH already setup"
fi
