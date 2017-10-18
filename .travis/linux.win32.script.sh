#!/usr/bin/env bash

set -e

export CMAKE_OPTS="$CMAKE_FLAGS -DUSE_WERROR=ON"
docker run -v ${TRAVIS_BUILD_DIR}:/lmms:rw lmms-build /bin/sh /lmms/.travis/linux.cross.win32/script.sh

