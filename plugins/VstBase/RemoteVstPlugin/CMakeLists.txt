cmake_minimum_required(VERSION 3.13)

if(POLICY "CMP0092")
	cmake_policy(SET CMP0092 NEW) # MSVC warning flags are not in CMAKE_<LANG>_FLAGS by default.
endif()

project(RemoteVstPlugin 
	LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

include(CheckCXXPreprocessor)
include(CheckCXXSourceCompiles)

CHECK_CXX_DEFINE(IS_WIN "_WIN32")
CHECK_CXX_DEFINE(IS_WIN64 "_WIN64")
CHECK_CXX_DEFINE(IS_MINGW "__MINGW32__")

if(IS_WIN64 OR CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BITNESS 64)
else()
	set(BITNESS 32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/32")
endif()

FOREACH( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
	STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	SET("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
ENDFOREACH()

# Add warning and error flags
include(ErrorFlags)

# Import of windows.h breaks min()/max()
add_definitions(-DNOMINMAX)

ADD_SUBDIRECTORY("${LMMS_SOURCE_DIR}/src/common" common)

if(NOT IS_WIN)
set(EXE_NAME NativeLinuxRemoteVstPlugin${BITNESS})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
else()
set(EXE_NAME RemoteVstPlugin${BITNESS})
endif()
add_executable(${EXE_NAME} WIN32
	../RemoteVstPlugin.cpp
	${LMMS_COMMON_SRCS}
)
if(IS_WIN)
	target_link_libraries(${EXE_NAME} ole32)
else()
	target_link_libraries(${EXE_NAME} dl X11)
endif()
if(NOT WIN32)
	target_link_libraries(${EXE_NAME} pthread)
endif()

if(LMMS_HAVE_LIBRT)
	target_link_libraries(${EXE_NAME} rt)
endif()

target_include_directories(${EXE_NAME}
	PRIVATE
		"${LMMS_SOURCE_DIR}/include"
		"${LMMS_BINARY_DIR}"
)

target_compile_definitions(${EXE_NAME} PRIVATE BUILD_REMOTE_PLUGIN_CLIENT)

# Workaround for missing WinMain
if(MSVC)
	target_link_options(${EXE_NAME} PRIVATE "/entry:mainCRTStartup")
endif()

if(IS_MINGW)
	SET(CMAKE_REQUIRED_FLAGS "-std=c++20")
endif()

if(LMMS_BUILD_WIN32)
	if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
		set(NOOP_COMMAND "${CMAKE_COMMAND}" "-E" "true")
	else()
		set(NOOP_COMMAND "${CMAKE_COMMAND}" "-E" "echo")
	endif()
	if(STRIP)
		# TODO CMake 3.19: Now that CONFIG generator expressions support testing for
		# multiple configurations, combine the OR into a single CONFIG expression.
		set(STRIP_COMMAND "$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,${NOOP_COMMAND},${STRIP}>")
	else()
		set(STRIP_COMMAND "${NOOP_COMMAND}")
	endif()
	add_custom_command(
		TARGET "${EXE_NAME}"
		POST_BUILD
		COMMAND "${STRIP_COMMAND}" "$<TARGET_FILE:${EXE_NAME}>"
		VERBATIM
		COMMAND_EXPAND_LISTS
	)
endif()

if(BITNESS EQUAL 32)
	INSTALL(TARGETS ${EXE_NAME} RUNTIME DESTINATION "${PLUGIN_DIR}/32")
else()
	INSTALL(TARGETS ${EXE_NAME} RUNTIME DESTINATION "${PLUGIN_DIR}")
endif()

if(BUILD_WITH_EXTERNALPROJECT)
	include(InstallTargetDependencies)
	INSTALL_TARGET_DEPENDENCIES(TARGETS ${EXE_NAME}
			DESTINATION "${PLUGIN_DIR}/32")
else()
	# Needed to deploy dependencies of RemoteVstPlugin
	SET_PROPERTY(GLOBAL APPEND PROPERTY PLUGINS_BUILT "${EXE_NAME}")
endif()
