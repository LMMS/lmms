#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Accomodate both linux windows mingw locations
if [ -z "$ARCH" ]; then
	ARCH=32
fi

if [ "$ARCH" == "32" ]; then
        MINGW=/usr/i686-w64-mingw32
else
        MINGW=/usr/x86_64-w64-mingw32
fi
if [ -z "$MSYSCON" ]; then
	MINGW=/opt$MINGW

	DISTRO=$(lsb_release -si)
	DISTRO_VERSION=$(lsb_release -sr)

	if [ "$DISTRO" != "Ubuntu" ]; then
		echo "This script only supports Ubuntu"
		exit 1
	fi

	if [ "$DISTRO_VERSION" == "14.04" ]; then
		TOOLCHAIN="$DIR/toolchains/Ubuntu-MinGW-X-Trusty-$ARCH.cmake"
	else
		TOOLCHAIN="$DIR/toolchains/Ubuntu-MinGW-W64-$ARCH.cmake"
	fi
else
	CMAKE_OPTS="$CMAKE_OPTS -DLMMS_BUILD_MSYS=1"
fi

export PATH=$MINGW/bin:$PATH
export CXXFLAGS="$CFLAGS"
if [ "$ARCH" == "32" ]; then
	export CFLAGS="-march=pentium3 -mtune=generic -mpreferred-stack-boundary=5 -mfpmath=sse"
fi

CMAKE_OPTS="-DCMAKE_PREFIX_PATH=$MINGW -DMINGW_PREFIX=$MINGW $CMAKE_OPTS"

# shellcheck disable=SC2086
cmake "$DIR/.." -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_MODULE_PATH="$DIR/../cmake/modules/" $CMAKE_OPTS
