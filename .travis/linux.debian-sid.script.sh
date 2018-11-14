#!/bin/sh
set -e

BASETGZ="$HOME/pbuilder-bases/debian-sid-amd64.tgz"
MIRROR=http://cdn-fastly.deb.debian.org/debian
KEYRING=/usr/share/keyrings/debian-archive-keyring.gpg

if [ ! -e "$BASETGZ.stamp" ]
then
	mkdir -p "$HOME/pbuilder-bases"
	sudo pbuilder --create --basetgz "$BASETGZ" --mirror $MIRROR \
		--distribution sid --architecture amd64 \
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
env -i sudo pbuilder --build --basetgz "$BASETGZ" *.dsc
