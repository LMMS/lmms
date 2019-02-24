#!/usr/bin/env bash
set -e

mkdir build
cd build

# Workaround; No FindQt5.cmake module exists
CMAKE_PREFIX_PATH="$(brew --prefix qt5)"
export CMAKE_PREFIX_PATH

# shellcheck disable=SC2086
cmake -DUSE_WERROR=OFF -DCMAKE_INSTALL_PREFIX=../target $CMAKE_FLAGS ..

make -j4
make tests
./tests/tests
