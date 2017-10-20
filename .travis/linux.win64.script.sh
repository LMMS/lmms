#!/usr/bin/env bash

set -e

export CMAKE_OPTS="$CMAKE_FLAGS -DUSE_WERROR=ON"
../cmake/build_mingw64.sh
