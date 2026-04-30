#!/usr/bin/env bash
# Calculate macOS-specific environment variables
# 1. Sets the value of MACOSX_DEPLOYMENT_TARGET by matching another binary on the system.
#
# Usage:
#    source macos-env.sh [package_manager] [package_name] [package_version]
#
# Example:
#    source macos-env.sh brew qt 5
#    source macos-env.sh port wget

set -e

unset package_manager package_name package_version

# Obtain the package manager
if [ -n "$1" ]; then
	# Set package manager to first parameter
	package_manager="$1"
	case "$package_manager" in
		"homebrew")
			package_manager="brew"
			;;
		"macports")
			package_manager="port"
			;;
	esac
	# Test the command
	if ! command -v "$package_manager" &> /dev/null; then
    	echo "ERROR: Command '$package_manager' was not found, aborting." 1>&2
    	exit 1
    fi
else
	# Try to guess the package manager
	if command -v brew &> /dev/null; then
		package_manager="brew"
	elif command -v port &> /dev/null; then
		package_manager="port"
	else
		echo "WARNING: A compatible package manager wasn't found, we'll search for '$package_name' on \$PATH instead" 1>&2
	fi
fi

# Obtain the package name
if [ -n "$2" ]; then
	package_name="$2"
else
	package_name="qt"
fi

# Special fallback handling for when package_name != binary_name, adjust as needed
if [ "$package_name" = "qt" ]; then
	binary_name="qmake"
elif [ "$package_name" = "fltk" ]; then
	binary_name="fluid"
else
	# Assume the binary_name is the package_name
	binary_name="$package_name"
fi

# Obtain the package version
if [ -n "$3" ]; then
	package_version="$3"
else
	if [ "$package_name" = "qt" ]; then
		# Sane fallback for qt, adjust as needed
      	package_version="5"
    fi
fi

# Format $package_name to include $package_version
if [ -n "$package_version" ]; then
	# Specify qt version in package name
	case "$package_manager" in
        "port")
        	case "$package_name" in
                "qt")
                	# Special handling for "qt5-qtbase"
                	package_name="$package_name${package_version}-${package_name}base"
                ;;
            	*)
            		package_name="$package_name$package_version"
            	;;
            esac
            ;;
        "brew" | *)
            package_name="$package_name@$package_version"
            ;;
    esac
fi

# Determine MACOSX_DEPLOYMENT_TARGET
if [ -n "$package_manager" ]; then
	# Specify qt version in package name
    	case "$package_manager" in
            "port")
				binary_path="$(port contents "$package_name" |grep "bin/${binary_name}\$" |head -1 |sed -e 's/^[[:space:]]*//')"
                ;;
            "brew" | *)
            	binary_path="$("$package_manager" --prefix "$package_name")/bin/$binary_name"
                ;;
        esac
        otool_out="$(otool -l "$binary_path")"

else
	otool_out="$(otool -l "$(which "$binary_name")")"
fi

echo "Using '$binary_name' (from $package_manager) to calculate the macOS deployment target..."

min_ver="$(echo "$otool_out" |grep "minos" || echo "$otool_out" |grep -A 2 "LC_VERSION_MIN_MACOS"|grep "version")"
MACOSX_DEPLOYMENT_TARGET="$(echo "$min_ver"|awk '{print $2}')"

# Echo the lowest target macOS version supported by this SDK
sdk_settings="$(xcrun --sdk macosx --show-sdk-path)/SDKSettings.plist"
sdk_min="$(plutil -extract SupportedTargets.macosx.MinimumDeploymentTarget raw "$sdk_settings")"
echo "- Lowest SDK supported by this environment is '$sdk_min' based on $sdk_settings"

# Echo the sane target macOS version based on another build dependency
export MACOSX_DEPLOYMENT_TARGET
echo "- Exporting 'MACOSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET' based on $binary_name"