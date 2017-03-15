#!/usr/bin/env bash
set -e

CACHE_DIR=$TRAVIS_BUILD_DIR/apt_mingw_cache/$1
mkdir -p $CACHE_DIR

pushd $CACHE_DIR

apt-get --print-uris --yes install $MINGW_PACKAGES | grep ^\' | cut -d\' -f2 > downloads.list
wget -N --input-file downloads.list

sudo cp ./*.deb /var/cache/apt/archives/

popd
