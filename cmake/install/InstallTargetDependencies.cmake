# Like INSTALL_DEPENDENCIES but can be called from regular cmake code
# (instead of install(CODE)), takes targets instead of files,
# takes care of configuring search paths, and other platform-specific tweaks.
FUNCTION(INSTALL_TARGET_DEPENENCIES)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs TARGETS DESTINATION LIB_DIRS_SUFFIXES LIB_DIRS)
    cmake_parse_arguments(DEPS "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

    if(NOT DEPS_LIB_DIRS)
        set(DEPS_LIB_DIRS "")
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

    FOREACH(LIB_PATH ${DEPS_LIB_DIRS})
        FOREACH(suffix ${DEPS_LIB_DIRS_SUFFIXES})
            list(APPEND DEPS_LIB_DIRS "${LIB_PATH}/${suffix}")
        ENDFOREACH()
    ENDFOREACH()

    # Create the list of files using file(GENERATE)
    # generator expressions on install(CODE) are not supported on old
    # cmake versions.
    SET(DEPLOY_LIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/${DEPS_NAME}_filelist_${CMAKE_BUILD_TYPE}.txt")
    FILE(GENERATE OUTPUT "${DEPLOY_LIST_FILE}" CONTENT "${DEPLOY_TARGETS}")

    LIST(APPEND DEPS_LIB_DIRS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
    LIST(REMOVE_DUPLICATES DEPS_LIB_DIRS)

    message("${DEPS_LIB_DIRS}")

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
		FILE(READ \"${DEPLOY_LIST_FILE}\" DEPLOY_FILES)

		INSTALL_DEPENDENCIES(
			FILES \"\${DEPLOY_FILES}\"
			DESTINATION \"${DEPS_DESTINATION}\"
			LIB_DIRS \"${DEPS_LIB_DIRS}\"
			SEARCH_PATHS \"${DEPS_SEARCH_PATHS}\"
			${additional_args}
		)
	")
ENDFUNCTION()