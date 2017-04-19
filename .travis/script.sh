#!/usr/bin/env bash

set -e

if [ "$TYPE" = 'style' ]; then

	# shellcheck disable=SC2046
	shellcheck $(find -O3 "$TRAVIS_BUILD_DIR/.travis/" "$TRAVIS_BUILD_DIR/cmake/" -type f -name '*.sh' -o -name "*.sh.in")

else

	mkdir build
	cd build

	export CMAKE_FLAGS="-DWANT_QT5=$QT5 -DUSE_WERROR=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo"

	if [ -z "$TRAVIS_TAG" ]; then
		export CMAKE_FLAGS="$CMAKE_FLAGS -DUSE_CCACHE=ON"
	fi

	"$TRAVIS_BUILD_DIR/.travis/$TRAVIS_OS_NAME.$TARGET_OS.script.sh"

	make -j4

	if [[ $TARGET_OS != win* ]]; then

		make tests
		tests/tests

	fi
fi
