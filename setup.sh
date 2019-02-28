#!/bin/bash

set -e
sudo echo "thanks for admin privileges"

# Install dependencies

# Run from $HOME directory
pushd ~

sudo apt-get update

sudo apt-get install -y libyaml-dev
sudo apt-get install -y liburcu-dev

# Install autotools for libjudy
sudo apt-get install -y autoconf
sudo apt-get install -y libtool
sudo apt-get install -y build-essential

if [ -d libjudy ]; then
	rm -rf libjudy
fi
git clone https://github.com/siriobalmelli/libjudy.git

pushd libjudy
./configure
make
make check
sudo make install

popd # libjudy

# Make sure meson is installed

if [ -d nonlibc ]; then
	rm -rf nonlibc
fi

sudo apt-get install -y pkg-config
sudo apt-get install -y python3-pip
sudo pip3 install meson
sudo apt-get install -y ninja-build

git clone https://github.com/siriobalmelli/nonlibc.git

pushd nonlibc

meson --buildtype=release build_release
pushd build_release

ninja
sudo ninja install

popd # build_release
popd # nonlibc

if [ -d xdpacket ]; then
	rm -rf xdpacket
fi

git clone https://github.com/siriobalmelli/xdpacket.git

pushd xdpacket

meson --buildtype=release build_release
pushd build_release

# This step should fail with build errors
ninja
sudo ninja install

popd # build_release
popd # xdpacket
