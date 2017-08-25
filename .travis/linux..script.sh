#!/usr/bin/env bash

set -e

# shellcheck disable=SC2086
cmake -DUSE_WERROR=ON $CMAKE_FLAGS ..
