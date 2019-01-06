#!/usr/bin/env bash

set -e

if [ "$TYPE" = 'style' ]; then

	# SC2185 is disabled because of: https://github.com/koalaman/shellcheck/issues/942
	# once it's fixed, it should be enabled again
	# shellcheck disable=SC2185
	# shellcheck disable=SC2046
	shellcheck $(find -O3 "$TRAVIS_BUILD_DIR/.travis/" "$TRAVIS_BUILD_DIR/cmake/" -type f -name '*.sh' -o -name "*.sh.in")

else

	mkdir -p build
	cd build

	export CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=RelWithDebInfo"

	if [ -z "$TRAVIS_TAG" ]; then
		export CMAKE_FLAGS="$CMAKE_FLAGS -DUSE_CCACHE=ON"
	fi

	"$TRAVIS_BUILD_DIR/.travis/$TRAVIS_OS_NAME.$TARGET_OS.script.sh"

	make -j4
	make tests

	if [[ $TARGET_OS != win* ]]; then
		tests/tests
	fi

	# Package and upload non-tagged builds
	if [ ! -z "$TRAVIS_TAG" ]; then
		# Skip, handled by travis deploy instead
		exit 0
	elif [[ $TARGET_OS == win* ]]; then
		make -j4 package
		PACKAGE="$(ls lmms-*win*.exe)"
	elif [[ $TRAVIS_OS_NAME == osx ]]; then
		make -j4 install > /dev/null
		make dmg
		PACKAGE="$(ls lmms-*.dmg)"
	else
		make -j4 install > /dev/null
		make appimage
		PACKAGE="$(ls lmms-*.AppImage)"
	fi

	echo "Uploading $PACKAGE to transfer.sh..."
	curl --upload-file "$PACKAGE" "https://transfer.sh/$PACKAGE" || true
fi
