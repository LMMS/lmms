#
function(create_fixup_bundle)
    set(options)
    set(oneValueArgs BINARY_PATH SCRIPT_OUTPUT_PATH)
    set(multiValueArgs LIBRARIES_PATHS IGNORE_ITEMS)
    cmake_parse_arguments(BUNDLE "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

      configure_file(
        "${LMMS_SOURCE_DIR}/cmake/win32/FixupBundle.cmake.in"
        "${BUNDLE_SCRIPT_OUTPUT_PATH}"
        @ONLY
    )
endfunction()

macro(create_and_install_fixup_bundle)
    set(options)
    set(oneValueArgs BINARY_PATH)
    set(multiValueArgs IGNORE_ITEMS LIBRARIES_PATHS)
    cmake_parse_arguments(BUNDLE "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

    set(BUNDLE_LIBRARIES_PATHS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} ${BUNDLE_LIBRARIES_PATHS})
    create_fixup_bundle(
            BINARY_PATH ${BUNDLE_BINARY_PATH}
            LIBRARIES_PATHS ${BUNDLE_LIBRARIES_PATHS}
            SCRIPT_OUTPUT_PATH "${CMAKE_BINARY_DIR}/FixupBundle.cmake"
            IGNORE_ITEMS ${BUNDLE_IGNORE_ITEMS}
    )

    install(SCRIPT "${CMAKE_BINARY_DIR}/FixupBundle.cmake")
endmacro()