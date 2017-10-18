#!/usr/bin/env bash

set -e

docker build -t lmms-build ${TRAVIS_BUILD_DIR}/.travis/linux.cross.win64/

