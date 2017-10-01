#!/usr/bin/env bash

set -e

if [ "$TYPE" = 'style' ]; then

	# SC2185 is disabled because of: https://github.com/koalaman/shellcheck/issues/942
	# once it's fixed, it should be enabled again
	# shellcheck disable=SC2185
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
