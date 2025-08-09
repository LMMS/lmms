# Copyright (c) 2023 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(Ogg Ogg::ogg
	LIBRARY_NAMES "ogg"
	INCLUDE_NAMES "ogg/ogg.h"
	PKG_CONFIG ogg
)

find_package_config_mode_with_fallback(Vorbis Vorbis::vorbis
	LIBRARY_NAMES "vorbis"
	INCLUDE_NAMES "vorbis/codec.h"
	PKG_CONFIG vorbis
	DEPENDS Ogg::ogg
)

find_package_config_mode_with_fallback(Vorbis Vorbis::vorbisfile
	LIBRARY_NAMES "vorbisfile"
	INCLUDE_NAMES "vorbis/vorbisfile.h"
	PKG_CONFIG vorbisfile
	DEPENDS Vorbis::vorbis
	PREFIX VorbisFile
)

find_package_config_mode_with_fallback(Vorbis Vorbis::vorbisenc
	LIBRARY_NAMES "vorbisenc"
	INCLUDE_NAMES "vorbis/vorbisenc.h"
	PKG_CONFIG vorbisenc
	DEPENDS Vorbis::vorbis
	PREFIX VorbisEnc
)

determine_version_from_source(Vorbis_VERSION Vorbis::vorbis [[
	#include <iostream>
	#include <string_view>
	#include <vorbis/codec.h>

	auto main() -> int
	{
		// Version string has the format "org name version"
		const auto version = std::string_view{vorbis_version_string()};
		const auto nameBegin = version.find(' ') + 1;
		const auto versionBegin = version.find(' ', nameBegin) + 1;
		std::cout << version.substr(versionBegin);
	}
]])

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OggVorbis
	REQUIRED_VARS
		Ogg_LIBRARY
		Ogg_INCLUDE_DIRS
		Vorbis_LIBRARY
		Vorbis_INCLUDE_DIRS
		VorbisFile_LIBRARY
		VorbisFile_INCLUDE_DIRS
		VorbisEnc_LIBRARY
		VorbisEnc_INCLUDE_DIRS
	# This only reports the Vorbis version - Ogg can have a different version,
	# so if we ever care about that, it should be split off into a different
	# find module.
	VERSION_VAR Vorbis_VERSION
)
