
FUNCTION(GET_COMPILER_SEARCH_DIR VAR)
	SET(results "")
	IF(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang|AppleClang)")
		EXECUTE_PROCESS(
			COMMAND ${CMAKE_CXX_COMPILER} --print-search-dirs
			OUTPUT_VARIABLE out
		)
		STRING(REPLACE "\n" ";" out "${out}")
		FOREACH(line ${out})
			IF(line MATCHES "^.+:")
				STRING(REPLACE " " ";" line "${line}")
				LIST(GET line 1 paths)
				# Remove "=" prefix
				STRING(REGEX REPLACE "^=" "" paths "${paths}")
				STRING(REPLACE ":" ";" paths "${paths}")
				FOREACH(path ${paths})
					LIST(APPEND results ${path})
				ENDFOREACH()
			ENDIF()
		ENDFOREACH()
	ENDIF()

	SET(paths "")
	FOREACH(result ${results})
		GET_FILENAME_COMPONENT(result ${result} REALPATH)
		IF(IS_DIRECTORY "${result}")
			LIST(APPEND paths ${result})
		ENDIF()
	ENDFOREACH()
	LIST(REMOVE_DUPLICATES paths)

	SET(${VAR} ${paths} PARENT_SCOPE)
ENDFUNCTION()

SET(PLUGIN_FILES "")
IF(LMMS_BUILD_WIN32)
	INSTALL(FILES $<TARGET_FILE:Qt5::QWindowsIntegrationPlugin> DESTINATION platforms)
ENDIF()

IF(LMMS_BUILD_WIN32 OR LMMS_INSTALL_DEPENDENCIES)
	# Install system libraries
	include(InstallRequiredSystemLibraries)

	# Collect directories to search for DLLs
	GET_FILENAME_COMPONENT(QTBIN_DIR "${QT_QMAKE_EXECUTABLE}" PATH)
	set(LIB_DIRS "${QTBIN_DIR}")

	GET_PROPERTY(PLUGINS_BUILT GLOBAL PROPERTY PLUGINS_BUILT)

	foreach(target lmms ${PLUGINS_BUILT})
		get_target_property(target_libs ${target} LINK_LIBRARIES)

		foreach(lib ${target_libs})
			if(TARGET ${lib} OR NOT IS_ABSOLUTE ${lib})
				continue()
			endif()

			if(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE)
				list(APPEND LIB_DIRS ${lib_dir})
			elseif(lib MATCHES ".(lib|dll.a)$")
				get_filename_component(lib_dir ${lib} PATH)
				if(IS_DIRECTORY ${lib_dir}/../bin)
					list(APPEND LIB_DIRS ${lib_dir}/../bin)
				endif()
				if(IS_DIRECTORY ${lib_dir}/bin)
					list(APPEND LIB_DIRS ${lib_dir}/bin)
				endif()
			endif()
		endforeach()
	endforeach()

	GET_COMPILER_SEARCH_DIR(COMPILER_SEARCH_DIRS)
	LIST(APPEND LIB_DIRS ${COMPILER_SEARCH_DIRS})

	LIST(REMOVE_DUPLICATES LIB_DIRS)

	# Collect plugin files to inspect
	FOREACH(PLUGIN ${PLUGINS_BUILT})
		LIST(APPEND DEPLOY_TARGETS "$<TARGET_FILE:${PLUGIN}>")
	ENDFOREACH()
	# Create the list of files using file(GENERATE)
	SET(DEPLOY_LIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/filelist.txt")
	FILE(GENERATE OUTPUT "${DEPLOY_LIST_FILE}" CONTENT "${DEPLOY_TARGETS}")

	IF(LMMS_BUILD_LINUX)
		FILE(DOWNLOAD "https://raw.githubusercontent.com/AppImage/AppImages/master/excludelist"
			"${CMAKE_BINARY_DIR}/excludelist")
		SET(additional_args INCLUDE_SYSTEM IGNORE_LIBS_FILE ${CMAKE_BINARY_DIR}/excludelist)
	ELSEIF(LMMS_BUILD_WIN32 AND CMAKE_CROSSCOMPILING)
		SET(additional_args IGNORE_LIBS_FILE ${CMAKE_CURRENT_LIST_DIR}/excludelist-win GP_TOOL objdump)
	ENDIF()

	IF(LMMS_BUILD_WIN32)
		SET(LMMS_DEP_DESTINATION ${BIN_DIR})
		SET(PLUGIN_DEP_DESTINATION ${BIN_DIR})
	ELSE()
		SET(LMMS_DEP_DESTINATION ${LIB_DIR})
		SET(PLUGIN_DEP_DESTINATION ${LIB_DIR})
	ENDIF()

	INSTALL(CODE "
		INCLUDE(\"${CMAKE_SOURCE_DIR}/cmake/modules/InstallDependencies.cmake\")
		# Install dependencies of lmms
		INSTALL_DEPENDENCIES(
			FILES ${BIN_DIR}/lmms${CMAKE_EXECUTABLE_SUFFIX}
			DESTINATION ${LMMS_DEP_DESTINATION}
			LIB_DIRS ${LIB_DIRS}
			${additional_args}
		)
		# Install dependencies of plugins
		FILE(READ \"${DEPLOY_LIST_FILE}\" DEPLOY_FILES)
		INSTALL_DEPENDENCIES(
			FILES \"\${DEPLOY_FILES}\"
			DESTINATION \"${PLUGIN_DEP_DESTINATION}\"
			LIB_DIRS ${LIB_DIRS} \"${BIN_DIR}\" \"${PLUGIN_DIR}\"
			SEARCH_PATHS \"${BIN_DIR}\" \"${PLUGIN_DIR}\"
			${additional_args}
		)
	")
ENDIF()

IF(LMMS_BUILD_APPLE)
	INSTALL(CODE "EXECUTE_PROCESS(COMMAND chmod u+x ${CMAKE_BINARY_DIR}/install_apple.sh)")
	INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_BINARY_DIR}/install_apple.sh)")
ENDIF()
