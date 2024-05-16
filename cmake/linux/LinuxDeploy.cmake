# Variables must be prefixed with "CPACK_" to be visible here
set(lmms "${CPACK_PROJECT_NAME}")
set(LMMS "${CPACK_PROJECT_NAME_UCASE}")
set(ARCH "${CPACK_TARGET_ARCH}")
set(APP "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/${LMMS}.AppDir")

# Target AppImage file
set(APPIMAGE_FILE "${CPACK_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.AppImage")
set(APPIMAGE_BEFORE_RENAME "${CPACK_BINARY_DIR}/${LMMS}-${ARCH}.AppImage")

set(DESKTOP_FILE "${APP}/usr/share/applications/${lmms}.desktop")

# 0 = no output, 1 = error/warning, 2 = normal, 3 = debug
set(VERBOSITY 1)
# Set to "STDOUT" to show all verbose commands
set(COMMAND_ECHO NONE)

if(CPACK_DEBUG)
	set(VERBOSITY 2)
	set(COMMAND_ECHO STDOUT)
endif()

include("${CPACK_SOURCE_DIR}/cmake/modules/DownloadBinary.cmake")
include("${CPACK_SOURCE_DIR}/cmake/modules/CreateSymlink.cmake")

# Cleanup CPack "External" json files, old AppImage files
file(GLOB cleanup "${CPACK_BINARY_DIR}/lmms-*.json"
	"${CPACK_BINARY_DIR}/${LMMS}-*.AppImage"
	"${CPACK_BINARY_DIR}/${CPACK_PROJECT_NAME_UCASE}-*.AppImage"
	"${CPACK_BINARY_DIR}/install_manifest.txt")
file(REMOVE ${cleanup})

# Download linuxdeploy
download_binary(LINUXDEPLOY_BIN
	"https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
	linuxdeploy-${ARCH}.AppImage
	false)

# Download linuxdeploy-plugin-qt
download_binary(LINUXDEPLOY_PLUGIN_BIN
	"https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage"
	linuxdeploy-plugin-qt-${ARCH}.AppImage
	false)

message(STATUS "Creating AppDir ${APP}...")

file(REMOVE_RECURSE "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/include")
file(MAKE_DIRECTORY "${APP}/usr")

# Setup AppDir structure (/usr/bin, /usr/lib, /usr/share... etc)
file(GLOB files "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/*")
foreach(_file ${files})
	get_filename_component(_filename "${_file}" NAME)
	if(NOT _filename MATCHES ".AppDir")
		file(RENAME "${_file}" "${APP}/usr/${_filename}")
	endif()
endforeach()

# Ensure project's "qmake" executable is first on the PATH
get_filename_component(QTBIN "${CPACK_QMAKE_EXECUTABLE}" DIRECTORY)
set(ENV{PATH} "${QTBIN}:$ENV{PATH}")

# Ensure "linuxdeploy.AppImage" binary is first on the PATH
set(ENV{PATH} "${CPACK_CURRENT_BINARY_DIR}:$ENV{PATH}")

# Symlink executables so linuxdeployqt can find them
set(BIN_ZYN "${APP}/usr/bin/RemoteZynAddSubFx")
set(BIN_VST32 "${APP}/usr/bin/RemoteVstPlugin32.exe.so")
set(BIN_VST64 "${APP}/usr/bin/RemoteVstPlugin64.exe.so")

create_symlink("${APP}/usr/lib/${lmms}/RemoteZynAddSubFx" "${BIN_ZYN}")
create_symlink("${APP}/usr/lib/${lmms}/32/RemoteVstPlugin32.exe.so" "${BIN_VST32}")
create_symlink("${APP}/usr/lib/${lmms}/RemoteVstPlugin64.exe.so" "${BIN_VST64}")

# Deliberatly clobber LD_LIBRARY_PATH per https://github.com/probonopd/linuxdeployqt/issues/129
set(ENV{LD_LIBRARY_PATH} "${APP}/usr/lib/${lmms}/:${APP}/usr/lib/${lmms}/optional")

