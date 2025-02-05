#!/usr/bin/env bash

# Workaround nuances with carla being an optional-yet-hard-linked plugin
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
ME="$( basename "${BASH_SOURCE[0]}")"
CARLA_LIB="libcarla_native-plugin.so"
KNOWN_LOCATIONS=("lib" "lib64")

# Check for carla at "known" locations
carla_lib_found=false
if command -v carla > /dev/null 2>&1; then
	CARLA_PATH="$(command -v carla)"
	CARLA_PREFIX="${CARLA_PATH%/bin*}"

	# Look for libcarla_native-plugin.so in adjacent lib directory
	for lib in "${KNOWN_LOCATIONS[@]}"; do
		if [ -e "$CARLA_PREFIX/$lib/carla/$CARLA_LIB" ]; then
			# Add library to LD_PRELOAD so libcarlabase.so can find it
			carla_lib_found=true
			export LD_LIBRARY_PATH="$CARLA_PREFIX/$lib/carla/:$LD_LIBRARY_PATH"
			echo "[$ME] Carla appears to be installed on this system at $CARLA_PREFIX/$lib/carla so we'll use it." >&2
			break
		fi
	done
else
	echo "[$ME] Carla does not appear to be installed.  That's OK, please ignore any related library errors." >&2
fi

# Additional workarounds for library conflicts
# libgobject has been versioned "2.0" for over 20 years, but the ABI is constantly changing
KNOWN_CONFLICTS=("libgobject-2.0.so.0")
KNOWN_LDCONFIG=("ldconfig" "/sbin/ldconfig")
if [ "$carla_lib_found" = true ]; then
	# ldconfig may be in /sbin, see https://github.com/LMMS/lmms/issues/4846
	for cmd in "${KNOWN_LDCONFIG[@]}"; do
		ldconfig_path="$cmd"
	done

	if [ -n "$ldconfig_path" ]; then
		for conflict in "${KNOWN_CONFLICTS[@]}"; do
			if command -v "$cmd" > /dev/null 2>&1; then
				# Only prepend LD_PRELOAD if we bundle the same version
				if [ -e "$DIR/usr/lib/$conflict" ]; then
					conflict_sys="$("$cmd" -p |grep "$(arch)" |grep "$conflict" |head -n 1 |awk '{print $4}')"
					if [ -e "$conflict_sys" ]; then
						# Add library to LD_PRELOAD so lmms can find it over its bundled version
						echo "[$ME] Preferring the system's \"$conflict\" over the version bundled." >&2
						export LD_PRELOAD="$conflict_sys:$LD_PRELOAD"
					fi
				fi
			fi
		done
	else
		echo "[$ME] Unable to locate ldconfig, skipping workaround for" "${KNOWN_CONFLICTS[@]}" >&2
	fi
fi