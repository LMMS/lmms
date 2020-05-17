#!/usr/bin/env bash

set -e

if [ "$TYPE" = 'style' ]; then

	# SC2185 is disabled because of: https://github.com/koalaman/shellcheck/issues/942
	# once it's fixed, it should be enabled again
	# shellcheck disable=SC2185
	# shellcheck disable=SC2046
	shellcheck $(find -O3 . -maxdepth 3 -type f -name '*.sh' -o -name "*.sh.in")
	shellcheck doc/bash-completion/lmms

else

	export CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=RelWithDebInfo"

	if [ -z "$TRAVIS_TAG" ]; then
		export CMAKE_FLAGS="$CMAKE_FLAGS -DUSE_CCACHE=ON"
	fi

	"$TRAVIS_BUILD_DIR/.travis/$TRAVIS_OS_NAME.$TARGET_OS.script.sh"

	# Package and upload non-tagged builds
	if [ -n "$TRAVIS_TAG" ]; then
		# Skip, handled by travis deploy instead
		exit 0
	elif [[ $TARGET_OS == win* ]]; then
		cd build
		make -j4 package
		PACKAGE="$(ls lmms-*win*.exe)"
	elif [[ $TRAVIS_OS_NAME == osx ]]; then
		cd build
		make -j4 install > /dev/null
		make dmg
		PACKAGE="$(ls lmms-*.dmg)"
	elif [[ $TARGET_OS != debian-sid ]]; then
		cd build
		make -j4 install > /dev/null
		make appimage
		PACKAGE="$(ls lmms-*.AppImage)"
	fi

	echo "Uploading $PACKAGE to transfer.sh..."
	# Limit the connection time to 3 minutes and total upload time to 5 minutes
	# Otherwise the build may hang
	curl --connect-timeout 180 --max-time 300 --upload-file "$PACKAGE" "https://transfer.sh/$PACKAGE" || true
fi
