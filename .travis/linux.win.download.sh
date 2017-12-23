#!/usr/bin/env bash

set -e

export MANUAL_PACKAGES_URLS="https://www.libsdl.org/release/SDL2-devel-2.0.7-mingw.tar.gz,native"

CACHE_DIR=$TRAVIS_BUILD_DIR/apt_mingw_cache/$1
mkdir -p "$CACHE_DIR"

pushd "$CACHE_DIR"

for PACKAGE_URL_AND_OPTS in $MANUAL_PACKAGES_URLS; do
    pushd "$PWD"
    PACKAGE_URL_AND_OPTS=(${PACKAGE_URL_AND_OPTS//,/ })

    PACKAGE_URL="${PACKAGE_URL_AND_OPTS[0]}"
    OPTS="${PACKAGE_URL_AND_OPTS[1]}"

    echo "Downloading $PACKAGE_URL ..."

    mkdir PACKAGE_URL_TEMP
    cd PACKAGE_URL_TEMP
    
    curl "$PACKAGE_URL" | tar xfz -
    dir_name=$(ls)
    cd "$dir_name"

    echo "Installing package $dir_name ..."
    sudo make install "$OPTS"

    popd
    rm -r PACKAGE_URL_TEMP
done

# shellcheck disable=SC2086
apt-get --print-uris --yes install $MINGW_PACKAGES | grep ^\' | cut -d\' -f2 > downloads.list
wget -N --input-file downloads.list

sudo cp ./*.deb /var/cache/apt/archives/

popd
