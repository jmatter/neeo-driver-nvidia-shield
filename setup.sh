#!/bin/bash

SETUP_DIR=`pwd`

echo '---------------------'
echo 'Setting up peripheral'
echo '---------------------'
echo
echo 'Please note: this setup script is tested on a Pi 3B with attached USB BLE adapter..'
echo
echo 'Starting with setup in 5 seconds...'
echo
sleep 5

if [ ! -d "peripheral/src/assets" ]; then
	mkdir peripheral/src/assets
	sudo apt-get install git gcc libusb-1.0 pkg-config
fi

if [ ! -d "peripheral/src/assets/btstack" ]; then
	echo '- Cloning "btstack" library ...';
	git clone -b develop --single-branch --depth 1 --quiet https://github.com/bluekitchen/btstack peripheral/src/assets/btstack
	if [ $? == 1 ]; then
		echo '  - Error running GIT command, exiting!'
		exit 1
	fi
fi

if grep -q 'BTSTACK_ROOT = ../..' ./peripheral/src/assets/btstack/port/libusb/Makefile; then
	echo '- Compiling "btstack" library ...';
	cd ./peripheral/src/assets/btstack/port/libusb && make
	if [ $? == 1 ]; then
		echo '  - Error building btstack, exiting!'
		exit 1
	fi
	cd $SETUP_DIR

	# Make sure btstack can be included from our project and compilation of examples remain working.
	echo '- Patching "libusb" port Makefile to enable compilation ...';
	sed -i '2s;^;CWD:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))\n;' ./peripheral/src/assets/btstack/port/libusb/Makefile
	sed -i 's/BTSTACK_ROOT = /BTSTACK_ROOT = $(CWD)\//g' ./peripheral/src/assets/btstack/port/libusb/Makefile
fi

if [ ! -d "peripheral/src/assets/mongoose" ]; then
	echo '- Cloning "mongoose" library ...';
	git clone --single-branch --depth 1 --quiet https://github.com/cesanta/mongoose peripheral/src/assets/mongoose
	if [ $? == 1 ]; then
		echo '  - Error running GIT command, exiting!'
		exit 1
	fi
fi

echo '- (re)Compiling ...';
echo
cd ./peripheral/src && make
if [ $? == 1 ]; then
	echo '  - Error building btstack, exiting!'
	exit 1
else
	echo '  - All done!'
fi
