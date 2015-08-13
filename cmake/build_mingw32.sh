#!/bin/sh

MINGW=/opt/mingw32
export PATH=$PATH:$MINGW/bin
#export CFLAGS="-march=pentium3 -mtune=generic -mpreferred-stack-boundary=5 -fno-tree-vectorize"
export CFLAGS="-march=pentium3 -mtune=generic -mpreferred-stack-boundary=5"
export CXXFLAGS="$CFLAGS"

if [ "$1" = "-qt5" ] ; then
	CMAKE_OPTS="-DWANT_QT5=ON -DCMAKE_PREFIX_PATH=$MINGW"
fi

cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/modules/Win32Toolchain.cmake -DCMAKE_MODULE_PATH=`pwd`/../cmake/modules/ $CMAKE_OPTS

