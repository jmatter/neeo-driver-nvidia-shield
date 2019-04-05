#!/bin/bash

SETUP_DIR=`pwd`

echo '---------------------'
echo 'Setting up peripheral'
echo '---------------------'
echo 'Please note: this script is tested on a Pi 3B with attached USB BLE adapter..'
echo

if [ ! -d "peripheral/assets" ]; then
	mkdir peripheral/assets
fi

if [ ! -d "peripheral/assets/mongoose" ]; then
	echo '- Cloning "mongoose" library ...';
	git clone --single-branch --depth 1 --quiet https://github.com/cesanta/mongoose peripheral/assets/mongoose
	if [ $? == 1 ]; then
		echo '  - Error running GIT command, exiting!'
		exit 1
	fi
fi

if [ ! -d "peripheral/assets/btstack" ]; then
	echo '- Cloning "btstack" library ...';
	git clone -b develop --single-branch --depth 1 --quiet https://github.com/bluekitchen/btstack peripheral/assets/btstack
	if [ $? == 1 ]; then
		echo '  - Error running GIT command, exiting!'
		exit 1
	fi
fi

if grep -q 'BTSTACK_ROOT = ../..' ./peripheral/assets/btstack/port/libusb/Makefile; then
	echo '- Compiling "btstack" library ...';
	cd ./peripheral/assets/btstack/port/libusb && make
	if [ $? == 1 ]; then
		echo '  - Error building btstack, exiting!'
		exit 1
	fi
	cd $SETUP_DIR

	# Make sure btstack can be included from our project and compilation of examples remain working.
	echo '- Patching "libusb" port Makefile to enable compilation ...';
	sed -i '2s;^;CWD:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))\n;' ./peripheral/assets/btstack/port/libusb/Makefile
	sed -i 's/BTSTACK_ROOT = /BTSTACK_ROOT = $(CWD)\//g' ./peripheral/assets/btstack/port/libusb/Makefile
fi

echo '- (re)Compiling "ws-ble-hid" ...';
cd ./peripheral/src && make
if [ $? == 1 ]; then
	echo '  - Error building btstack, exiting!'
	exit 1
else
	echo '  - All done!'
fi
