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
        include(CreateTempFile)
        if(CMAKE_CONFIGURATION_TYPES) # in case of multi-config generators like MSVC generators
            CreateTempFilePath(OUTPUT_VAR file_path TAG "${VAR_NAME}" CONTENT "${VAR_CONTENT}" CONFIG_SUFFIX)
            install(CODE "file(READ \"${file_path}_\${CMAKE_INSTALL_CONFIG_NAME}\" \"${VAR_NAME}\")")
        else()
            CreateTempFilePath(OUTPUT_VAR file_path TAG "${VAR_NAME}" CONTENT "${VAR_CONTENT}")
            install(CODE "file(READ \"${file_path}\" \"${VAR_NAME}\")")
        endif()
    else()
        if(VAR_GENERATOR_EXPRESSION)
            cmake_policy(SET CMP0087 NEW)
        endif()
        install(CODE "set(\"${VAR_NAME}\" \"${VAR_CONTENT}\")")
    endif()
endfunction()
