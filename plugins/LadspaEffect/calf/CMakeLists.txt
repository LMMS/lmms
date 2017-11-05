# INCLUDE(CheckSubmodule)
# CHECK_SUBMODULE(swh/configure.ac)

# Note:
# 	The last version of Calf that was LADSPA-capable is version 0.0.18.2

# Parse version info from autoconf
FILE(READ calf/configure.ac VERSION_FILE)
STRING(REPLACE "[" ";" VERSION_FILE ${VERSION_FILE} )
STRING(REPLACE "]" ";" VERSION_FILE ${VERSION_FILE} )
LIST(GET VERSION_FILE 2 VERSION)
CONFIGURE_FILE(config.h.in config.h)

FILE(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/calf/src/*.cpp")
LIST(SORT SOURCES)

# Skip files matching pattern
SET(FILE_PATTERNS "ctl;gui;gtk;session;connector;jack;rdf;draw;fluid;dist;preset;lv2;benchmark;win")
FOREACH(_item ${SOURCES})
	FOREACH(_pattern ${FILE_PATTERNS})
		IF(${_item} MATCHES ${_pattern})
			LIST(REMOVE_ITEM SOURCES ${_item})
		ENDIF()
	ENDFOREACH()
ENDFOREACH()

ADD_LIBRARY(calf MODULE ${SOURCES})
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include"
                    "${CMAKE_BINARY_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/calf/src")
		
INSTALL(TARGETS calf LIBRARY DESTINATION "${PLUGIN_DIR}/ladspa")
SET_TARGET_PROPERTIES(calf PROPERTIES PREFIX "")
SET(INLINE_FLAGS "")
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	SET(INLINE_FLAGS "-finline-functions-called-once -finline-limit=80")
ENDIF()
SET_TARGET_PROPERTIES(calf PROPERTIES COMPILE_FLAGS "-fexceptions -O2 -finline-functions ${INLINE_FLAGS}")

# Don't strip if "Debug" or "RelWithDebInfo"
IF(LMMS_BUILD_WIN32 AND NOT CMAKE_BUILD_TYPE MATCHES "Deb")
	ADD_CUSTOM_COMMAND(TARGET calf POST_BUILD COMMAND "${STRIP}" "$<TARGET_FILE:calf>")
ENDIF()
IF(NOT LMMS_BUILD_APPLE AND NOT LMMS_BUILD_OPENBSD)
	SET_TARGET_PROPERTIES(calf PROPERTIES LINK_FLAGS "${LINK_FLAGS} -shared -Wl,-no-undefined")
ENDIF()

