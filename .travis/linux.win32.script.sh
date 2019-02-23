#!/usr/bin/env bash
mkdir build
cd build

export CMAKE_OPTS="$CMAKE_FLAGS -DUSE_WERROR=ON"
../cmake/build_mingw32.sh

make -j4

make package
