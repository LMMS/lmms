#!/bin/sh

MINGW=/opt/mingw64
export PATH=$PATH:$MINGW/bin
#export CFLAGS="-fno-tree-vectorize"
export CXXFLAGS="$CFLAGS"

if [ "$1" = "-qt5" ] ; then
	CMAKE_OPTS="-DWANT_QT5=ON -DCMAKE_PREFIX_PATH=$MINGW"
fi

cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/modules/Win64Toolchain.cmake -DCMAKE_MODULE_PATH=`pwd`/../cmake/modules/ $CMAKE_OPTS

