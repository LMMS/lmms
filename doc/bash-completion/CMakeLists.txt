IF(NOT LMMS_BUILD_WIN32 AND NOT LMMS_BUILD_APPLE)

	pkg_check_modules(BASH_COMPLETION bash-completion)
	pkg_get_variable(BASHCOMP_PKG_PATH bash-completion completionsdir)
	
	GET_FILENAME_COMPONENT(BINARY_ADJ_TARGET "${CMAKE_BINARY_DIR}/../target" ABSOLUTE)
	SET(BASHCOMP_WHITELIST_PREFIX "/usr;/usr/local;${BINARY_ADJ_TARGET}")
	SET(BASHCOMP_WHITELIST_PREFIX_NOLOCAL "/usr;${BINARY_ADJ_TARGET}")
	
	SET(BASHCOMP_PKG_PATH "/etc/bash_completion.d")

	IF("${BASHCOMP_PKG_PATH}" STREQUAL "/usr/share/bash-completion/completions")

		# bash completion uses the "new" way of /usr/share/bash-completion
		# this means both /usr/share/bash-completion and
		# /usr/local/share/bash-completion are valid
		# all other paths are user paths

		# Add ../target to bash-completion whitelist for appimages
		FOREACH(_prefix ${BASHCOMP_WHITELIST_PREFIX})
		IF("${CMAKE_INSTALL_PREFIX}" MATCHES "^${_prefix}/?$")
			SET(BASHCOMP_PATH_FINAL "${CMAKE_INSTALL_PREFIX}/share/bash-completion/completions")
		ENDIF()
		ENDFOREACH()
		
	ELSEIF("${BASHCOMP_PKG_PATH}" STREQUAL "/etc/bash_completion.d")

		# bash completion uses the "old" way of /etc/bash_completion.d
		# this means we can only do an install into architecture dependent paths
		# => forbid /usr/local

		FOREACH(_prefix ${BASHCOMP_WHITELIST_PREFIX_NOLOCAL})
		IF("${CMAKE_INSTALL_PREFIX}" MATCHES "^${_prefix}/?$")
			SET(BASHCOMP_PATH_FINAL "${CMAKE_INSTALL_PREFIX}/share/bash-completion/completions")
		ENDIF()
		ENDFOREACH()

	ENDIF()

	IF("${BASHCOMP_PATH_FINAL}" STREQUAL "")
	
		# maybe it's a user install?
	
		SET(BASHCOMP_WHITELIST_STARTSWITH "/home/;/root/;/Users/")

		FOREACH(_startswith ${BASHCOMP_WHITELIST_STARTSWITH})
		IF("${CMAKE_INSTALL_PREFIX}" MATCHES  "^${_startswith}")
			SET(BASHCOMP_PATH_FINAL "${CMAKE_INSTALL_PREFIX}/share/bash-completion/completions")
		ENDIF()
		ENDFOREACH()

	ENDIF()
	
	IF("${BASHCOMP_PATH_FINAL}" STREQUAL "")
		MESSAGE("LMMS provides bash completion in completions/lmms. Please install manually if wanted.")
	ELSE()
		MESSAGE(STATUS "Will install LMMS bash completion to ${BASHCOMP_PATH_FINAL}...")
		INSTALL(FILES lmms DESTINATION "${BASHCOMP_PATH_FINAL}"
			PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
	ENDIF()

ENDIF()
