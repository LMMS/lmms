#!/usr/bin/env bash
unset QTDIR QT_PLUGIN_PATH LD_LIBRARY_PATH
# shellcheck disable=SC1091
source /opt/qt59/bin/qt59-env.sh

set -e

mkdir build
cd build

# shellcheck disable=SC2086
cmake -DUSE_WERROR=ON -DCMAKE_INSTALL_PREFIX=../target $CMAKE_FLAGS ..
make -j4
make tests
./tests/tests
