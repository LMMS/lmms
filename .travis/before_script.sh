#!/usr/bin/env bash

if [ "$TYPE" != 'style' ]; then
  mkdir build && cd build || exit

  export CMAKE_FLAGS="-DWANT_QT5=$QT5 -DUSE_WERROR=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo"

  if [ -z "$TRAVIS_TAG" ]; then export CMAKE_FLAGS="$CMAKE_FLAGS -DUSE_CCACHE=ON"; fi
fi
