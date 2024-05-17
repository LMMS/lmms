# Variables must be prefixed with "CPACK_" to be visible here
set(APP "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/${CPACK_PROJECT_NAME_UCASE}.AppDir")

# Target AppImage file
set(APPIMAGE_FILE "${CPACK_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.AppImage")

# 0 = no output, 1 = error/warning, 2 = normal, 3 = debug
set(VERBOSITY 1)
# Set to "STDOUT" to show all verbose commands
set(COMMAND_ECHO NONE)

# Detect release|debug build
if(NOT CPACK_STRIP_FILES)
	set(NO_STRIP -no-strip)
endif()

if(CPACK_DEBUG)
	set(VERBOSITY 2)
	set(COMMAND_ECHO STDOUT)
endif()

include("${CPACK_SOURCE_DIR}/cmake/modules/CreateSymlink.cmake")

set(LINUXDEPLOYQT_URL "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/")
string(APPEND LINUXDEPLOYQT_URL "linuxdeployqt-continuous-${CPACK_TARGET_ARCH}.AppImage")
set(DESKTOP_FILE "${APP}/usr/share/applications/${CPACK_PROJECT_NAME}.desktop")
set(LMMS "${CPACK_PROJECT_NAME}")

message(STATUS "Creating AppDir ${APP}...")

file(REMOVE_RECURSE "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/include")
file(MAKE_DIRECTORY "${APP}/usr")

# Setup AppDir structure (/usr/bin, /usr/lib, ... etc)
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

# Ensure "linuxdeployqt" binary (in extracted form) is first on the PATH
set(ENV{PATH} "${CPACK_CURRENT_BINARY_DIR}/squashfs-root/usr/bin:$ENV{PATH}")

find_program(LINUXDEPLOYQT_BIN linuxdeployqt)
find_program(APPIMAGETOOL_BIN appimagetool)

set(LINUXDEPLOYQT_APPIMAGE "${CPACK_CURRENT_BINARY_DIR}/linuxdeployqt.AppImage")
if(NOT LINUXDEPLOYQT_BIN OR NOT APPIMAGETOOL_BIN)
	message(STATUS "Downloading linuxdeployqt...")
	file(DOWNLOAD
		"${LINUXDEPLOYQT_URL}"
		"${LINUXDEPLOYQT_APPIMAGE}"
		STATUS DOWNLOAD_STATUS)
	# Check if download was successful.
	list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
	list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
	if(NOT ${STATUS_CODE} EQUAL 0)
		file(REMOVE "${LINUXDEPLOYQT_APPIMAGE}")
		message(FATAL_ERROR "Error downloading ${LINUXDEPLOYQT_URL} ${ERROR_MESSAGE}")
	endif()

	# Ensure the file is executable
	file(CHMOD "${LINUXDEPLOYQT_APPIMAGE}" PERMISSIONS
		OWNER_EXECUTE OWNER_WRITE OWNER_READ
		GROUP_EXECUTE GROUP_WRITE GROUP_READ)
	# Extract linuxdeployqt to a predictable location ("bash -c" avoids syntax errors)
	execute_process(COMMAND bash -c "\"${LINUXDEPLOYQT_APPIMAGE}\" --appimage-extract"
		WORKING_DIRECTORY "${CPACK_CURRENT_BINARY_DIR}"
		COMMAND_ECHO ${COMMAND_ECHO}
		RESULT_VARIABLE STATUS_CODE)
	# Check if execution was successful
	if(NOT ${STATUS_CODE} EQUAL 0)
		file(REMOVE "${LINUXDEPLOYQT_APPIMAGE}")
		message(FATAL_ERROR "Error code ${STATUS_CODE} executing ${LINUXDEPLOYQT_APPIMAGE}")
	endif()

	find_program(LINUXDEPLOYQT_BIN linuxdeployqt REQUIRED)
	find_program(APPIMAGETOOL_BIN appimagetool REQUIRED)
endif()

# Create a wrapper script which calls the lmms executable
file(RENAME "${APP}/usr/bin/${LMMS}" "${APP}/usr/bin/${LMMS}.real")
file(RENAME "${APP}/usr/bin/launch_lmms.sh" "${APP}/usr/bin/${LMMS}")
# Ensure the file is executable
file(CHMOD "${APP}/usr/bin/${LMMS}" PERMISSIONS
	OWNER_EXECUTE OWNER_WRITE OWNER_READ
	GROUP_EXECUTE GROUP_WRITE GROUP_READ)

# Symlink executables so linuxdeployqt can find them
set(BIN_ZYN "${APP}/usr/bin/RemoteZynAddSubFx")
set(BIN_VST32 "${APP}/usr/bin/RemoteVstPlugin32.exe.so")
set(BIN_VST64 "${APP}/usr/bin/RemoteVstPlugin64.exe.so")

