# Detects the best place to install bash completion support
# - Windows is not supported
# - TODO: macOS support can be provided through Homebrew
IF(NOT LMMS_BUILD_WIN32 AND NOT LMMS_BUILD_APPLE)

	# standard install location for most distributions
	SET(BASHCOMP_FALLBACK "/usr/share/bash-completion/completions")

	# check for common portable installation (CMAKE_INSTALL_PREFIX=../target)
	GET_FILENAME_COMPONENT(BINARY_ADJ_INSTALL "${CMAKE_BINARY_DIR}/../target" ABSOLUTE)
	IF("${BINARY_ADJ_INSTALL}" STREQUAL "${CMAKE_INSTALL_PREFIX}")
		SET(SKIP_PKG_CHECK TRUE)
	ENDIF()

	# check for other signs of user-level install
	SET(USER_LEVEL_INSTALL "/home/;/root/;/Users/")
	FOREACH(_startswith ${USER_LEVEL_INSTALL})
		IF("${CMAKE_INSTALL_PREFIX}" MATCHES "^${_startswith}")
			SET(SKIP_PKG_CHECK TRUE)
			BREAK()
		ENDIF()
	ENDFOREACH()

	# determine BASHCOMP_PKG_PATH variable
	IF(SKIP_PKG_CHECK)
		SET(BASHCOMP_PKG_PATH "${CMAKE_INSTALL_PREFIX}${BASHCOMP_FALLBACK}")
	ELSE()
		PKG_CHECK_MODULES(BASH_COMPLETION bash-completion)
		PKG_GET_VARIABLE(BASHCOMP_PKG_PATH bash-completion completionsdir)
	ENDIF()

	# final fallback for non-user-level installs
	IF("${BASHCOMP_PKG_PATH}" STREQUAL "")
		# only fallback to whitelisted install locations
		SET(ROOT_LEVEL_INSTALL "/usr/;/usr/local/;/opt/")
		FOREACH(_startswith ${ROOT_LEVEL_INSTALL})
			IF("${CMAKE_INSTALL_PREFIX}" MATCHES "^${_startswith}")
				SET(BASHCOMP_PKG_PATH "${BASHCOMP_FALLBACK}")
				BREAK()
			ENDIF()
		ENDFOREACH()
	ENDIF()
	
	IF("${BASHCOMP_PKG_PATH}" STREQUAL "")
		MESSAGE("Unable to determine BASHCOMP_PKG_PATH.  Install manually from completions/lmms if needed.")
	ELSE()
		MESSAGE(STATUS "Bash completions will be installed to ${BASHCOMP_PKG_PATH}")
		INSTALL(FILES lmms DESTINATION "${BASHCOMP_PKG_PATH}" PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
	ENDIF()
ENDIF()
