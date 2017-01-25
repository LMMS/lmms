#!/usr/bin/env bash

export CMAKE_OPTS="-DUSE_WERROR=ON"

# Just a temporarily solution.
if [ $QT5 ];then
    bash ${TRAVIS_BUILD_DIR}/cmake/nsis/build_mingw_drumstick.sh 1.1.0 32
else
    bash ${TRAVIS_BUILD_DIR}/cmake/nsis/build_mingw_drumstick.sh 0.5.0 32
fi

../cmake/build_mingw32.sh
