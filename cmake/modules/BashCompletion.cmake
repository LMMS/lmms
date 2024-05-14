# Copyright (c) 2024, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Description:
#     Fail-safe bash-completion installation support
#     - Installs to ${CMAKE_INSTALL_PREFIX}/share/bash-completion/completions
#     - Attempts to calculate and install to system-wide location
#     - See also https://github.com/scop/bash-completion
#
# Usage:
#    INCLUDE(BashCompletion)
#    BASHCOMP_INSTALL(foo)
#    ... where "foo" is a shell script adjacent to the CMakeLists.txt

# Honor manual override if provided
if(NOT BASHCOMP_PKG_PATH)
	# First, use pkg-config, which is the most reliable
	find_package(PkgConfig QUIET)
	if(PKGCONFIG_FOUND)
		PKG_CHECK_MODULES(BASH_COMPLETION bash-completion)
		PKG_GET_VARIABLE(BASHCOMP_PKG_PATH bash-completion completionsdir)
	else()
		# Second, use cmake (preferred but less common)
		find_package(bash-completion QUIET)
		if(BASH_COMPLETION_FOUND)
			set(BASHCOMP_PKG_PATH "${BASH_COMPLETION_COMPLETIONSDIR}")
		endif()
	endif()

	# Third, use a hard-coded fallback value
	if("${BASHCOMP_PKG_PATH}" STREQUAL "")
		set(BASHCOMP_PKG_PATH "/usr/share/bash-completion/completions")
	endif()
endif()

# Always provide a fallback for non-root INSTALL()
set(BASHCOMP_USER_PATH "share/bash-completion/completions")

macro(BASHCOMP_INSTALL SCRIPT_NAME)
	# Note:  When running from CPack, message(...) will be supressed unless WARNING
	if(WIN32)
		message(STATUS "Bash completion is not supported on this platform.")
	elseif(APPLE)
		message(STATUS "Bash completion is not yet implemented for this platform.")
	else()
		install(FILES "${SCRIPT_NAME}" DESTINATION "${BASHCOMP_USER_PATH}")
		if(BASHCOMP_PKG_PATH)
			# TODO: CMake 3.21 Use "file(COPY_FILE ...)"
			install(CODE "
					execute_process(COMMAND ${CMAKE_COMMAND} -E copy \"${SCRIPT_NAME}\" \"${BASHCOMP_PKG_PATH}\" ERROR_QUIET RESULT_VARIABLE result)
					if(NOT result EQUAL 0)
						message(STATUS \"Unable to install bash-completion support system-wide: ${BASHCOMP_USER_PATH}/${SCRIPT_NAME}.  This is normal for user-space installs.\")
					else()
						message(STATUS \"Bash completion-support has been installed to ${BASHCOMP_USER_PATH}/${SCRIPT_NAME}\")
					endif()
			")
		endif()
	endif()
endmacro()



