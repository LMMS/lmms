#!/usr/bin/env bash

if [ $QT5 ];then
    bash ${TRAVIS_BUILD_DIR}/cmake/linux/build_linux_drumstick.sh 1.1.0 
fi

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_WERROR=ON -DWANT_QT5=$QT5 ..
