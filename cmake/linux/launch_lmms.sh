#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export PATH="$PATH:/sbin"
if command -v carla > /dev/null 2>&1; then
   CARLAPATH="$(command -v carla)"
   CARLAPREFIX="${CARLAPATH%/bin*}"
   echo "Carla appears to be installed on this system at $CARLAPREFIX/lib[64]/carla so we'll use it." >&2
   export LD_LIBRARY_PATH=$CARLAPREFIX/lib/carla:$CARLAPREFIX/lib64/carla:$LD_LIBRARY_PATH
else
   echo "Carla does not appear to be installed.  That's OK, please ignore any related library errors." >&2
fi
export LD_LIBRARY_PATH=$DIR/usr/lib/:$DIR/usr/lib/lmms:$LD_LIBRARY_PATH
# Prevent segfault on VirualBox
if lsmod |grep vboxguest > /dev/null 2>&1; then
   echo "VirtualBox detected.  Forcing libgl software rendering." >&2
   export LIBGL_ALWAYS_SOFTWARE=1;
fi
if ldconfig -p | grep libjack.so.0 > /dev/null 2>&1; then
   echo "Jack appears to be installed on this system, so we'll use it." >&2
else
   echo "Jack does not appear to be installed.  That's OK, we'll use a dummy version instead." >&2
   export LD_LIBRARY_PATH=$DIR/usr/lib/lmms/optional:$LD_LIBRARY_PATH
fi
QT_X11_NO_NATIVE_MENUBAR=1 "$DIR"/usr/bin/lmms.real "$@"
