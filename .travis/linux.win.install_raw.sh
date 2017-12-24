#!/usr/bin/env bash

while read -r PACKAGE_URL_AND_OPTS; do
    pushd "$PWD"
    IFS=',' read -ra PACKAGE_URL_AND_OPTS <<< "$PACKAGE_URL_AND_OPTS"

    PACKAGE_URL="${PACKAGE_URL_AND_OPTS[0]}"
    OPTS="${PACKAGE_URL_AND_OPTS[1]}"

    echo "Downloading $PACKAGE_URL ..."

    mkdir PACKAGE_URL_TEMP
    cd PACKAGE_URL_TEMP
    
    curl "$PACKAGE_URL" | tar xfz -
    dir_name=$(ls)
    cd "$dir_name"

    ls *
    echo "Installing package $dir_name (make ${OPTS}) ..."
    sudo make $OPTS

    popd
    rm -rf PACKAGE_URL_TEMP
done <<< "$MANUAL_PACKAGES_URLS"

