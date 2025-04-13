set_directory_properties(PROPERTIES SYSTEM TRUE)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include")
FILE(GLOB PLUGIN_SOURCES tap-plugins/*.c)
LIST(SORT PLUGIN_SOURCES)
if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fp:fast")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fno-strict-aliasing -funroll-loops -ffast-math")
endif()
FOREACH(_item ${PLUGIN_SOURCES})
	GET_FILENAME_COMPONENT(_plugin "${_item}" NAME_WE)
	ADD_LIBRARY("${_plugin}" MODULE "${_item}")
	if(MSVC)
		target_link_options("${_plugin}" PRIVATE "/EXPORT:ladspa_descriptor")
	endif()
	# TAP pinknoise will re-init srand(); use existing seed instead
	IF("${_plugin}" MATCHES "tap_pinknoise")
		TARGET_COMPILE_DEFINITIONS("${_plugin}" PRIVATE TAP_DISABLE_SRAND=1)
	ENDIF()
	INSTALL(TARGETS "${_plugin}" LIBRARY DESTINATION "${PLUGIN_DIR}/ladspa")
	SET_TARGET_PROPERTIES("${_plugin}" PROPERTIES PREFIX "")
	if(LMMS_BUILD_WIN32)
		add_custom_command(
			TARGET "${_plugin}"
			POST_BUILD
			COMMAND "${STRIP_COMMAND}" "$<TARGET_FILE:${_plugin}>"
			VERBATIM
			COMMAND_EXPAND_LISTS
		)
	endif()
	IF(NOT MSVC)
		TARGET_LINK_LIBRARIES("${_plugin}" m)
	ENDIF()
ENDFOREACH()
