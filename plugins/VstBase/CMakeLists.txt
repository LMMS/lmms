if(NOT LMMS_HAVE_VST)
	RETURN()
ENDIF()

INCLUDE(BuildPlugin)
INCLUDE(ExternalProject)

# These variables are not meant to be used normally, except packaging
SET(REMOTE_VST_PLUGIN_FILEPATH_32 "32/RemoteVstPlugin32" CACHE STRING "Relative file path to RemoteVstPlugin32")
SET(REMOTE_VST_PLUGIN_FILEPATH_64 "RemoteVstPlugin64" CACHE STRING "Relative file path to RemoteVstPlugin64")
IF(LMMS_BUILD_LINUX)
	SET(NATIVE_LINUX_REMOTE_VST_PLUGIN_FILEPATH_64 "NativeLinuxRemoteVstPlugin64"
		CACHE STRING "Relative file path to NativeLinuxRemoteVstPlugin64")
ENDIF()

ADD_SUBDIRECTORY(vstbase)

SET(EXTERNALPROJECT_ARGS
	SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/RemoteVstPlugin"
	#INSTALL_COMMAND ""
	BUILD_ALWAYS ON
	# Skip the install step.
	INSTALL_COMMAND ""
)

set(export_variables
		"LMMS_SOURCE_DIR"
		"LMMS_BINARY_DIR"
		"CMAKE_MODULE_PATH"
		"CMAKE_RUNTIME_OUTPUT_DIRECTORY"
		"CMAKE_BUILD_TYPE"
		"LMMS_BUILD_LINUX"
		"LMMS_BUILD_WIN32"
		"PLUGIN_DIR"
		"LMMS_HAVE_LIBRT"
		"USE_WERROR"
)

SET(EXTERNALPROJECT_CMAKE_ARGS
	"-DBUILD_WITH_EXTERNALPROJECT=ON"
)

macro(_export_var_to_external var_name)
	list(APPEND EXTERNALPROJECT_CMAKE_ARGS "-D${var_name}=${${var_name}}")
endmacro()
foreach(var ${export_variables})
	_export_var_to_external(${var})
endforeach()

# If we're building wine-powered RemoteVstPlugins for Linux,
# use a custom winebuild to fix ASLR-related VST crashes.
# See upstream bug report: https://bugs.winehq.org/show_bug.cgi?id=58480
if(LMMS_BUILD_LINUX AND (LMMS_HAVE_VST_32 OR LMMS_HAVE_VST_64))
	ExternalProject_Add(wine
		GIT_REPOSITORY https://github.com/tresf/wine
		GIT_TAG 1f8bb63e75baa5c9f901c8f50b4ea9dd69e0baa0
		GIT_SHALLOW ON
		SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/wine"
		SOURCE_SUBDIR tools/winebuild
		BUILD_ALWAYS ON
		BUILD_IN_SOURCE ON
		INSTALL_COMMAND ""
	)
	ExternalProject_Get_Property(wine SOURCE_DIR)
	set(CUSTOM_WINEBUILD_EXECUTABLE "${SOURCE_DIR}/tools/winebuild/winebuild")
	message(STATUS "Using custom winebuild: ${CUSTOM_WINEBUILD_EXECUTABLE}")
endif()

# build 32 bit version of RemoteVstPlugin
if(LMMS_HAVE_VST_32)
	INCLUDE("${CMAKE_CURRENT_LIST_DIR}/RemoteVstPlugin32.cmake")
ENDIF()
# build 64 bit version of RemoteVstPlugin
if(LMMS_HAVE_VST_64)
	INCLUDE("${CMAKE_CURRENT_LIST_DIR}/RemoteVstPlugin64.cmake")
ENDIF()

IF(LMMS_BUILD_LINUX)
	INCLUDE("${CMAKE_CURRENT_LIST_DIR}/NativeLinuxRemoteVstPlugin64.cmake")
ENDIF()
