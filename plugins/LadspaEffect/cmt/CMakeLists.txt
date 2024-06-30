set_directory_properties(PROPERTIES SYSTEM TRUE)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include")
FILE(GLOB_RECURSE SOURCES cmt/src/*.cpp)
LIST(SORT SOURCES)
ADD_LIBRARY(cmt MODULE ${SOURCES})
INSTALL(TARGETS cmt LIBRARY DESTINATION "${PLUGIN_DIR}/ladspa")

SET_TARGET_PROPERTIES(cmt PROPERTIES PREFIX "")

if(NOT MSVC)
	target_compile_options(cmt PRIVATE -O3 -fno-strict-aliasing)
endif()

if(LMMS_BUILD_WIN32)
	add_custom_command(
		TARGET cmt
		POST_BUILD
		COMMAND "${STRIP_COMMAND}" "$<TARGET_FILE:cmt>"
		VERBATIM
		COMMAND_EXPAND_LISTS
	)
endif()

if(MSVC)
	target_link_options(cmt PRIVATE "/EXPORT:ladspa_descriptor")
endif()

if(NOT LMMS_BUILD_WIN32)
	target_compile_options(cmt PRIVATE -fPIC)
endif()

IF(NOT LMMS_BUILD_APPLE AND NOT LMMS_BUILD_OPENBSD)
	target_link_libraries(cmt PRIVATE -shared)
ENDIF()

