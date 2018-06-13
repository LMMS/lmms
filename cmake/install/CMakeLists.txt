IF(LMMS_BUILD_WIN32)
	# Install system libraries
	if(MINGW)
	    GET_FILENAME_COMPONENT(mingw_path ${CMAKE_CXX_COMPILER} PATH)
	ENDIF()
	include(InstallRequiredSystemLibraries)

	# Collect directories to search for DLLs
	GET_FILENAME_COMPONENT(QTBIN_DIR "${QT_QMAKE_EXECUTABLE}" PATH)
	set(LIB_DIRS "${QTBIN_DIR}")
	foreach(lib ${LMMS_REQUIRED_LIBS})
		if(TARGET ${lib})
			continue()
		endif()
		if(MSVC AND lib MATCHES ".lib$")
			get_filename_component(lib_dir ${lib} PATH)
			list(APPEND LIB_DIRS  ${lib_dir}/../bin)
		endif()
	endforeach()
	LIST(REMOVE_DUPLICATES LIB_DIRS)

	# Collect plugin files to inspect
	SET(PLUGIN_FILES "")
	GET_PROPERTY(PLUGINS_BUILT GLOBAL PROPERTY PLUGINS_BUILT)
	FOREACH(PLUGIN ${PLUGINS_BUILT})
		LIST(APPEND PLUGIN_FILES "${PLUGIN_DIR}/${CMAKE_SHARED_MODULE_PREFIX}${PLUGIN}${CMAKE_SHARED_MODULE_SUFFIX}")
	ENDFOREACH()

	INSTALL(CODE "
		INCLUDE(\"${CMAKE_SOURCE_DIR}/cmake/modules/InstallDependencies.cmake\")
		# Install dependencies of lmms.exe
		INSTALL_DEPENDENCIES(
			FILES ${BIN_DIR}/lmms.exe
			DESTINATION ${BIN_DIR}
			LIB_DIRS ${LIB_DIRS}
		)
		# Install dependencies of plugins
		INSTALL_DEPENDENCIES(
			FILES ${PLUGIN_FILES}
			DESTINATION \"${PLUGIN_DIR}\"
			LIB_DIRS ${LIB_DIRS} \"${BIN_DIR}\" \"${PLUGIN_DIR}\"
			SEARCH_PATHS \"${BIN_DIR}\" \"${PLUGIN_DIR}\")
	")

	RETURN()
ENDIF()

IF(LMMS_BUILD_APPLE)
	INSTALL(CODE "EXECUTE_PROCESS(COMMAND chmod u+x ${CMAKE_BINARY_DIR}/install_apple.sh)")
	INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_BINARY_DIR}/install_apple.sh)")
ENDIF()
