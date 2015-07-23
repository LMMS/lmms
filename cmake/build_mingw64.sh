#!/bin/sh

# Accomodate both linux windows mingw locations
MINGW=/mingw64
if [ -z "$MSYSCON" ]; then
	MINGW=/opt$MINGW
else
	CMAKE_OPTS="$CMAKE_OPTS -DLMMS_BUILD_MSYS=1"
fi

export PATH=$PATH:$MINGW/bin

if [ "$1" = "-qt5" ] ; then
	CMAKE_OPTS="-DWANT_QT5=ON -DCMAKE_PREFIX_PATH=$MINGW $CMAKE_OPTS"
fi

cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/modules/Win64Toolchain.cmake -DCMAKE_MODULE_PATH=`pwd`/../cmake/modules/ $CMAKE_OPTS

