option(USE_COMPILE_CACHE "Use a compiler cache for compilation" OFF)

# Compatibility for old option name
if(USE_CCACHE)
	set(USE_COMPILE_CACHE ON)
endif()

if(NOT USE_COMPILE_CACHE)
	return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "(GNU|AppleClang|Clang|MSVC)")
	message(WARNING "Compiler cache only available with MSVC or GNU")
	return()
endif()

set(CACHE_TOOL_NAME ccache)
find_program(CACHE_TOOL "${CACHE_TOOL_NAME}")
if(NOT CACHE_TOOL)
	message(WARNING "USE_COMPILE_CACHE enabled, but no ${CACHE_TOOL_NAME} found")
	return()
endif()

if(MSVC)
	# ccache doesn't support debug information in the PDB format. Setting the
	# debug information format requires CMP0141, introduced with CMake 3.25, to
	# be set to NEW prior to the initial `project` command.
	if(CMAKE_VERSION VERSION_LESS "3.25")
		message(WARNING "Use of compiler cache with MSVC requires at least CMake 3.25")
		return()
	endif()

	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

message(STATUS "Using ${CACHE_TOOL} for compiler caching")

# TODO CMake 3.21: Use CMAKE_<LANG>_<COMPILER|LINKER>_LAUNCHER variables instead
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CACHE_TOOL}")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CACHE_TOOL}")
