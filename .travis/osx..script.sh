#!/usr/bin/env bash

set -e

if [ "$QT5" ]; then
        # Workaround; No FindQt5.cmake module exists
        CMAKE_PREFIX_PATH="$(brew --prefix qt55)"
        export CMAKE_PREFIX_PATH
fi

# shellcheck disable=SC2086
cmake $CMAKE_FLAGS -DUSE_WERROR=OFF ..
