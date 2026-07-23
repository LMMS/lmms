SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
SET(CMAKE_DEBUG_POSTFIX "")

# Enable C++20
SET(CMAKE_CXX_STANDARD 20)

IF(LMMS_BUILD_APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ENDIF()

include_directories("${CMAKE_BINARY_DIR}/src")

IF("MidiImport" IN_LIST PLUGIN_LIST OR "MidiExport" IN_LIST PLUGIN_LIST)
	ADD_LIBRARY(PortSMF STATIC
		MidiImport/portsmf/include/allegro.h
		MidiImport/portsmf/include/mfmidi.h
		MidiImport/portsmf/include/strparse.h
		MidiImport/portsmf/src/allegro.cpp
		MidiImport/portsmf/src/allegrord.cpp
		MidiImport/portsmf/src/allegrosmfrd.cpp
		MidiImport/portsmf/src/allegrosmfwr.cpp
		MidiImport/portsmf/src/mfmidi.cpp
		MidiImport/portsmf/src/strparse.cpp
	)
	TARGET_INCLUDE_DIRECTORIES(PortSMF PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/MidiImport/portsmf/include")
	SET_TARGET_PROPERTIES(PortSMF PROPERTIES POSITION_INDEPENDENT_CODE ON)

	IF(NOT MSVC)
		# PortSMF raises these warnings, which will prevent compilation with -Werror.
		TARGET_COMPILE_OPTIONS(PortSMF PRIVATE
			-Wno-error=unknown-pragmas
			-Wno-error=maybe-uninitialized
			-Wno-error=restrict)
	ENDIF()
ENDIF()

# See cmake/modules/PluginList.cmake
FOREACH(PLUGIN ${PLUGIN_LIST})
	ADD_SUBDIRECTORY(${PLUGIN})
ENDFOREACH()
