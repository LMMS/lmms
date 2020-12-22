#!/usr/bin/env bash

set -e

if [ "$TYPE" = 'style' ]; then
	sudo apt-get -yqq update
	sudo apt-get install shellcheck
else
	"$TRAVIS_BUILD_DIR/.travis/$TRAVIS_OS_NAME.$TARGET_OS.before_install.sh"
	"$TRAVIS_BUILD_DIR/.travis/$TRAVIS_OS_NAME.$TARGET_OS.install.sh"
fi
