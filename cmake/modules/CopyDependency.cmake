# Copy source_lib's dependency matching 'name_match' into specified location
# Sets variable named in relocated_lib to the destination
macro(copy_dependency source_lib name_match destination relocated_lib)
	if(NOT COMMAND_ECHO OR "${COMMAND_ECHO}" STREQUAL "NONE")
		set(_command_echo NONE)
	else()
		set(_command_echo "${COMMAND_ECHO}")
	endif()

	execute_process(COMMAND file -b --mime-type "${source_lib}" OUTPUT_VARIABLE file_type)

	set(_is_linux_lib false)
	set(_is_mac_lib false)

	if("${file_type}" MATCHES "application/x-pie-executable")
		# Linux ELF binary
		set(_is_linux_lib true)
		list(APPEND _lib_command ldd)
	elseif("${file_type}" MATCHES "application/x-mach-binary")
		# macOS Mach-O binary
		set(_is_mac_lib true)
		list(APPEND _lib_command otool -L)
	else()
		message(FATAL_ERROR "Copying dependencies for ${file_type} are not yet supported")
	endif()

	execute_process(COMMAND ${_lib_command}
		"${source_lib}"
		OUTPUT_VARIABLE raw_output
		OUTPUT_STRIP_TRAILING_WHITESPACE
		COMMAND_ECHO ${_command_echo}
		COMMAND_ERROR_IS_FATAL ANY)

    # escape periods to avoid double-escaping
    string(REPLACE "." "\\." name_match "${name_match}")

	# cli output --> list
	string(REPLACE "\n" ";" raw_list "${raw_output}")

	foreach(line ${raw_list})
		if(line MATCHES "${name_match}")
			if(_is_linux_lib)
				# Assumes format "libname.so.0 => /lib/location/libname.so.0 (0x00007f48d0b0e000)"
				string(REGEX MATCH "=> ([^\\(]+)" dummy_var "${line}")
				# Trim leading/trailing whitespace and add to our list
				string(STRIP "${CMAKE_MATCH_1}" lib)
			elseif(_is_mac_lib)
				# Assumes format "@loader_path/../Frameworks/libname-0.0.dylib (compatibility version 0.0.0, current version 0.24.26)"
				string(REGEX MATCH "^[ \t]+(.*) \\(" dummy_var "${line}")
				string(STRIP "${CMAKE_MATCH_1}" lib)
				get_filename_component(loader_path ${source_lib} DIRECTORY)
				string(REPLACE "@loader_path" "${loader_path}" resolved_lib "${lib}")
				string(REPLACE "@rpath" "${loader_path}" resolved_lib "${lib}")
				# Special handling for '@executable_path'
				if(line MATCHES "@executable_path")
					# Find the position of '/Contents/'
					string(FIND "${APP_PATH}" "/Contents/" APP_CONTENTS_POS)
					# Extract the base path up to '/Contents/'
					string(SUBSTRING "${loader_path}" 0 "${APP_CONTENTS_POS}" app_base_path)
					string(REPLACE "@executable_path" "${app_base_path}/Contents/MacOS" resolved_lib "${lib}")
				endif()
			endif()

			# Resolve any possible symlinks
			file(REAL_PATH "${lib}" libreal)
			get_filename_component(symname "${lib}" NAME)
			get_filename_component(realname "${libreal}" NAME)
			file(MAKE_DIRECTORY "${destination}")
			# Copy, but with original symlink name
			file(COPY "${libreal}" DESTINATION "${destination}")
			file(RENAME "${destination}/${realname}" "${destination}/${symname}")
			set("${relocated_lib}" "${destination}/${symname}")
			break()
		endif()
	endforeach()
endmacro()