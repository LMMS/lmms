#!/usr/bin/env bash

export CMAKE_OPTS="-DUSE_WERROR=ON"


if [ $QT5 ];then
    bash ${TRAVIS_BUILD_DIR}/cmake/nsis/build_mingw_drumstick.sh 1.1.0 64
else
    bash ${TRAVIS_BUILD_DIR}/cmake/nsis/build_mingw_drumstick.sh 0.5.0 64
fi
../cmake/build_mingw64.sh
