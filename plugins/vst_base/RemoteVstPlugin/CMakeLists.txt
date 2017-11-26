cmake_minimum_required(VERSION 3.0.0)
project(RemoteVstPlugin 
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BITNESS 64)
else()
	set(BITNESS 32)
endif()

set(EXE_NAME RemoteVstPlugin${BITNESS})

add_executable(${EXE_NAME}
	../RemoteVstPlugin.cpp
)

target_include_directories(${EXE_NAME}
	PRIVATE 
		"${LMMS_SOURCE_DIR}/plugins/vst_base/common"
		"${LMMS_SOURCE_DIR}/include"
		"${LMMS_BINARY_DIR}"
)

if(WIN32)
	find_package(Qt5 REQUIRED COMPONENTS Core)
	target_link_libraries(${EXE_NAME} Qt5::Core)
endif()

IF(STRIP)
	ADD_CUSTOM_COMMAND(TARGET RemoteVstPlugin32 POST_BUILD COMMAND "${STRIP}" "$<TARGET_FILE:RemoteVstPlugin32>")
ENDIF()
