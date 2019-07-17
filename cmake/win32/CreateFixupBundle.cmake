function(expand_bundle_paths)
    set(options)
    set(oneValueArgs RESULT_NAME)
    set(multiValueArgs PATHS ROOTS)
    cmake_parse_arguments(LIB "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

    set(result "")

    foreach(root ${LIB_ROOTS})
        foreach(path ${LIB_PATHS})
            list(APPEND result "${root}/${path}")
        endforeach()
    endforeach()

    set(${LIB_RESULT_NAME} ${result} PARENT_SCOPE)
endfunction()

function(create_fixup_bundle)
    set(options)
    set(oneValueArgs BINARY_PATH SCRIPT_OUTPUT_PATH)
    set(multiValueArgs IGNORE_ITEMS LIBRARIES_PATHS LIBRARIES_ROOTS)
    cmake_parse_arguments(BUNDLE "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

    expand_bundle_paths(PATHS ${BUNDLE_LIBRARIES_PATHS} ROOTS ${BUNDLE_LIBRARIES_ROOTS}
            RESULT_NAME BUNDLE_LIBRARIES_PATHS)

    set(BUNDLE_LIBRARIES_PATHS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} ${BUNDLE_LIBRARIES_PATHS})

    configure_file(
        "${LMMS_SOURCE_DIR}/cmake/win32/FixupBundle.cmake.in"
        "${BUNDLE_SCRIPT_OUTPUT_PATH}"
        @ONLY
    )
endfunction()

macro(create_and_install_fixup_bundle)
    set(options)
    set(oneValueArgs BINARY_PATH)
    set(multiValueArgs IGNORE_ITEMS LIBRARIES_PATHS LIBRARIES_ROOTS)
    cmake_parse_arguments(BUNDLE "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

    create_fixup_bundle(
            BINARY_PATH ${BUNDLE_BINARY_PATH}
            LIBRARIES_PATHS ${BUNDLE_LIBRARIES_PATHS}
            SCRIPT_OUTPUT_PATH "${CMAKE_BINARY_DIR}/FixupBundle.cmake"
            IGNORE_ITEMS ${BUNDLE_IGNORE_ITEMS}
            LIBRARIES_ROOTS ${BUNDLE_LIBRARIES_ROOTS}
    )

    install(SCRIPT "${CMAKE_BINARY_DIR}/FixupBundle.cmake")
endmacro()