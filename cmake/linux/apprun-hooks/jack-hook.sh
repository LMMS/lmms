#!/usr/bin/env bash

# Workaround crash when jack is missing by providing a dummy version
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
ME="$( basename "${BASH_SOURCE[0]}")"
# Set language to English
export LC_ALL=C
if ldd "$DIR/usr/bin/lmms" |grep "libjack.so" |grep "not found" > /dev/null 2>&1; then
	echo "[$ME] Jack does not appear to be installed.  That's OK, we'll use a dummy version instead." >&2
	export LD_LIBRARY_PATH="$DIR/usr/lib/jack:$LD_LIBRARY_PATH"
else
	echo "[$ME] Jack appears to be installed on this system, so we'll use it." >&2
fi
# Restore language
unset LC_ALL
