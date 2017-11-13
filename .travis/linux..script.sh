#!/usr/bin/env bash
if [ "$QT5" ]; then
	unset QTDIR QT_PLUGIN_PATH LD_LIBRARY_PATH
	# shellcheck disable=SC1091
	source /opt/qt58/bin/qt58-env.sh
fi

set -e

# shellcheck disable=SC2086
cmake -DUSE_WERROR=ON $CMAKE_FLAGS ..
