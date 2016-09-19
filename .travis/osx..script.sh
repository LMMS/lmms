#!/usr/bin/env bash

if [ $QT5 ]; then
        # Workaround; No FindQt5.cmake module exists
        export CMAKE_PREFIX_PATH="$(brew --prefix qt55)"
fi

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWANT_QT5=$QT5 -DUSE_WERROR=OFF ..
