#!/bin/sh

# Configure QPlatform Abstraction (qpa) to prefer X-Protocol C-Bindings (xcb) over Wayland
if [ -n "$QT_QPA_PLATFORM" ]; then
	echo "[${0##*/}] QT_QPA_PLATFORM=\"$QT_QPA_PLATFORM\" was provided, using." >&2
else
	export QT_QPA_PLATFORM="xcb"
	echo "[${0##*/}] Defaulting to QT_QPA_PLATFORM=\"$QT_QPA_PLATFORM\" for compatibility purposes." >&2
	echo "[${0##*/}] To force wayland, set QT_QPA_PLATFORM=\"wayland\" or call using \"-platform wayland\"." >&2
fi
