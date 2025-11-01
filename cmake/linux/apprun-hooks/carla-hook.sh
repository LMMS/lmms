#!/usr/bin/env bash

# Workaround nuances with carla being an optional-yet-hard-linked plugin
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
ME="$( basename "${BASH_SOURCE[0]}")"
CARLA_LIB_NAME="libcarla_native-plugin.so"
KNOWN_LOCATIONS=("lib" "lib64")
unset CARLA_LIB_FILE

# Check for carla at "known" locations
if command -v carla > /dev/null 2>&1; then
	CARLA_PATH="$(command -v carla)"
	CARLA_PREFIX="${CARLA_PATH%/bin*}"

	# Look for libcarla_native-plugin.so in adjacent lib directory
	for lib in "${KNOWN_LOCATIONS[@]}"; do
		if [ -e "$CARLA_PREFIX/$lib/carla/$CARLA_LIB_NAME" ]; then
			# Add directory to LD_LIBRARY_PATH so libcarlabase.so can find it
			CARLA_LIB_FILE="$CARLA_PREFIX/$lib/carla/$CARLA_LIB_NAME"
			export LD_LIBRARY_PATH="$CARLA_PREFIX/$lib/carla/:$LD_LIBRARY_PATH"
			echo "[$ME] Carla appears to be installed on this system at $CARLA_PREFIX/$lib/carla so we'll use it." >&2
			break
		fi
	done
else
	echo "[$ME] Carla does not appear to be installed, we'll remove it from the plugin listing." >&2
	export "LMMS_EXCLUDE_PLUGINS=libcarla,${LMMS_EXCLUDE_PLUGINS}"
fi

# Additional workarounds for library conflicts
# libgobject has been versioned "2.0" for over 20 years, but the ABI is constantly changing
KNOWN_CONFLICTS=("libgobject-2.0.so.0")
if [ -n "$CARLA_LIB_FILE" ]; then
	for conflict in "${KNOWN_CONFLICTS[@]}"; do
		# Only prepend LD_PRELOAD if we bundle the same version
		if [ -e "$DIR/usr/lib/$conflict" ]; then
			conflict_sys="$(ldd "$CARLA_LIB_FILE" | grep "$conflict" | awk '{print $3}')"
			if [ -e "$conflict_sys" ]; then
				# Add library to LD_PRELOAD so lmms can find it over its bundled version
				echo "[$ME] Preferring the system's \"$conflict\" over the version bundled." >&2
				export LD_PRELOAD="$conflict_sys:$LD_PRELOAD"
			fi
		fi
	done
fi
