# Variables must be prefixed with "CPACK_" to be visible here
set(lmms "${CPACK_PROJECT_NAME}")
set(APP "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/${CPACK_PROJECT_NAME_UCASE}.app")

# 0 = no output, 1 = error/warning, 2 = normal, 3 = debug
set(VERBOSITY 1)
# Set to "STDOUT" to show all verbose commands
set(COMMAND_ECHO NONE)

# Detect release|debug build
if(NOT CPACK_STRIP_FILES_ORIG)
	# -use-debug-libs implies -no-strip
	if(CPACK_QMAKE_EXECUTABLE MATCHES "/homebrew/|/usr/local")
		message(STATUS "Homebrew does not provide debug qt libraries, replacing \"-use-debug-libs\" with \"-no-strip\" instead")
		set(USE_DEBUG_LIBS -no-strip)
	else()
		set(USE_DEBUG_LIBS -use-debug-libs)
	endif()
endif()

if(CPACK_DEBUG)
	set(VERBOSITY 2)
	set(COMMAND_ECHO STDOUT)
endif()

# Create bundle structure
file(MAKE_DIRECTORY "${APP}/Contents/MacOS")
file(MAKE_DIRECTORY "${APP}/Contents/Frameworks")
file(MAKE_DIRECTORY "${APP}/Contents/Resources")

# Fix layout
file(RENAME "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/lib" "${APP}/Contents/lib")
file(RENAME "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/share" "${APP}/Contents/share")
file(RENAME "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/bin" "${APP}/Contents/bin")

# Move binaries into Contents/MacOS
file(RENAME "${APP}/Contents/bin/${lmms}" "${APP}/Contents/MacOS/${lmms}")
file(RENAME "${APP}/Contents/lib/${lmms}/RemoteZynAddSubFx" "${APP}/Contents/MacOS/RemoteZynAddSubFx")
file(REMOVE_RECURSE "${APP}/Contents/bin")
file(REMOVE_RECURSE "${APP}/Contents/share/man1")
file(REMOVE_RECURSE "${APP}/Contents/include")

# Copy missing files
# Convert https://lmms.io to io.lmms
string(REPLACE "." ";" mime_parts "${CPACK_PROJECT_URL}")
string(REPLACE ":" ";" mime_parts "${mime_parts}")
string(REPLACE "/" "" mime_parts "${mime_parts}")
list(REMOVE_AT mime_parts 0)
list(REVERSE mime_parts)
list(JOIN mime_parts "." MACOS_MIMETYPE_ID)
configure_file("${CPACK_CURRENT_SOURCE_DIR}/lmms.plist.in" "${APP}/Contents/Info.plist" @ONLY)
file(COPY "${CPACK_CURRENT_SOURCE_DIR}/project.icns" DESTINATION "${APP}/Contents/Resources")
file(COPY "${CPACK_CURRENT_SOURCE_DIR}/icon.icns" DESTINATION "${APP}/Contents/Resources")
file(RENAME "${APP}/Contents/Resources/icon.icns" "${APP}/Contents/Resources/${lmms}.icns")

# Make all libraries writable for macdeployqt
file(CHMOD_RECURSE "${APP}/Contents" PERMISSIONS
	OWNER_EXECUTE OWNER_WRITE OWNER_READ
	GROUP_EXECUTE GROUP_WRITE GROUP_READ
	WORLD_READ)

# Qt6: Fix @rpath bug https://github.com/orgs/Homebrew/discussions/2823
include(CreateSymlink)
get_filename_component(QTBIN "${CPACK_QMAKE_EXECUTABLE}" DIRECTORY)
get_filename_component(QTDIR "${QTBIN}" DIRECTORY)
create_symlink("${QTDIR}/lib" "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/lib")

# Replace @rpath with @loader_path for Carla
execute_process(COMMAND install_name_tool -change
	"@rpath/libcarlabase.dylib"
	"@loader_path/libcarlabase.dylib"
	"${APP}/Contents/lib/${lmms}/libcarlapatchbay.so"
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND install_name_tool -change
	"@rpath/libcarlabase.dylib"
	"@loader_path/libcarlabase.dylib"
	"${APP}/Contents/lib/${lmms}/libcarlarack.so"
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

# Build list of executables to inform macdeployqt about
# e.g. -executable=foo.dylib -executable=bar.dylib
file(GLOB LIBS "${APP}/Contents/lib/${lmms}/*.so")
file(GLOB LADSPA "${APP}/Contents/lib/${lmms}/ladspa/*.so")
list(APPEND LIBS ${LADSPA})
list(APPEND LIBS "${APP}/Contents/MacOS/RemoteZynAddSubFx")
list(SORT LIBS)

# Construct macdeployqt parameters
foreach(_lib IN LISTS LIBS)
	list(APPEND EXECUTABLES "-executable=${_lib}")
endforeach()

# Call macdeployqt
get_filename_component(QTBIN "${CPACK_QMAKE_EXECUTABLE}" DIRECTORY)
message(STATUS "Calling ${QTBIN}/macdeployqt ${APP} [... executables]")
execute_process(COMMAND "${QTBIN}/macdeployqt" "${APP}" ${EXECUTABLES}
	-verbose=${VERBOSITY}
	${USE_DEBUG_LIBS}
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

# Cleanup symlink
file(REMOVE "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/lib")

# Remove dummy carla libs, relink to a sane location (e.g. /Applications/Carla.app/...)
# (must be done after calling macdeployqt)
file(GLOB CARLALIBS "${APP}/Contents/lib/${lmms}/libcarla*")
foreach(_carlalib IN LISTS CARLALIBS)
	foreach(_lib "${CPACK_CARLA_LIBRARIES}")
		set(_oldpath "../../Frameworks/lib${_lib}.dylib")
		set(_newpath "Carla.app/Contents/MacOS/lib${_lib}.dylib")
		execute_process(COMMAND install_name_tool -change
			"@loader_path/${_oldpath}"
			"@executable_path/../../../${_newpath}"
			"${_carlalib}"
			COMMAND_ECHO ${COMMAND_ECHO}
			COMMAND_ERROR_IS_FATAL ANY)
		file(REMOVE "${APP}/Contents/Frameworks/lib${_lib}.dylib")
	endforeach()
endforeach()

# Call ad-hoc codesign manually (CMake offers this as well)
execute_process(COMMAND codesign --force --deep --sign - "${APP}"
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

# Create DMG
# appdmg won't allow volume names > 27 char https://github.com/LinusU/node-alias/issues/7
find_program(APPDMG_BIN appdmg REQUIRED)
string(SUBSTRING "${CPACK_PROJECT_NAME_UCASE} ${CPACK_PROJECT_VERSION}" 0 27 APPDMG_VOLUME_NAME)
# We'll configure this file twice (again in MacDeployQt.cmake once we know CPACK_TEMPORARY_INSTALL_DIRECTORY)
configure_file("${CPACK_CURRENT_SOURCE_DIR}/appdmg.json.in" "${CPACK_CURRENT_BINARY_DIR}/appdmg.json" @ONLY)

file(REMOVE "${CPACK_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.dmg")
execute_process(COMMAND "${APPDMG_BIN}"
	"${CPACK_CURRENT_BINARY_DIR}/appdmg.json"
	"${CPACK_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.dmg"
	COMMAND_ECHO ${COMMAND_ECHO}
    COMMAND_ERROR_IS_FATAL ANY)