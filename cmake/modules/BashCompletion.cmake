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
# * "lmms" subfolder ensures we don't pollute /usr/local/share/ on default "make install"
set(BASHCOMP_USER_PATH "share/${PROJECT_NAME}/bash-completion/completions")

macro(BASHCOMP_INSTALL SCRIPT_NAME)
	# Note:  When running from CPack, message(...) will be suppressed unless WARNING
	if(WIN32)
		message(STATUS "Bash completion is not supported on this platform.")
	else()
	    # Install a copy of bash completion to the default install prefix
	    # See also: https://github.com/LMMS/lmms/pull/7252/files#r1815749125
		install(FILES "${SCRIPT_NAME}" DESTINATION "${BASHCOMP_USER_PATH}")

		# Next, blindly attempt a system-wide install, ignoring failure
		# See also: https://stackoverflow.com/q/58448332
		# * CPack doesn't use CMAKE_INSTALL_PREFIX, so the original will be missing when packaging
		#   and this step will be skipped
		# * For non-root installs (e.g. ../target), this will silently fail
		set(BASHCOMP_ORIG "${CMAKE_INSTALL_PREFIX}/${BASHCOMP_USER_PATH}/${CMAKE_PROJECT_NAME}")
		set(BASHCOMP_LINK "${BASHCOMP_PKG_PATH}/${CMAKE_PROJECT_NAME}")

		if(BASHCOMP_PKG_PATH)
			# TODO: CMake 3.21 Use "file(COPY_FILE ...)"
			install(CODE "
				if(EXISTS \"${BASHCOMP_ORIG}\")
					file(REMOVE \"${BASHCOMP_LINK}\")
					execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
						\"${BASHCOMP_ORIG}\"
						\"${BASHCOMP_LINK}\"
						ERROR_QUIET
						RESULT_VARIABLE result)
					if(result EQUAL 0)
						message(STATUS \"Bash completion-support has been installed to ${BASHCOMP_LINK}\")
					endif()
				endif()
			")
		endif()
	endif()
endmacro()