create_symlink("${APP}/usr/lib/${LMMS}/RemoteZynAddSubFx" "${BIN_ZYN}")
create_symlink("${APP}/usr/lib/${LMMS}/32/RemoteVstPlugin32.exe.so" "${BIN_VST32}")
create_symlink("${APP}/usr/lib/${LMMS}/RemoteVstPlugin64.exe.so" "${BIN_VST64}")

# Deliberatly clobber LD_LIBRARY_PATH per https://github.com/probonopd/linuxdeployqt/issues/129
set(ENV{LD_LIBRARY_PATH} "${APP}/usr/lib/${LMMS}/:${APP}/usr/lib/${LMMS}/optional")

# Handle wine linking
if(EXISTS "{APP}/usr/lib/${LMMS}/32/RemoteVstPlugin32.exe.so")
	execute_process(COMMAND ldd "${BIN_VST32}"
			OUTPUT_VARIABLE LDD_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ECHO ${COMMAND_ECHO}
			COMMAND_ERROR_IS_FATAL ANY)
	string(replace "\n" ";" LDD_LIST "${LDD_OUTPUT}")
	foreach(line ${LDD_LIST})
		if(line MATCHES "libwine.so" AND line MATCHES "not found")
			set(ENV{LD_LIBRARY_PATH} "$ENV{LD_LIBRARY_PATH}:${CPACK_WINE_32_LIBRARY_DIRS}")
			continue()
		endif()
	endforeach()
endif()
if(EXISTS "${APP}/usr/lib/${LMMS}/RemoteVstPlugin64.exe.so")
	execute_process(COMMAND ldd "${BIN_VST64}"
			OUTPUT_VARIABLE LDD_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ECHO ${COMMAND_ECHO}
			COMMAND_ERROR_IS_FATAL ANY)
	string(replace "\n" ";" LDD_LIST "${LDD_OUTPUT}")
	foreach(line ${LDD_LIST})
		if(line MATCHES "libwine.so" AND line MATCHES "not found")
			set(ENV{LD_LIBRARY_PATH} "$ENV{LD_LIBRARY_PATH}:${CPACK_WINE_64_LIBRARY_DIRS}")
			continue()
		endif()
	endforeach()
endif()

# Patch desktop file
file(READ "${DESKTOP_FILE}" DESKTOP_FILE_CONTENTS)
string(REPLACE "Exec=${LMMS}" "Exec=${LMMS}.real" DESKTOP_FILE_CONTENTS "${DESKTOP_FILE_CONTENTS}")
file(WRITE "${DESKTOP_FILE}" "${DESKTOP_FILE_CONTENTS}")
file(APPEND "${DESKTOP_FILE}" "X-AppImage-Version=${CPACK_PROJECT_VERSION}\n")

# Build list of executables to inform linuxdeployqt about
# e.g. -executable=foo.dylib -executable=bar.dylib
file(GLOB LIBS "${APP}/usr/lib/${LMMS}/*.so")
#file(GLOB LADSPA "${APP}/usr/lib/${LMMS}/ladspa/*.so")
list(APPEND LIBS "${APP}/usr/lib/lmms/ladspa/imp_1199.so")
list(APPEND LIBS "${APP}/usr/lib/lmms/ladspa/imbeq_1197.so")
list(APPEND LIBS "${APP}/usr/lib/lmms/ladspa/pitch_scale_1193.so")
list(APPEND LIBS "${APP}/usr/lib/lmms/ladspa/pitch_scale_1194.so")
list(APPEND LIBS ${LADSPA})
list(APPEND LIBS "${BIN_ZYN}")
list(APPEND LIBS "${BIN_VST32}")
list(APPEND LIBS "${BIN_VST64}")
list(SORT LIBS)

# Construct linuxdeployqt parameters
foreach(_LIB IN LISTS LIBS)
	list(APPEND EXECUTABLES "-executable=${_LIB}")
endforeach()

# Call linuxdeployqt
message(STATUS "Calling ${LINUXDEPLOYQT_BIN} ${DESKTOP_FILE} [... executables]")
execute_process(COMMAND "${LINUXDEPLOYQT_BIN}" "${DESKTOP_FILE}" ${EXECUTABLES}
	-bundle-non-qt-libs
	-verbose=${VERBOSITY}
	${NO_STRIP}
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)
	#-unsupported-allow-new-glibc

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
execute_process(COMMAND ldd "${APP}/usr/bin/${LMMS}.real"
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

# Point the AppRun to the wrapper script
file(REMOVE "${APP}/AppRun")
create_symlink("${APP}/usr/bin/${LMMS}" "${APP}/AppRun")

# Add AppDir icon
create_symlink("${APP}/${LMMS}.png" "${APP}/.DirIcon")

# Create AppImage
message(STATUS "Finishing the AppImage...")
execute_process(COMMAND "${APPIMAGETOOL_BIN}"
	"${APP}"
	"${APPIMAGE_FILE}"
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

if(EXISTS "${APPIMAGE_FILE}")
	message(STATUS "AppImage create: ${APPIMAGE_FILE}")
else()
	message(FATAL_ERROR "An error occured generating the AppImage")
endif()