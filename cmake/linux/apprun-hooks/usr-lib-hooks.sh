#!/usr/bin/env bash

# Workaround libraries being incorrectly placed in usr/lib (e.g. instead of usr/lib/lmms, etc)
# FIXME: Remove when linuxdeploy supports subfolders https://github.com/linuxdeploy/linuxdeploy/issues/305
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
export LMMS_PLUGIN_DIR="$DIR/usr/lib/"
export LADSPA_PATH="$DIR/usr/lib/"
export SUIL_MODULE_DIR="$DIR/usr/lib/"
