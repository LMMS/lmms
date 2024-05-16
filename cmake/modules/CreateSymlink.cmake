# Offer symlink support via "cmake -E create_symlink"
# For verbose, set COMMAND_ECHO to STDOUT in calling script
macro(create_symlink filepath sympath)
	if(NOT DEFINED COMMAND_ECHO)
		set(_COMMAND_ECHO STDOUT)
	else()
		set(_COMMAND_ECHO "${COMMAND_ECHO}")
	endif()
	execute_process(COMMAND ${CPACK_CMAKE_COMMAND} -E create_symlink "${filepath}" "${sympath}" COMMAND_ECHO ${_COMMAND_ECHO})
endmacro()