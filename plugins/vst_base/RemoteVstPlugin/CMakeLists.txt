cmake_minimum_required(VERSION 3.1)
project(RemoteVstPlugin 
	LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

include(CheckCXXPreprocessor)
include(CheckCXXSourceCompiles)

CHECK_CXX_DEFINE(IS_WIN "_WIN32")
CHECK_CXX_DEFINE(IS_WIN64 "_WIN64")
CHECK_CXX_DEFINE(IS_MINGW "__MINGW32__")

if(NOT IS_WIN)
	message(FATAL_ERROR "Toolchain used does not target windows.")
ENDIF()

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

set(EXE_NAME RemoteVstPlugin${BITNESS})
add_executable(${EXE_NAME} WIN32
	../RemoteVstPlugin.cpp
)

target_link_libraries(${EXE_NAME} ole32)

target_include_directories(${EXE_NAME}
	PRIVATE 
		"${LMMS_SOURCE_DIR}/plugins/vst_base/common"
		"${LMMS_SOURCE_DIR}/include"
		"${LMMS_BINARY_DIR}"
)

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
	SET(CMAKE_REQUIRED_FLAGS "-std=c++11")

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

IF(STRIP)
	ADD_CUSTOM_COMMAND(TARGET ${EXE_NAME} POST_BUILD COMMAND "${STRIP}" "$<TARGET_FILE:${EXE_NAME}>")
ENDIF()

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