# Handle wine linking
if(IS_DIRECTORY "${CPACK_WINE_32_LIBRARY_DIR}")
	execute_process(COMMAND ldd "${BIN_VST32}"
			OUTPUT_VARIABLE LDD_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ECHO ${COMMAND_ECHO}
			COMMAND_ERROR_IS_FATAL ANY)
	string(replace "\n" ";" LDD_LIST "${LDD_OUTPUT}")
	foreach(line ${LDD_LIST})
		if(line MATCHES "libwine.so" AND line MATCHES "not found")
			set(ENV{LD_LIBRARY_PATH} "$ENV{LD_LIBRARY_PATH}:${CPACK_WINE_32_LIBRARY_DIR}")
			continue()
		endif()
	endforeach()
endif()
if(IS_DIRECTORY "${CPACK_WINE_64_LIBRARY_DIR}")
	execute_process(COMMAND ldd "${BIN_VST64}"
			OUTPUT_VARIABLE LDD_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ECHO ${COMMAND_ECHO}
			COMMAND_ERROR_IS_FATAL ANY)
	string(replace "\n" ";" LDD_LIST "${LDD_OUTPUT}")
	foreach(line ${LDD_LIST})
		if(line MATCHES "libwine.so" AND line MATCHES "not found")
			set(ENV{LD_LIBRARY_PATH} "$ENV{LD_LIBRARY_PATH}:${CPACK_WINE_64_LIBRARY_DIR}")
			continue()
		endif()
	endforeach()
endif()

# Patch desktop file
file(READ "${DESKTOP_FILE}" DESKTOP_FILE_CONTENTS)
#string(REPLACE "Exec=${lmms}" "Exec=${lmms}.real" DESKTOP_FILE_CONTENTS "${DESKTOP_FILE_CONTENTS}")
file(WRITE "${DESKTOP_FILE}" "${DESKTOP_FILE_CONTENTS}")
file(APPEND "${DESKTOP_FILE}" "X-AppImage-Version=${CPACK_PROJECT_VERSION}\n")

# TODO: Fix launch_lmms.sh to point directly to /usr/bin/lmms
# linuxdeploy supports wrappers natively; linuxdeployqt didn't; keep until linuxdeployqt is removed
create_symlink("${APP}/usr/bin/${lmms}" "${APP}/usr/bin/${lmms}.real")

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
list(APPEND LIBS "${BIN_ZYN}")
list(APPEND LIBS "${BIN_VST32}")
list(APPEND LIBS "${BIN_VST64}")
list(SORT LIBS)

# Construct linuxdeploy parameters
foreach(_LIB IN LISTS LIBS)
	list(APPEND EXECUTABLES "-executable=${_LIB}")
endforeach()

# Call linuxdeploy
message(STATUS "Calling ${LINUXDEPLOY_BIN} ${DESKTOP_FILE} [... executables]")
execute_process(COMMAND "${LINUXDEPLOY_BIN}"
	--appdir "${APP}"
	--icon-file "${CPACK_SOURCE_DIR}/cmake/linux/icons/scalable/apps/${lmms}.svg"
	--desktop-file "${APP}/usr/share/applications/${lmms}.desktop"
	--custom-apprun "${CPACK_SOURCE_DIR}/cmake/linux/launch_lmms.sh"
	--plugin qt
	--verbosity ${VERBOSITY}
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

# Remove libraries that are normally sytem-provided
file(GLOB UNWANTED_LIBS
	"${APP}/usr/lib/libwine*"
	"${APP}/usr/lib/libcarla*"
	"${APP}/usr/lib/optional/libcarla*"
	"${APP}/usr/lib/libjack*")

foreach(_LIB UNWANTED_LIBS)
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

# Create AppImage
message(STATUS "Finishing the AppImage...")
execute_process(COMMAND "${LINUXDEPLOY_BIN}"
	--appdir "${APP}"
	--output appimage
	--verbosity ${VERBOSITY}
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

message(STATUS "AppImage created successfully... renaming...")

if(EXISTS "${APPIMAGE_BEFORE_RENAME}")
	file(RENAME "${APPIMAGE_BEFORE_RENAME}" "${APPIMAGE_FILE}")
	message(STATUS "AppImage create: ${APPIMAGE_FILE}")
else()
	message(FATAL_ERROR "An error occured generating the AppImage")
endif()