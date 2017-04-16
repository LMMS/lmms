#!/usr/bin/env bash

if [ "$TYPE" = 'style' ]; then
  sudo apt install astyle kwstyle shellcheck
else
  # shellcheck disable=SC1090
  . "$TRAVIS_BUILD_DIR/.travis/$TRAVIS_OS_NAME.$TARGET_OS.install.sh"
fi
