cmake_minimum_required(VERSION 3.8)
project(RemoteVstPlugin 
	LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

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
		"${LMMS_SOURCE_DIR}/plugins/vst_base/common"
		"${LMMS_SOURCE_DIR}/include"
		"${LMMS_BINARY_DIR}"
)

target_compile_definitions(${EXE_NAME} PRIVATE BUILD_REMOTE_PLUGIN_CLIENT)

# Workaround for missing WinMain
if(MSVC)
	set_property(TARGET ${EXE_NAME}
	APPEND
	PROPERTY LINK_FLAGS "/entry:mainCRTStartup"
)
endif()


if(WIN32)
	find_package(Qt5Core REQUIRED)
	target_link_libraries(${EXE_NAME} Qt5::Core)
endif()

if(IS_MINGW)
	SET(CMAKE_REQUIRED_FLAGS "-std=c++17")

	CHECK_CXX_SOURCE_COMPILES("
	#include <mutex>
	int main(int argc, const char* argv[]) {
		std::mutex m;
		return 0;
	}
	" HAS_STD_MUTEX)

	if(NOT HAS_STD_MUTEX)
		target_include_directories(${EXE_NAME} PRIVATE
			"${LMMS_SOURCE_DIR}/src/3rdparty/mingw-std-threads")
		target_compile_definitions(${EXE_NAME} PRIVATE
			-DUSE_MINGW_THREADS_REPLACEMENT)
	endif()
endif()

if(LMMS_BUILD_WIN32)
	if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
		set(NOOP_COMMAND "${CMAKE_COMMAND}" "-E" "true")
	else()
		set(NOOP_COMMAND "${CMAKE_COMMAND}" "-E" "echo")
	endif()
	if(STRIP)
		set(STRIP_COMMAND "$<IF:$<TARGET:Debug,RelWithDebInfo>,${NOOP_COMMAND},${STRIP}>")
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
