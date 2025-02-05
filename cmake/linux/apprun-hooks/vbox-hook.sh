#!/usr/bin/env bash
ME="$( basename "${BASH_SOURCE[0]}")"
# Workaround crash in VirtualBox when hardware rendering is enabled
if lsmod |grep vboxguest > /dev/null 2>&1; then
   echo "[$ME] VirtualBox detected.  Forcing libgl software rendering." >&2
   export LIBGL_ALWAYS_SOFTWARE=1;
fi
