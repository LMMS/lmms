# Copyright (c) 2023 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(mp3lame mp3lame::mp3lame
	LIBRARY_NAMES "mp3lame"
	INCLUDE_NAMES "lame/lame.h"
	PREFIX Lame
)

determine_version_from_source(Lame_VERSION mp3lame::mp3lame [[
	#include <iostream>
	#include <lame/lame.h>

	auto main() -> int
	{
		auto version = lame_version_t{};
		get_lame_version_numerical(&version);
		std::cout << version.major << "." << version.minor;
	}
]])

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Lame
	REQUIRED_VARS Lame_LIBRARY Lame_INCLUDE_DIRS
	VERSION_VAR Lame_VERSION
)
