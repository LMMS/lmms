#!/usr/bin/env bash

# Accomodate both linux windows mingw locations
MINGW=/mingw64
if [ -z "$MSYSCON" ]; then
	MINGW=/usr/x86_64-w64-mingw32
else
	CMAKE_OPTS="$CMAKE_OPTS -DLMMS_BUILD_MSYS=1"
	export PATH=$MINGW/bin:$PATH
fi

export PKG_CONFIG_PATH=$MINGW/lib/pkgconfig/

CMAKE_OPTS="-DWANT_QT5=True -DCMAKE_PREFIX_PATH=$MINGW $CMAKE_OPTS"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# shellcheck disable=SC2086
cmake "$DIR/.." -DCMAKE_TOOLCHAIN_FILE="$DIR/../cmake/modules/Win64Toolchain.cmake" -DCMAKE_MODULE_PATH="$DIR/../cmake/modules/" $CMAKE_OPTS
