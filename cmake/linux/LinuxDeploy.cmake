# Create a Linux desktop installer using linuxdeploy
#  * Creates a relocatable LMMS.AppDir installation in build/_CPack_Packages using linuxdeploy
#    * If CPACK_TOOL=appimagetool or is not set, bundles AppDir into redistributable ".AppImage" file
#    * If CPACK_TOOL=makeself is provided, bundles into a redistributable ".run" file
#
# Copyright (c) 2025, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Variables must be prefixed with "CPACK_" to be visible here
set(lmms "${CPACK_PROJECT_NAME}")
set(LMMS "${CPACK_PROJECT_NAME_UCASE}")
set(ARCH "${CPACK_TARGET_ARCH}")
set(APP "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/${LMMS}.AppDir")

# Target AppImage file
set(APPIMAGE_FILE "${CPACK_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.AppImage")
set(APPIMAGE_BEFORE_RENAME "${CPACK_BINARY_DIR}/${LMMS}-${ARCH}.AppImage")

set(DESKTOP_FILE "${APP}/usr/share/applications/${lmms}.desktop")

# Determine which packaging tool to use
if(NOT CPACK_TOOL)
	# Pick up environmental variable
	if(DEFINED ENV{CPACK_TOOL})
		set(CPACK_TOOL "$ENV{CPACK_TOOL}")
	else()
		set(CPACK_TOOL "appimagetool")
	endif()
endif()

# Toggle command echoing & verbosity
# 0 = no output, 1 = error/warning, 2 = normal, 3 = debug
if(NOT CPACK_DEBUG)
	set(VERBOSITY 1)
	set(APPIMAGETOOL_VERBOSITY "")
	set(COMMAND_ECHO NONE)
	set(OUTPUT_QUIET OUTPUT_QUIET)
else()
	set(VERBOSITY 2)
	set(APPIMAGETOOL_VERBOSITY "--verbose")
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

# Download linuxdeploy, expose bundled appimagetool to PATH
download_binary(LINUXDEPLOY_BIN
	"https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
	linuxdeploy-${ARCH}.AppImage
	FALSE)

# Symlink nested appimagetool
set(_APPIMAGETOOL_LINK "${CPACK_CURRENT_BINARY_DIR}/appimagetool")
if(NOT EXISTS "${_APPIMAGETOOL_LINK}")
	set(_APPIMAGETOOL "${CPACK_CURRENT_BINARY_DIR}/.linuxdeploy-${ARCH}.AppImage/squashfs-root/plugins/linuxdeploy-plugin-appimage/appimagetool-prefix/AppRun")
	message(STATUS "Creating a symbolic link ${_APPIMAGETOOL_LINK} which points to ${_APPIMAGETOOL}")
	create_symlink("${_APPIMAGETOOL}" "${_APPIMAGETOOL_LINK}")
endif()

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

# Copy stk/rawwaves
if(CPACK_STK_RAWWAVE_ROOT)
	set(STK_RAWWAVE_TARGET "${APP}/usr/share/stk/rawwaves/")
	file(MAKE_DIRECTORY "${STK_RAWWAVE_TARGET}")
	file(GLOB RAWWAVES "${CPACK_STK_RAWWAVE_ROOT}/*.raw")
	file(COPY ${RAWWAVES} DESTINATION "${STK_RAWWAVE_TARGET}")
endif()

# Ensure project's "qmake" executable is first on the PATH
get_filename_component(QTBIN "${CPACK_QMAKE_EXECUTABLE}" DIRECTORY)
set(ENV{PATH} "${QTBIN}:$ENV{PATH}")

# Ensure "linuxdeploy-<arch>.AppImage" and "appimagetool" binaries are first on the PATH
set(ENV{PATH} "${CPACK_CURRENT_BINARY_DIR}:$ENV{PATH}")

# Promote finding our own libraries first
set(ENV{LD_LIBRARY_PATH} "${APP}/usr/lib/${lmms}/:${APP}/usr/lib/${lmms}/optional:$ENV{LD_LIBRARY_PATH}")

# Skip slow searching of copyright files https://github.com/linuxdeploy/linuxdeploy/issues/278
set(ENV{DISABLE_COPYRIGHT_FILES_DEPLOYMENT} 1)

# Patch desktop file
file(APPEND "${DESKTOP_FILE}" "X-AppImage-Version=${CPACK_PROJECT_VERSION}\n")

# Prefer a hard-copy of .DirIcon over appimagetool's symlinking
# 256x256 default for Cinnamon Desktop https://forums.linuxmint.com/viewtopic.php?p=2585952
file(COPY "${APP}/usr/share/icons/hicolor/256x256/apps/${lmms}.png" DESTINATION "${APP}")
file(RENAME "${APP}/${lmms}.png" "${APP}/.DirIcon")
file(COPY "${APP}/usr/share/icons/hicolor/256x256/apps/${lmms}.png" DESTINATION "${APP}")

# Build list of libraries to inform linuxdeploy about
# e.g. --library=foo.so --library=bar.so
file(GLOB LIBS "${APP}/usr/lib/${lmms}/*.so")

