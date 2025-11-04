#!/bin/sh
ME="$(basename "$0")"

# Workaround crash when jack is missing by providing a dummy version
export LC_ALL=C # Set language to English
if ldd "$APPDIR/usr/bin/lmms" |grep "libjack.so" |grep "not found" > /dev/null 2>&1; then
	echo "[$ME] Jack does not appear to be installed.  That's OK, we'll use a dummy version instead." >&2
	export LD_LIBRARY_PATH="$APPDIR/usr/lib/jack:$LD_LIBRARY_PATH"
else
	echo "[$ME] Jack appears to be installed on this system, so we'll use it." >&2
fi
unset LC_ALL # Restore language
