#!/usr/bin/env bash

# Accomodate both linux windows mingw locations
MINGW=/mingw64
if [ -z "$MSYSCON" ]; then
	MINGW=/opt$MINGW
else
	CMAKE_OPTS="$CMAKE_OPTS -DLMMS_BUILD_MSYS=1"
fi

export PATH=$MINGW/bin:$PATH

if [ "$1" = "-qt5" ]; then
        QT5=True
fi

if [ $QT5 ]; then
        CMAKE_OPTS="-DWANT_QT5=$QT5 -DCMAKE_PREFIX_PATH=$MINGW $CMAKE_OPTS"
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cmake $DIR/.. -DCMAKE_TOOLCHAIN_FILE=$DIR/../cmake/modules/Win64Toolchain.cmake -DCMAKE_MODULE_PATH=$DIR/../cmake/modules/ $CMAKE_OPTS
