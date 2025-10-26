# Copyright (c) 2023 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(SndFile SndFile::sndfile
	LIBRARY_NAMES "sndfile" "libsndfile" "libsndfile-1"
	INCLUDE_NAMES "sndfile.h"
	PKG_CONFIG sndfile
)

determine_version_from_source(SndFile_VERSION SndFile::sndfile [[
	#include <iostream>
	#include <string_view>
	#include <sndfile.h>

	auto main() -> int
	{
		// Version string has the format "name-version", optionally followed by "-exp"
		const auto version = std::string_view{sf_version_string()};
		const auto begin = version.find('-') + 1;
		const auto end = version.find('-', begin);
		std::cout << version.substr(begin, end - begin);
	}
]])

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(SndFile
	REQUIRED_VARS SndFile_LIBRARY SndFile_INCLUDE_DIRS
	VERSION_VAR SndFile_VERSION
)
