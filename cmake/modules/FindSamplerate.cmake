# Copyright (c) 2023 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(SampleRate SampleRate::samplerate
	LIBRARY_NAMES "samplerate" "libsamplerate" "libsamplerate-0"
	INCLUDE_NAMES "samplerate.h"
	PKG_CONFIG samplerate
	PREFIX Samplerate
)

determine_version_from_source(Samplerate_VERSION SampleRate::samplerate [[
	#include <iostream>
	#include <string_view>
	#include <samplerate.h>

	auto main() -> int
	{
		// Version string has the format "name-version copyright"
		const auto version = std::string_view{src_get_version()};
		const auto begin = version.find('-') + 1;
		const auto end = version.find(' ', begin);
		std::cout << version.substr(begin, end - begin);
	}
]])

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Samplerate
	REQUIRED_VARS Samplerate_LIBRARY Samplerate_INCLUDE_DIRS
	VERSION_VAR Samplerate_VERSION
)
