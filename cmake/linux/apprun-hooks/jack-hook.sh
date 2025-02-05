#!/usr/bin/env bash

# Workaround crash when jack is missing by providing a dummy version
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
ME="$( basename "${BASH_SOURCE[0]}")"
if ldconfig -p | grep libjack.so.0 > /dev/null 2>&1; then
   echo "[$ME] Jack appears to be installed on this system, so we'll use it." >&2
else
   echo "[$ME] Jack does not appear to be installed.  That's OK, we'll use a dummy version instead." >&2
   export LD_LIBRARY_PATH=$DIR/usr/lib/lmms/optional:$LD_LIBRARY_PATH
fi