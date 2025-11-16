#!/usr/bin/env bash

# Configure QPlatform Abstraction (qpa) to prefer X-Protocol C-Bindings (xcb) over Wayland 
ME="$( basename "${BASH_SOURCE[0]}")"

if [ -n "$QT_QPA_PLATFORM" ]; then
	echo "[$ME] QT_QPA_PLATFORM=\"$QT_QPA_PLATFORM\" was provided, using." >&2
else
	export QT_QPA_PLATFORM="xcb"
	echo "[$ME] Defaulting to QT_QPA_PLATFORM=\"$QT_QPA_PLATFORM\" for compatibility purposes." >&2
	echo "[$ME] To force wayland, set QT_QPA_PLATFORM=\"wayland\" or call using \"-platform wayland\"." >&2
fi
