# Variables must be prefixed with "CPACK_" to be visible here
set(lmms "${CPACK_PROJECT_NAME}")
set(LMMS "${CPACK_PROJECT_NAME_UCASE}")
set(ARCH "${CPACK_TARGET_ARCH}")
set(APP "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/${LMMS}.AppDir")

# Target AppImage file
set(APPIMAGE_FILE "${CPACK_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.AppImage")
set(APPIMAGE_BEFORE_RENAME "${CPACK_BINARY_DIR}/${LMMS}-${ARCH}.AppImage")

set(DESKTOP_FILE "${APP}/usr/share/applications/${lmms}.desktop")

# Toggle command echoing & verbosity
# 0 = no output, 1 = error/warning, 2 = normal, 3 = debug
if(NOT CPACK_DEBUG)
	set(VERBOSITY 1)
	set(COMMAND_ECHO NONE)
	set(OUTPUT_QUIET OUTPUT_QUIET)
else()
	set(VERBOSITY 2)
	set(COMMAND_ECHO STDOUT)
	unset(OUTPUT_QUIET)
endif()

include(DownloadBinary)
include(CreateSymlink)

# Cleanup CPack "External" json, txt files, old AppImage files
file(GLOB cleanup "${CPACK_BINARY_DIR}/${lmms}-*.json"
	"${CPACK_BINARY_DIR}/${lmms}-*.AppImage"
	"${CPACK_BINARY_DIR}/install_manifest.txt")
list(SORT cleanup)
file(REMOVE ${cleanup})

# Download linuxdeploy
download_binary(LINUXDEPLOY_BIN
	"https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
	linuxdeploy-${ARCH}.AppImage
	FALSE)

# Download linuxdeploy-plugin-qt
download_binary(LINUXDEPLOY_PLUGIN_BIN
	"https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage"
	linuxdeploy-plugin-qt-${ARCH}.AppImage
	FALSE)

message(STATUS "Creating AppDir ${APP}...")

file(REMOVE_RECURSE "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/include")
file(MAKE_DIRECTORY "${APP}/usr")

# Setup AppDir structure (/usr/bin, /usr/lib, /usr/share... etc)
file(GLOB files "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/*")
list(SORT files)
foreach(_file ${files})
	get_filename_component(_filename "${_file}" NAME)
	if(NOT _filename MATCHES ".AppDir")
		file(RENAME "${_file}" "${APP}/usr/${_filename}")
	endif()
endforeach()

# Copy Suil modules
if(CPACK_SUIL_MODULES)
	set(SUIL_MODULES_TARGET "${APP}/usr/lib/${CPACK_SUIL_MODULES_PREFIX}")
	file(MAKE_DIRECTORY "${SUIL_MODULES_TARGET}")
	file(COPY ${CPACK_SUIL_MODULES} DESTINATION "${SUIL_MODULES_TARGET}")
endif()

# Ensure project's "qmake" executable is first on the PATH
get_filename_component(QTBIN "${CPACK_QMAKE_EXECUTABLE}" DIRECTORY)
set(ENV{PATH} "${QTBIN}:$ENV{PATH}")

# Ensure "linuxdeploy.AppImage" binary is first on the PATH
set(ENV{PATH} "${CPACK_CURRENT_BINARY_DIR}:$ENV{PATH}")

# Promote finding our own libraries first
set(ENV{LD_LIBRARY_PATH} "${APP}/usr/lib/${lmms}/:${APP}/usr/lib/${lmms}/optional:$ENV{LD_LIBRARY_PATH}")

# Symlink executables so linuxdeploy can find them
set(ZYN "${APP}/usr/bin/RemoteZynAddSubFx")
create_symlink("${APP}/usr/lib/${lmms}/RemoteZynAddSubFx" "${ZYN}")

