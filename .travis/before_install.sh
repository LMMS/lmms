#!/usr/bin/env bash

if [ "$TYPE" != 'style' ]; then
  # shellcheck disable=SC1090
  . "$TRAVIS_BUILD_DIR/.travis/$TRAVIS_OS_NAME.$TARGET_OS.before_install.sh"
fi
