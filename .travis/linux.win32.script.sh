#!/usr/bin/env bash
set -e

mkdir build
cd build

export CMAKE_OPTS="$CMAKE_FLAGS -DUSE_WERROR=ON"
../cmake/build_win32.sh

make -j4
make tests
