#!/bin/bash

set -e
sudo echo "thanks for admin privileges"


#	get_repo()
# clone (if necessary) or update a repo
#	$1	repo url
#	$2	install steps (quoted)
get_repo()
{
	pushd ~

	# https://github.com/siriobalmelli/libjudy.git -> libjudy
	NAME=${1##*/}; NAME=${NAME%%.*}
	if [ ! -d $NAME ]; then
		git clone $1
		cd $NAME
	else
		cd $NAME
		git fetch
		git reset --hard origin/master
	fi

	# execute whatever install steps asked for
	bash -c "$2"
	popd
}


# platform dependencies
DEPS=( \
	autoconf \
	build-essential \
	libtool \
	liburcu-dev \
	libyaml-dev \
	ninja-build \
	pkg-config \
	python3-pip \
)
sudo apt-get update
sudo apt-get install -y ${DEPS[@]}
sudo pip3 install meson


# dependencies from source
get_repo https://github.com/siriobalmelli/libjudy.git \
	"./configure \
	&& make \
	&& make check \
	&& sudo make install"

get_repo https://github.com/siriobalmelli/nonlibc.git \
	"meson --buildtype=release build_release \
	&& cd build_release \
	&& ninja \
	&& sudo ninja install"


# our package
# assume we are in xdpacket toplevel dir already
meson --buildtype=release build_release
cd build_release
ninja
sudo ninja install
