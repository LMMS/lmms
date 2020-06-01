function(CreateTempFilePath)
    set(options CONFIG_SUFFIX)
    set(oneValueArgs OUTPUT_VAR TAG)
    set(multiValueArgs CONTENT)
    cmake_parse_arguments(TEMP "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )

    # Use hash to create a unique identifier
    # for this file.
    string(SHA1 hashed_content "${TEMP_CONTENT}")

    set(file_name "${CMAKE_BINARY_DIR}/${TEMP_TAG}_${hashed_content}")
    set(${TEMP_OUTPUT_VAR} "${file_name}" PARENT_SCOPE)
    if(CONFIG_SUFFIX)
        set(file_name "${file_name}_$<CONFIG>")
    endif()

    file(GENERATE OUTPUT "${file_name}"
            CONTENT "${TEMP_CONTENT}")

endfunction()
