#!/usr/bin/env bash

# Accomodate both linux windows mingw locations
MINGW=/mingw32
if [ -z "$MSYSCON" ]; then
	MINGW=/opt$MINGW
else
	CMAKE_OPTS="$CMAKE_OPTS -DLMMS_BUILD_MSYS=1"
fi

export PATH=$MINGW/bin:$PATH
export CFLAGS="-march=pentium3 -mtune=generic -mpreferred-stack-boundary=5 -mfpmath=sse"
export CXXFLAGS="$CFLAGS"

CMAKE_OPTS="-DCMAKE_PREFIX_PATH=$MINGW $CMAKE_OPTS"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# shellcheck disable=SC2086
cmake "$DIR/.." -DCMAKE_TOOLCHAIN_FILE="$DIR/../cmake/modules/Win32Toolchain.cmake" -DCMAKE_MODULE_PATH="$DIR/../cmake/modules/" $CMAKE_OPTS
