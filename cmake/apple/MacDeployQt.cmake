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

execute_process(COMMAND convert
	"${CPACK_CURRENT_SOURCE_DIR}/*.png"
	"${CPACK_CURRENT_BINARY_DIR}/background.tiff"
	COMMAND_ECHO ${COMMAND_ECHO}
	COMMAND_ERROR_IS_FATAL ANY)

# Copy missing files
file(COPY "${CPACK_CURRENT_SOURCE_DIR}/project.icns" DESTINATION "${APP}/Contents/Resources")

# Updates appdmg.json with CPACK_TEMPORARY_INSTALL_DIRECTORY
# - .DS_Store can be recreated using appdmg appdmg.json lmms.dmg
# - Find new .DS_Store from the root of the DMG file
# - cp /Volumes/lmms-x.x.x/.DS_Store ../cmake/apple/DS_Store
configure_file("${CPACK_CURRENT_BINARY_DIR}/_appdmg.json.in" "${CPACK_CURRENT_BINARY_DIR}/appdmg.json")
file(REMOVE "${CPACK_CURRENT_BINARY_DIR}/_appdmg.json.in")

# Create bundle structure
file(MAKE_DIRECTORY "${APP}/Contents/MacOS")
file(MAKE_DIRECTORY "${APP}/Contents/Frameworks")
file(MAKE_DIRECTORY "${APP}/Contents/Resources")

# Make all libraries writable for macdeployqt
file(CHMOD_RECURSE "${APP}/Contents" PERMISSIONS
	OWNER_EXECUTE OWNER_WRITE OWNER_READ
	GROUP_EXECUTE GROUP_WRITE GROUP_READ
	WORLD_READ)

# Fix layout
file(RENAME "${APP}/Contents/Resources/lib" "${APP}/Contents/lib")
file(RENAME "${APP}/Contents/Resources/share" "${APP}/Contents/share")
file(RENAME "${APP}/Contents/Resources/bin" "${APP}/Contents/bin")

# Move binaries into Contents/MacOS
file(RENAME "${APP}/Contents/bin/${lmms}" "${APP}/Contents/MacOS/${lmms}")
file(RENAME "${APP}/Contents/lib/${lmms}/RemoteZynAddSubFx" "${APP}/Contents/MacOS/RemoteZynAddSubFx")
file(REMOVE_RECURSE "${APP}/Contents/bin")
file(REMOVE_RECURSE "${APP}/Contents/share/man1")
file(REMOVE_RECURSE "${APP}/Contents/include")

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