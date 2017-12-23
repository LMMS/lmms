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
    rm -rf PACKAGE_URL_TEMP
done
