#!/usr/bin/env bash

sudo apt update
echo "GiteaPC installation: NOT FOUND\n"
echo "GiteaPC  will be installed... \n"
sudo apt install curl git python3 build-essential pkg-config
curl "https://git.planet-casio.com/Lephenixnoir/GiteaPC/raw/branch/master/install.sh" -o /tmp/giteapc-install.sh && bash /tmp/giteapc-install.sh
sudo apt install patch wget cmake python3-pil libusb-1.0-0-dev libsdl2-dev libudisks2-dev libglib2.0-dev libpng-dev
giteapc install Lephenixnoir/fxsdk@dev Lephenixnoir/sh-elf-binutils Lephenixnoir/sh-elf-gcc Lephenixnoir/sh-elf-gdb
giteapc install Lephenixnoir/OpenLibm Vhex-Kernel-Core/fxlibc
giteapc install Lephenixnoir/sh-elf-gcc
python3 -m pip install pillow
giteapc install Lephenixnoir/gint Lephenixnoir/libprof
echo "This is gint-ctl, a transfer of gint-ctl from https://git.planet-casio.com/Lephenixnoir/gintctl. \n"
echo "This is simply a tutorial. To build, run ```fxsdk build-<target platform>```\n"
echo "There will eventually be a GiteaPC installation tool. This will be run instead of all of these sudo apt install ______ s"
