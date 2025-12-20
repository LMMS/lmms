#!/bin/sh

# Workaround Unity desktop menubar integration
# - Unity's menubar relocation breaks Qt's MDI window handling in Linux
# - Unity was default in Ubuntu 11.04 - 18.04
if [ "$XDG_CURRENT_DESKTOP" = "Unity" ]; then
	export QT_X11_NO_NATIVE_MENUBAR=1
fi
