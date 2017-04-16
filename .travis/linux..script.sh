#!/usr/bin/env bash

# shellcheck disable=SC2086
cmake -DUSE_WERROR=ON $CMAKE_FLAGS -DFIX_SYSTEM_HEADERS=ON ..
