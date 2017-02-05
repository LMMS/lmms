#!/usr/bin/env bash

if [ $QT5 ]; then
        # Workaround; No FindQt5.cmake module exists
        export CMAKE_PREFIX_PATH="$(brew --prefix qt55)"
fi

if [ $QT5 ];then
    bash ${TRAVIS_BUILD_DIR}/cmake/apple/build_osx_drumstick.sh 1.1.0
else

    bash ${TRAVIS_BUILD_DIR}/cmake/apple/build_osx_drumstick.sh 0.5.0
fi

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWANT_QT5=$QT5 -DUSE_WERROR=OFF ..
