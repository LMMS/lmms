#!/usr/bin/env bash

if [ $QT5 ]; then
        # Workaround; No FindQt5.cmake module exists
        export CMAKE_PREFIX_PATH="$(brew --prefix qt5)"
fi

cmake $CMAKE_FLAGS -DUSE_WERROR=OFF ..