# Handle wine32 linking
set(VST32_BEFORE "${APP}/usr/lib/${lmms}/32/RemoteVstPlugin32.exe.so")
set(VST32 "${APP}/usr/bin/RemoteVstPlugin32.exe.so")
if(EXISTS "${VST32_BEFORE}")
	create_symlink("${VST32_BEFORE}" "${VST32}")
	execute_process(COMMAND ldd "${VST32}"
			OUTPUT_VARIABLE LDD_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ECHO ${COMMAND_ECHO}
			COMMAND_ERROR_IS_FATAL ANY)
	string(REPLACE "\n" ";" LDD_LIST "${LDD_OUTPUT}")
	foreach(line ${LDD_LIST})
		if(line MATCHES "libwine.so" AND line MATCHES "not found")
			set(ENV{LD_LIBRARY_PATH} "${CPACK_WINE_32_LIBRARY_DIRS}:$ENV{LD_LIBRARY_PATH}")
			message(STATUS "Prepended ${CPACK_WINE_32_LIBRARY_DIRS} to LD_LIBRARY_PATH: $ENV{LD_LIBRARY_PATH}")
			continue()
		endif()
	endforeach()
endif()

# Handle wine64 linking
set(VST64_BEFORE "${APP}/usr/lib/${lmms}/RemoteVstPlugin64.exe.so")
set(VST64 "${APP}/usr/bin/RemoteVstPlugin64.exe.so")
if(EXISTS "${VST64_BEFORE}")
	create_symlink("${VST64_BEFORE}" "${VST64}")
	execute_process(COMMAND ldd "${VST64}"
			OUTPUT_VARIABLE LDD_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ECHO ${COMMAND_ECHO}
			COMMAND_ERROR_IS_FATAL ANY)
	string(REPLACE "\n" ";" LDD_LIST "${LDD_OUTPUT}")
	foreach(line ${LDD_LIST})
		if(line MATCHES "libwine.so" AND line MATCHES "not found")
			set(ENV{LD_LIBRARY_PATH} "${CPACK_WINE_64_LIBRARY_DIRS}:$ENV{LD_LIBRARY_PATH}")
			message(STATUS "Prepended ${CPACK_WINE_64_LIBRARY_DIRS} to LD_LIBRARY_PATH: $ENV{LD_LIBRARY_PATH}")
			continue()
		endif()
	endforeach()
endif()

# Patch desktop file
file(APPEND "${DESKTOP_FILE}" "X-AppImage-Version=${CPACK_PROJECT_VERSION}\n")

# Build list of executables to inform linuxdeploy about
# e.g. -executable=foo.dylib -executable=bar.dylib
file(GLOB LIBS "${APP}/usr/lib/${lmms}/*.so")
#file(GLOB LADSPA "${APP}/usr/lib/${lmms}/ladspa/*.so")
# TODO: Both Linux and Mac have LADPSA plugins in this listing, but why?
list(APPEND LIBS "${APP}/usr/lib/lmms/ladspa/imp_1199.so")
list(APPEND LIBS "${APP}/usr/lib/lmms/ladspa/imbeq_1197.so")
list(APPEND LIBS "${APP}/usr/lib/lmms/ladspa/pitch_scale_1193.so")
list(APPEND LIBS "${APP}/usr/lib/lmms/ladspa/pitch_scale_1194.so")
list(APPEND LIBS ${LADSPA})
list(APPEND LIBS "${ZYN}")
list(APPEND LIBS "${VST32}")
list(APPEND LIBS "${VST64}")
list(APPEND LIBS ${CPACK_SUIL_MODULES})
list(SORT LIBS)

# Construct linuxdeploy parameters
foreach(_LIB IN LISTS LIBS)
	list(APPEND EXECUTABLES "-executable=${_LIB}")
endforeach()

