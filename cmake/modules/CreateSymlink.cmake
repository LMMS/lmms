# Offer relative symlink support via "cmake -E create_symlink"
# For verbose, set COMMAND_ECHO to STDOUT in calling script
macro(create_symlink filepath sympath)
	if(CMAKE_COMMAND)
		set(_cmake_command "${CMAKE_COMMAND}")
	elseif(CPACK_CMAKE_COMMAND)
		set(_cmake_command "${CPACK_CMAKE_COMMAND}")
	else()
		message(FATAL_ERROR "Sorry, can't resolve variable CMAKE_COMMAND")
	endif()

	if(NOT IS_ABSOLUTE "${sympath}")
		message(FATAL_ERROR "Sorry, this command only works with absolute paths")
	endif()

	if(NOT DEFINED COMMAND_ECHO)
		set(_command_echo NONE)
	else()
		set(_command_echo "${COMMAND_ECHO}")
	endif()

	# Calculate the relative path
	file(RELATIVE_PATH reldir "${sympath}/../" "${filepath}")
	get_filename_component(symname "${sympath}" NAME)

	# Calculate the working directory
	get_filename_component(sympath_parent "${sympath}" DIRECTORY)

	# Create the symbolic link
	execute_process(COMMAND "${_cmake_command}" -E create_symlink "${reldir}" "${symname}"
		WORKING_DIRECTORY "${sympath_parent}"
		COMMAND_ECHO ${_command_echo}
		COMMAND_ERROR_IS_FATAL ANY)
endmacro()