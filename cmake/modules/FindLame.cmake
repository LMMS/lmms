# - Try to find LAME
# Once done this will define
#
# Lame_FOUND - system has liblame
# Lame_INCLUDE_DIRS - the liblame include directory
# Lame_LIBRARIES - The liblame libraries
# mp3lame::mp3lame - an imported target providing lame

find_package(mp3lame CONFIG QUIET)

if(TARGET mp3lame::mp3lame)
	# Extract details for find_package_handle_standard_args
	get_target_property(Lame_LIBRARIES mp3lame::mp3lame LOCATION)
	get_target_property(Lame_INCLUDE_DIRS mp3lame::mp3lame INTERFACE_INCLUDE_DIRECTORIES)
else()
	find_path(Lame_INCLUDE_DIRS lame/lame.h)
	find_library(Lame_LIBRARIES mp3lame)

	list(APPEND Lame_DEFINITIONS HAVE_LIBMP3LAME=1)

	mark_as_advanced(Lame_INCLUDE_DIRS Lame_LIBRARIES Lame_DEFINITIONS)

	if(Lame_LIBRARIES AND Lame_INCLUDE_DIRS)
		add_library(mp3lame::mp3lame UNKNOWN IMPORTED)

		set_target_properties(mp3lame::mp3lame PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${Lame_INCLUDE_DIRS}"
			INTERFACE_COMPILE_DEFINITIONS "${Lame_DEFINITIONS}"
			IMPORTED_LOCATION "${Lame_LIBRARIES}"
		)
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lame
	REQUIRED_VARS Lame_LIBRARIES Lame_INCLUDE_DIRS
)
