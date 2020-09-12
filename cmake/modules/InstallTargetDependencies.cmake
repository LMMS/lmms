include(DefineInstallVar)

SET(DEFAULT_SEARCH_DIRECTORIES "${BIN_DIR}" "${LIB_DIR}" "${CMAKE_FIND_ROOT_PATH}" "${CMAKE_PREFIX_PATH}")
SET(DEFAULT_SEARCH_SUFFIXES "bin" "lib" "../bin")

# Like INSTALL_DEPENDENCIES but can be called from regular cmake code
# (instead of install(CODE)), takes targets instead of files,
# takes care of configuring search paths, and other platform-specific tweaks.
# Arguments:
#	TARGETS:			list of cmake targets to install.
#	NAME:				unique string for this install.
#	DESTINATION:		directory path to install the binaries to.
#	LIB_DIRS:			list of paths for looking up dependencies.
#	LIB_DIRS_SUFFIXES:	list of possible suffixes for LIB_DIRS entries.
#	SEARCH_PATHS:			list of library search paths on runtime
#	NO_DEFAULT_PATHS:	supply this value to avoid adding DEFAULT_SEARCH_DIRECTORIES
#						to LIB_DIRS and DEFAULT_SEARCH_SUFFIXES to LIB_DIRS_SUFFIXES.
FUNCTION(INSTALL_TARGET_DEPENDENCIES)
	set(options NO_DEFAULT_PATHS)
	set(oneValueArgs NAME)
	set(multiValueArgs TARGETS DESTINATION LIB_DIRS_SUFFIXES LIB_DIRS SEARCH_PATHS)
	cmake_parse_arguments(DEPS "${options}" "${oneValueArgs}"
			"${multiValueArgs}" ${ARGN} )

	if(NOT DEPS_LIB_DIRS)
		set(DEPS_LIB_DIRS "")
	endif()

	# Set default values.
	if(NOT DEPS_NO_DEFAULT_PATHS)
		list(APPEND DEPS_LIB_DIRS ${DEFAULT_SEARCH_DIRECTORIES})
		set(DEPS_LIB_DIRS_SUFFIXES "${DEPS_LIB_DIRS_SUFFIXES}" ${DEFAULT_SEARCH_SUFFIXES})
	endif()

	FOREACH(TARGET ${DEPS_TARGETS})
		IF(NOT TARGET ${TARGET})
			message(FATAL_ERROR "Not a target: ${TARGET}")
		ENDIF()

		# Collect target output files.
		LIST(APPEND DEPLOY_TARGETS "$<TARGET_FILE:${TARGET}>")

		# Collect target link directories
		get_target_property(target_libs ${TARGET} LINK_LIBRARIES)

		foreach(lib ${target_libs})
			if(TARGET ${lib} OR NOT IS_ABSOLUTE ${lib})
				continue()
			endif()

			get_filename_component(lib_dir ${lib} PATH)
			list(APPEND DEPS_LIB_DIRS ${lib_dir})
		endforeach()
	ENDFOREACH()

	LIST(APPEND DEPS_LIB_DIRS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})

	FOREACH(LIB_PATH ${DEPS_LIB_DIRS})
		FOREACH(suffix ${DEPS_LIB_DIRS_SUFFIXES})
			list(APPEND DEPS_LIB_DIRS "${LIB_PATH}/${suffix}")
		ENDFOREACH()
	ENDFOREACH()

	DEFINE_INSTALL_VAR(NAME "DEPLOY_FILES" CONTENT "${DEPLOY_TARGETS}" GENERATOR_EXPRESSION)

	LIST(REMOVE_DUPLICATES DEPS_LIB_DIRS)

	IF(LMMS_BUILD_LINUX)
		FILE(DOWNLOAD "https://raw.githubusercontent.com/AppImage/AppImages/master/excludelist"
				"${CMAKE_BINARY_DIR}/excludelist")
		SET(additional_args INCLUDE_SYSTEM IGNORE_LIBS_FILE ${CMAKE_BINARY_DIR}/excludelist)
	ELSEIF(LMMS_BUILD_WIN32)
		SET(additional_args IGNORE_CASE IGNORE_LIBS_FILE "${LMMS_SOURCE_DIR}/cmake/install/excludelist-win")
		IF(CMAKE_CROSSCOMPILING)
			SET(additional_args "${additional_args}" GP_TOOL objdump)
		ENDIF()
	ENDIF()

	INSTALL(CODE "
		INCLUDE(\"${LMMS_SOURCE_DIR}/cmake/modules/InstallDependencies.cmake\")

		INSTALL_DEPENDENCIES(
			FILES \"\${DEPLOY_FILES}\"
			DESTINATION \"${DEPS_DESTINATION}\"
			LIB_DIRS \"${DEPS_LIB_DIRS}\"
			SEARCH_PATHS \"${DEPS_SEARCH_PATHS}\"
			${additional_args}
		)
	")
ENDFUNCTION()