# Call linuxdeploy
message(STATUS "Calling ${LINUXDEPLOY_BIN} --appdir \"${APP}\" ... [... executables]")
execute_process(COMMAND "${LINUXDEPLOY_BIN}"
	--appdir "${APP}"
	--icon-file "${CPACK_SOURCE_DIR}/cmake/linux/icons/scalable/apps/${lmms}.svg"
	--desktop-file "${APP}/usr/share/applications/${lmms}.desktop"
	--custom-apprun "${CPACK_SOURCE_DIR}/cmake/linux/launch_lmms.sh"
	--plugin qt
	--verbosity ${VERBOSITY}
	${OUTPUT_QUIET}
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

# Remove libraries that are normally sytem-provided
file(GLOB UNWANTED_LIBS
	"${APP}/usr/lib/libwine*"
	"${APP}/usr/lib/libcarla*"
	"${APP}/usr/lib/optional/libcarla*"
	"${APP}/usr/lib/libjack*")

list(SORT UNWANTED_LIBS)
foreach(_LIB IN LISTS UNWANTED_LIBS)
	file(REMOVE "${_LIB}")
endforeach()

# Bundle jack to avoid crash for systems without it
# See https://github.com/LMMS/lmms/pull/4186
execute_process(COMMAND ldd "${APP}/usr/bin/${lmms}"
			OUTPUT_VARIABLE LDD_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ECHO ${COMMAND_ECHO}
			COMMAND_ERROR_IS_FATAL ANY)
string(REPLACE "\n" ";" LDD_LIST "${LDD_OUTPUT}")
foreach(line ${LDD_LIST})
	if(line MATCHES "libjack\\.so")
		# Assume format "libjack.so.0 => /lib/x86_64-linux-gnu/libjack.so.0 (0x00007f48d0b0e000)"
		string(REPLACE " " ";" parts "${line}")
		list(LENGTH parts len)
		math(EXPR index "${len}-2")
		list(GET parts ${index} lib)
		# Get symlink target
		file(REAL_PATH "${lib}" libreal)
		get_filename_component(symname "${lib}" NAME)
		get_filename_component(realname "${libreal}" NAME)
		file(MAKE_DIRECTORY "${APP}/usr/lib/lmms/optional/")
		# Copy, but with original symlink name
		file(COPY "${libreal}" DESTINATION "${APP}/usr/lib/lmms/optional/")
		file(RENAME "${APP}/usr/lib/lmms/optional/${realname}" "${APP}/usr/lib/lmms/optional/${symname}")
		continue()
	endif()
endforeach()

if(CPACK_TOOL STREQUAL "makeself" OR "$ENV{CPACK_TOOL}" STREQUAL "makeself")
	find_program(MAKESELF_BIN makeself REQUIRED)

	# Create self-extracting ".run" script using makeself
	message(STATUS "Finishing the .run file using ${MAKESELF_BIN}...")
	string(REPLACE ".AppImage" ".run" RUN_FILE "${APPIMAGE_FILE}")
	configure_file(
		"${CPACK_SOURCE_DIR}/cmake/linux/makeself_setup.sh.in" "${APP}/setup.sh" @ONLY
		FILE_PERMISSIONS
			OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_WRITE GROUP_READ
			WORLD_READ)

	if(OUTPUT_QUIET)
		set(MAKESELF_QUIET "--quiet")
		set(ERROR_QUIET ERROR_QUIET)
	endif()

	# makeself.sh [args] archive_dir file_name label startup_script [script_args]
	file(REMOVE "${RUN_FILE}")
	execute_process(COMMAND makeself
		--keep-umask
	    --nox11
	    ${MAKESELF_QUIET}
		"${APP}"
		"${RUN_FILE}"
		"${LMMS} Installer"
		"./setup.sh"
		${OUTPUT_QUIET}
		COMMAND_ECHO ${COMMAND_ECHO}
		COMMAND_ERROR_IS_FATAL ANY)

	# ensure the installer can be executed as a script file
	execute_process(COMMAND "${RUN_FILE}" --help
    		${OUTPUT_QUIET}
    		${ERROR_QUIET}
    		COMMAND_ECHO ${COMMAND_ECHO}
    		COMMAND_ERROR_IS_FATAL ANY)

    message(STATUS "Installer created: ${RUN_FILE}")
else()
	# Create AppImage (default)
    # appimage plugin needs ARCH set when running in extracted form from squashfs-root / CI
    set(ENV{ARCH} "${ARCH}")
    message(STATUS "Finishing the AppImage...")
    execute_process(COMMAND "${LINUXDEPLOY_BIN}"
    	--appdir "${APP}"
    	--output appimage
    	--verbosity ${VERBOSITY}
    	${OUTPUT_QUIET}
    	COMMAND_ECHO ${COMMAND_ECHO}
    	COMMAND_ERROR_IS_FATAL ANY)

    message(STATUS "AppImage created successfully... renaming...")

    if(EXISTS "${APPIMAGE_BEFORE_RENAME}")
    	file(RENAME "${APPIMAGE_BEFORE_RENAME}" "${APPIMAGE_FILE}")
    	message(STATUS "AppImage created: ${APPIMAGE_FILE}")
    else()
    	message(FATAL_ERROR "An error occured generating the AppImage")
    endif()
endif()

