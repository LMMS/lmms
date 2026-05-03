# Note:
# 	The last version of Calf that was LADSPA-capable is version 0.0.18.2

set_directory_properties(PROPERTIES SYSTEM TRUE)

# Parse version info from autoconf
FILE(READ veal/configure.ac VERSION_FILE)
STRING(REPLACE "[" ";" VERSION_FILE ${VERSION_FILE} )
STRING(REPLACE "]" ";" VERSION_FILE ${VERSION_FILE} )
LIST(GET VERSION_FILE 2 VERSION)
CONFIGURE_FILE(config.h.in config.h)

FILE(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/veal/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/veal/src/*.c")
LIST(SORT SOURCES)

# Skip files matching pattern
SET(FILE_PATTERNS "ctl;gui;gtk;session;connector;jack;rdf;draw;fluid;preset;lv2;benchmark;win;plugin.cpp")
FOREACH(_item ${SOURCES})
	GET_FILENAME_COMPONENT(m_basename ${_item} NAME)
	FOREACH(_pattern ${FILE_PATTERNS})
		IF(${m_basename} MATCHES ${_pattern})
			LIST(REMOVE_ITEM SOURCES ${_item})
		ENDIF()
	ENDFOREACH()
ENDFOREACH()

ADD_LIBRARY(veal MODULE ${SOURCES})
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include"
                    "${CMAKE_BINARY_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/veal/src")
		
INSTALL(TARGETS veal LIBRARY DESTINATION "${PLUGIN_DIR}/ladspa")
SET_TARGET_PROPERTIES(veal PROPERTIES PREFIX "")

# Disable OSC messaging, it's not mingw compatible
TARGET_COMPILE_DEFINITIONS(veal PRIVATE DISABLE_OSC=1)

SET(INLINE_FLAGS "")
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	SET(INLINE_FLAGS -finline-functions-called-once -finline-limit=80)
ENDIF()

if(NOT MSVC)
	target_compile_options(veal PRIVATE -fexceptions -O2 -finline-functions ${INLINE_FLAGS})
endif()

if(MSVC)
	target_link_options(veal PRIVATE "/EXPORT:ladspa_descriptor")
endif()

if(LMMS_BUILD_WIN32)
	add_custom_command(
		TARGET veal
		POST_BUILD
		COMMAND "${STRIP_COMMAND}" "$<TARGET_FILE:veal>"
		VERBATIM
		COMMAND_EXPAND_LISTS
	)
endif()
IF(NOT LMMS_BUILD_APPLE AND NOT LMMS_BUILD_OPENBSD)
	target_link_libraries(veal PRIVATE -shared)
ENDIF()