# Inform linuxdeploy about LADSPA plugins; may depend on bundled fftw3f, etc.
file(GLOB LADSPA "${APP}/usr/lib/${lmms}/ladspa/*.so")

# Inform linuxdeploy about remote plugins
file(GLOB REMOTE_PLUGINS "${APP}/usr/lib/${lmms}/*Remote*")

# Collect, sort and dedupe all libraries
list(APPEND LIBS ${LADSPA})
list(APPEND LIBS ${REMOTE_PLUGINS})
list(APPEND LIBS ${CPACK_SUIL_MODULES})
list(REMOVE_DUPLICATES LIBS)
list(SORT LIBS)

# Handle non-relinkable files (e.g. RemoveVstPlugin[32|64], but not NativeLinuxRemoteVstPlugin)
list(FILTER LIBS EXCLUDE REGEX "\\/RemoteVst")

# Construct linuxdeploy parameters
foreach(_lib IN LISTS LIBS)
	if(EXISTS "${_lib}")
		list(APPEND LIBRARIES "--library=${_lib}")
	endif()
endforeach()

# Call linuxdeploy
message(STATUS "Calling ${LINUXDEPLOY_BIN} --appdir \"${APP}\" ... [... libraries].")
execute_process(COMMAND "${LINUXDEPLOY_BIN}"
	--appdir "${APP}"
	--desktop-file "${DESKTOP_FILE}"
	--custom-apprun "${CPACK_SOURCE_DIR}/cmake/linux/launch_lmms.sh"
	--plugin qt
	${LIBRARIES}
	--verbosity ${VERBOSITY}
	${OUTPUT_QUIET}
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

# Remove svg ambitiously placed by linuxdeploy
file(REMOVE "${APP}/${lmms}.svg")

# Remove libraries that are normally sytem-provided
file(GLOB EXCLUDE_LIBS
	"${APP}/usr/lib/libwine*"
	"${APP}/usr/lib/libcarla_native*"
	"${APP}/usr/lib/${lmms}/optional/libcarla*"
	"${APP}/usr/lib/libjack*")

list(SORT EXCLUDE_LIBS)
foreach(_lib IN LISTS EXCLUDE_LIBS)
	if(EXISTS "${_lib}")
		file(REMOVE "${_lib}")
	endif()
endforeach()

# FIXME: Remove when linuxdeploy supports subfolders https://github.com/linuxdeploy/linuxdeploy/issues/305
foreach(_lib IN LISTS LIBS)
	if(EXISTS "${_lib}")
		file(REMOVE "${_lib}")
	endif()
endforeach()
# Move RemotePlugins into to LMMS_PLUGIN_DIR
file(GLOB WINE_VST_LIBS
	"${APP}/usr/lib/${lmms}/RemoteVstPlugin*"
	"${APP}/usr/lib/${lmms}/32")
foreach(_file IN LISTS WINE_VST_LIBS)
	if(EXISTS "${_file}")
		get_filename_component(_name "${_file}" NAME)
		file(RENAME "${_file}" "${APP}/usr/lib/${_name}")
	endif()
endforeach()
file(GLOB WINE_32_LIBS
	"${APP}/usr/lib/${lmms}/RemoteVstPlugin*")
foreach(_lib IN LISTS WINE_64_LIBS)
	if(EXISTS "${_lib}")
		get_filename_component(_file "${_lib}" NAME)
		file(RENAME "${_lib}" "${APP}/usr/lib/${_file}")
	endif()
endforeach()

file(REMOVE_RECURSE "${SUIL_MODULES_TARGET}" "${APP}/usr/lib/${lmms}/ladspa/")

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
		file(MAKE_DIRECTORY "${APP}/usr/lib/${lmms}/optional/")
		# Copy, but with original symlink name
		file(COPY "${libreal}" DESTINATION "${APP}/usr/lib/${lmms}/optional/")
		file(RENAME "${APP}/usr/lib/${lmms}/optional/${realname}" "${APP}/usr/lib/${lmms}/optional/${symname}")
		continue()
	endif()
endforeach()

if(CPACK_TOOL STREQUAL "appimagetool")
	# Create ".AppImage" file using appimagetool (default)

	# appimage plugin needs ARCH set when running in extracted form from squashfs-root / CI
	set(ENV{ARCH} "${ARCH}")
	message(STATUS "Finishing the AppImage...")
	execute_process(COMMAND ${CPACK_TOOL} "${APP}" "${APPIMAGE_FILE}"
		${APPIMAGETOOL_VERBOSITY}
		${OUTPUT_QUIET}
		COMMAND_ECHO ${COMMAND_ECHO}
		COMMAND_ERROR_IS_FATAL ANY)

	message(STATUS "AppImage created: ${APPIMAGE_FILE}")
elseif(CPACK_TOOL STREQUAL "makeself")
	# Create self-extracting ".run" script using makeself
	find_program(MAKESELF_BIN makeself REQUIRED)

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
	execute_process(COMMAND "${MAKESELF_BIN}"
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
	message(FATAL_ERROR "Packaging tool CPACK_TOOL=\"${CPACK_TOOL}\" is not yet supported")
endif()
