# Copyright (c) 2024 Kevin Zander
# Based on FindPortAudio.cmake, copyright (c) 2023 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(gig libgig::libgig
	LIBRARY_NAMES "gig"
	INCLUDE_NAMES "libgig/gig.h"
	PKG_CONFIG gig
	PREFIX Gig
)

determine_version_from_source(Gig_VERSION libgig::libgig [[
	#include <iostream>
	#include <libgig/gig.h>

	auto main() -> int
	{
		const auto version = gig::libraryVersion();
		std::cout << version;
	}
]])

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Gig
	REQUIRED_VARS Gig_LIBRARY Gig_INCLUDE_DIRS
	VERSION_VAR Gig_VERSION
)
