# This functions forwards a variable to
# the install stage.
# Parameters:
#   CONTENT: Variable content.
#   NAME: Variable name.
# Options:
#   GENERATOR_EXPRESSION: Support generator expression for CONTENT.
function(DEFINE_INSTALL_VAR)
    set(options GENERATOR_EXPRESSION)
    set(oneValueArgs NAME )
    set(multiValueArgs CONTENT)
    cmake_parse_arguments(VAR "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

    # install(CODE) does not support generator expression in ver<3.14
    if(VAR_GENERATOR_EXPRESSION AND ${CMAKE_VERSION} VERSION_LESS "3.14.0")
        if(MSVC)
            message(FATAL_ERROR "Installing is not supported with msvc and cmake<3.14")
        endif()

        include(CreateTempFile)
        CreateTempFilePath(OUTPUT_VAR file_path TAG "${VAR_NAME}" CONTENT "${VAR_CONTENT}")
        install(CODE "file(READ \"${file_path}\" \"${VAR_NAME}\")")
    else()
        if(VAR_GENERATOR_EXPRESSION)
            cmake_policy(SET CMP0087 NEW)
        endif()
        install(CODE "set(\"${VAR_NAME}\" \"${VAR_CONTENT}\")")
    endif()
endfunction()
