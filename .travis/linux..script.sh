#!/usr/bin/env bash
if [ $QT5 ]; then
	unset QTDIR QT_PLUGIN_PATH LD_LIBRARY_PATH
	source /opt/qt59/bin/qt59-env.sh
fi

cmake -DUSE_WERROR=ON $CMAKE_FLAGS ..
