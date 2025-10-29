# Copyright (c) 2023 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(portaudio portaudio
	LIBRARY_NAMES "portaudio"
	INCLUDE_NAMES "portaudio.h"
	PKG_CONFIG portaudio-2.0
	PREFIX Portaudio
)

determine_version_from_source(Portaudio_VERSION portaudio [[
	#include <iostream>
	#include "portaudio.h"

	auto main() -> int
	{
		// Version number has the format 0xMMmmpp
		const auto version = Pa_GetVersion();
		std::cout << ((version >> 16) & 0xff)
			<< "." << ((version >> 8) & 0xff)
			<< "." << ((version >> 0) & 0xff);
	}
]])

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Portaudio
	REQUIRED_VARS Portaudio_LIBRARY Portaudio_INCLUDE_DIRS
	VERSION_VAR Portaudio_VERSION
)
