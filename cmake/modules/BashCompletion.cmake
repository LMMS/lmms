# A wrapper around pkg-config-provided and cmake-provided bash completion that
# will have dynamic behavior at INSTALL() time to allow both root-level
# INSTALL() as well as user-level INSTALL().
#
# See also https://github.com/scop/bash-completion
#
# Copyright (c) 2018, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Usage:
#    INCLUDE(BashCompletion)
#    BASHCOMP_INSTALL(foo)
#    ... where "foo" is a shell script adjacent to the CMakeLists.txt
#
# How it determines BASHCOMP_PKG_PATH, in order:
#    1. Uses BASHCOMP_PKG_PATH if already set (e.g. -DBASHCOMP_PKG_PATH=...)
#       a. If not, uses pkg-config's PKG_CHECK_MODULES to determine path
#       b. Fallback to cmake's FIND_PACKAGE(bash-completion) path
#       c. Fallback to hard-coded /usr/share/bash-completion/completions
#    2. Final fallback to ${CMAKE_INSTALL_PREFIX}/share/bash-completion/completions if
#       detected path is unwritable.

# - Windows does not support bash completion
# - macOS support should eventually be added for Homebrew (TODO)
IF(WIN32)
	MESSAGE(STATUS "Bash competion is not supported on this platform.")
ELSEIF(APPLE)
	MESSAGE(STATUS "Bash completion is not yet implemented for this platform.")
ELSE()
	INCLUDE(FindUnixCommands)
	# Honor manual override if provided
	IF(NOT BASHCOMP_PKG_PATH)
		# First, use pkg-config, which is the most reliable
		FIND_PACKAGE(PkgConfig QUIET)
		IF(PKGCONFIG_FOUND)
			PKG_CHECK_MODULES(BASH_COMPLETION bash-completion)
			PKG_GET_VARIABLE(BASHCOMP_PKG_PATH bash-completion completionsdir)
		ELSE()
			# Second, use cmake (preferred but less common)
			FIND_PACKAGE(bash-completion QUIET)
			IF(BASH_COMPLETION_FOUND)
				SET(BASHCOMP_PKG_PATH "${BASH_COMPLETION_COMPLETIONSDIR}")
			ENDIF()
		ENDIF()

		# Third, use a hard-coded fallback value
		IF("${BASHCOMP_PKG_PATH}" STREQUAL "")
			SET(BASHCOMP_PKG_PATH "/usr/share/bash-completion/completions")
		ENDIF()
	ENDIF()

	# Always provide a fallback for non-root INSTALL()
	SET(BASHCOMP_USER_PATH "${CMAKE_INSTALL_PREFIX}/share/bash-completion/completions")

	# Cmake doesn't allow easy use of conditional logic at INSTALL() time
	# this is a problem because ${BASHCOMP_PKG_PATH} may not be writable and we
	# need sane fallback behavior for bundled INSTALL() (e.g. .AppImage, etc).
	#
	# The reason this can't be detected by cmake is that it's fairly common to
	# run "cmake" as a one user (i.e. non-root) and "make install" as another user
	# (i.e. root).
	#
	# - Creates a script called "install_${SCRIPT_NAME}_completion.sh" into the
	#   working binary directory and invokes this script at install.
	# - Script handles INSTALL()-time conditional logic for sane ballback behavior
	#   when ${BASHCOMP_PKG_PATH} is unwritable (i.e. non-root); Something cmake
	#   can't handle on its own at INSTALL() time)
	MACRO(BASHCOMP_INSTALL SCRIPT_NAME)
		# A shell script for wrapping conditionl logic
		SET(BASHCOMP_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_${SCRIPT_NAME}_completion.sh")

		FILE(WRITE ${BASHCOMP_SCRIPT} "\
#!${BASH}\n\
set -e\n\
if [ -w \"${BASHCOMP_PKG_PATH}\" ]; then\n\
  BASHCOMP_PKG_PATH=\"${BASHCOMP_PKG_PATH}\"\n\
else \n\
  BASHCOMP_PKG_PATH=\"\$DESTDIR${BASHCOMP_USER_PATH}\"\n\
fi\n\
echo -e \"\\nInstalling bash completion...\\n\"\n\
mkdir -p \"\$BASHCOMP_PKG_PATH\"\n\
cp \"${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT_NAME}\" \"\$BASHCOMP_PKG_PATH\"\n\
chmod a+r \"\$BASHCOMP_PKG_PATH/${SCRIPT_NAME}\"\n\
echo -e \"Bash completion for ${SCRIPT_NAME} has been installed to \$BASHCOMP_PKG_PATH/${SCRIPT_NAME}\"\n\
")
		INSTALL(CODE "EXECUTE_PROCESS(COMMAND chmod u+x \"install_${SCRIPT_NAME}_completion.sh\" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )")
		INSTALL(CODE "EXECUTE_PROCESS(COMMAND \"./install_${SCRIPT_NAME}_completion.sh\" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )")

		MESSAGE(STATUS "Bash completion script for ${SCRIPT_NAME} will be installed to ${BASHCOMP_PKG_PATH} or fallback to ${BASHCOMP_USER_PATH} if unwritable.")
	ENDMACRO()
ENDIF()

