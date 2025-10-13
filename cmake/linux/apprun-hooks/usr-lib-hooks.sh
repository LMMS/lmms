#!/usr/bin/env bash

# Paths for plugin systems to pick-up
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
export SUIL_MODULE_DIR="$DIR/usr/lib/suil-0/" # See also ${SUIL_MODULES_TARGET}
