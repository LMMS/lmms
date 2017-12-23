#!/usr/bin/env bash

set -e

CACHE_DIR=$TRAVIS_BUILD_DIR/apt_mingw_cache/$1
mkdir -p "$CACHE_DIR"

pushd "$CACHE_DIR"

for PACKAGE_URL_AND_OPTS in $MANUAL_PACKAGES_URLS; do
    PACKAGE_URL_AND_OPTS=(${PACKAGE_URL_AND_OPTS//,/ })

    PACKAGE_URL="${PACKAGE_URL_AND_OPTS[0]}"
    OPTS="${PACKAGE_URL_AND_OPTS[1]}"

    mkdir PACKAGE_URL_TEMP
    cd PACKAGE_URL_TEMP
    
    curl "$PACKAGE_URL" | tar xfz - 
    cd *

    make install $OPTS

    rm -r PACKAGE_URL_TEMP
done

# shellcheck disable=SC2086
apt-get --print-uris --yes install $MINGW_PACKAGES | grep ^\' | cut -d\' -f2 > downloads.list
wget -N --input-file downloads.list

sudo cp ./*.deb /var/cache/apt/archives/

popd
