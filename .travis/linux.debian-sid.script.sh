#!/bin/sh
set -e

: "${TARGET_ARCH:=amd64}"

BASETGZ="$HOME/pbuilder-bases/debian-sid-$TARGET_ARCH.tgz"
MIRROR=http://cdn-fastly.deb.debian.org/debian
KEYRING=/usr/share/keyrings/debian-archive-keyring.gpg

if [ -z "$TRAVIS_TAG" ]
then
	sudo \
	sh -c "echo CCACHEDIR=$HOME/.ccache >> /etc/pbuilderrc"
fi

if [ "$CC" = clang ]
then
	sudo sh -c "echo EXTRAPACKAGES=clang >> /etc/pbuilderrc"
fi

if [ ! -e "$BASETGZ.stamp" ]
then
	mkdir -p "$HOME/pbuilder-bases"
	sudo pbuilder --create --basetgz "$BASETGZ" --mirror $MIRROR \
		--distribution sid --architecture $TARGET_ARCH \
		--debootstrapopts --variant=buildd \
		--debootstrapopts --keyring=$KEYRING \
		--debootstrapopts --include=perl
	touch "$BASETGZ.stamp"
else
	sudo pbuilder --update --basetgz "$BASETGZ"
fi

DIR="$PWD"
cd ..
dpkg-source -b "$DIR"
env -i CC="$CC" CXX="$CXX" sudo pbuilder --build --debbuildopts "--jobs=auto" \
	--basetgz "$BASETGZ" ./*.dsc
