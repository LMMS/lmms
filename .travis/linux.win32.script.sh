#!/usr/bin/env bash

export CMAKE_OPTS="-DUSE_WERROR=ON -DUSE_CCACHE=ON"
../cmake/build_mingw32.sh
