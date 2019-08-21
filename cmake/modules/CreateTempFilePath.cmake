define_property(GLOBAL PROPERTY "CreateTempFile_counter" BRIEF_DOCS "counter for CreateTempFile"
        FULL_DOCS "counter for CreateTempFile")

function(CreateTempFilePath)
    set(options)
    set(oneValueArgs OUTPUT_VAR)
    set(multiValueArgs)
    cmake_parse_arguments(TEMP "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

    get_property(counter GLOBAL PROPERTY CreateTempFile_counter)
    if(NOT ${counter})
        set(counter 0)
    endif()

    math(EXPR counter "${counter} + 1")
    set_property(GLOBAL PROPERTY CreateTempFile_counter ${counter})

    set(${TEMP_OUTPUT_VAR} "${CMAKE_BINARY_DIR}/temp${counter}" PARENT_SCOPE)
endfunction()