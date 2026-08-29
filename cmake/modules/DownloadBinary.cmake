# Downloads an executable from the provided URL for use in a build system
# and optionally prepends it to the PATH
#
# Assumes:
# - CMAKE_CURRENT_BINARY_DIR/[${_name}]
# - CPACK_CURRENT_BINARY_DIR/[${_name}]
# - Fallback to $ENV{TMPDIR}/[RANDOM]/[${_name}]
# - For verbose, set COMMAND_ECHO to STDOUT in calling script
#
macro(download_binary RESULT_VARIABLE _url _name _prepend_to_path)
	if(NOT COMMAND_ECHO OR "${COMMAND_ECHO}" STREQUAL "NONE")
		set(_command_echo NONE)
		set(_output_quiet OUTPUT_QUIET)
		set(_error_quiet ERROR_QUIET)
	else()
		set(_command_echo "${COMMAND_ECHO}")
		set(_output_quiet "")
		set(_error_quiet "")
	endif()

	# Check if fuse is needed
	if("${RESULT_VARIABLE}" MATCHES "\\.AppImage$" OR "${_name}" MATCHES "\\.AppImage$")
		message(STATUS "AppImage detected, we'll extract the AppImage before using")
		set(_${RESULT_VARIABLE}_IS_APPIMAGE TRUE)
    endif()

	# Determine a suitable working directory
	if(CMAKE_CURRENT_BINARY_DIR)
		# Assume we're called from configure step
		set(_working_dir "${CMAKE_CURRENT_BINARY_DIR}")
	elseif(CPACK_CURRENT_BINARY_DIR)
		# Assume cpack (non-standard variable name, but used throughout)
		set(_working_dir "${CPACK_CURRENT_BINARY_DIR}")
	else()
		# Fallback to somewhere temporary, writable
		if($ENV{_tmpdir})
			# POSIX
			set(_tmpdir "$ENV{_tmpdir}")
		elseif($ENV{TEMP})
			# Windows
			set(_tmpdir "$ENV{TEMP}")
		else()
			# Linux, shame on you!
			find_program(MKTEMP mktemp)
			if(MKTEMP)
				execute_process(COMMAND mktemp
					OUTPUT_VARIABLE _working_dir
					OUTPUT_STRIP_TRAILING_WHITESPACE
					${_output_quiet}
					COMMAND_ECHO ${_command_echo})
					# mktemp formats it how we want it
			else()
				# Ummm... Linux you can do better!
				set(_tmpdir "/tmp")
            endif()
		endif()
		if(NOT DEFINED _working_dir)
			string(RANDOM subdir)
			set(_working_dir "${_tmpdir}/tmp.${subdir}")
		endif()
		if(NOT EXISTS "${_working_dir}")
			file(MAKE_DIRECTORY "${_working_dir}")
		endif()
	endif()

	if(_prepend_to_path)
		# Ensure the PATH is configured
		string(FIND "$ENV{PATH}" "${_working_dir}" _pathloc)
		if(NOT $_pathloc EQUAL 0)
			set(ENV{PATH} "${_working_dir}:$ENV{PATH}")
		endif()
	endif()

	# First ensure the binary doesn't already exist
	find_program(_${RESULT_VARIABLE} "${_name}" HINTS "${_working_dir}")

	set(_binary_path "${_working_dir}/${_name}")
	if(NOT _${RESULT_VARIABLE})
		message(STATUS "Downloading ${_name} from ${_url}...")
		file(DOWNLOAD
			"${_url}"
			"${_binary_path}"
			STATUS DOWNLOAD_STATUS)
		# Check if download was successful.
		list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
		list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
		if(NOT ${STATUS_CODE} EQUAL 0)
			file(REMOVE "${_binary_path}")
			message(FATAL_ERROR "Error downloading ${_url} ${ERROR_MESSAGE}")
		endif()

		# Ensure the file is executable
		file(CHMOD "${_binary_path}" PERMISSIONS
			OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_WRITE GROUP_READ)

		# Ensure it's found
		find_program(_${RESULT_VARIABLE} "${_name}" HINTS "${_working_dir}" REQUIRED)
	endif()

	# We need to create a subdirectory for this binary and symlink it's AppRun to where it's expected
	if(_${RESULT_VARIABLE}_IS_APPIMAGE AND NOT IS_SYMLINK "${_${RESULT_VARIABLE}}")
		if(NOT COMMAND create_symlink)
			include(CreateSymlink)
		endif()

		message(STATUS "Extracting ${_${RESULT_VARIABLE}} to ${_working_dir}/.${_name}/")

		# extract appimage
		execute_process(COMMAND "${_${RESULT_VARIABLE}}" --appimage-extract
			WORKING_DIRECTORY "${_working_dir}"
			COMMAND_ECHO ${_command_echo}
			${_output_quiet}
			${_error_quiet}
			COMMAND_ERROR_IS_FATAL ANY)

		# move extracted files to dedicated location (e.g. ".linuxdeploy-x86_64.AppImage/squashfs-root/")
		file(MAKE_DIRECTORY "${_working_dir}/.${_name}/")
		file(RENAME "${_working_dir}/squashfs-root/" "${_working_dir}/.${_name}/squashfs-root/")

		# remove the unusable binary
		file(REMOVE "${_${RESULT_VARIABLE}}")

		# symlink the expected binary name to the AppRun file
		message(STATUS "Creating a symbolic link ${_${RESULT_VARIABLE}} which points to ${_working_dir}/.${_name}/squashfs-root/AppRun")
		create_symlink("${_working_dir}/.${_name}/squashfs-root/AppRun" "${_${RESULT_VARIABLE}}")
	endif()

	# Test the binary
	# - TODO: Add support for bad binaries that set "$?" to an error code for no good reason
	# - TODO: Add support for Windows binaries expecting "/?" instead of "--help"
	message(STATUS "Testing that ${_name} works on this system...")
	set(_test_param "--help")

	execute_process(COMMAND "${_${RESULT_VARIABLE}}" ${_test_param}
		COMMAND_ECHO ${_command_echo}
		${_output_quiet}
		${_error_quiet}
		COMMAND_ERROR_IS_FATAL ANY)

	message(STATUS "The binary \"${_${RESULT_VARIABLE}}\" is now available")
	set(${RESULT_VARIABLE} "${_${RESULT_VARIABLE}}")
endmacro()