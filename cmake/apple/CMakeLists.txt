# Info.plist
set(MACOS_ICON "${PROJECT_NAME_UCASE}.icns")
set(MACOS_MIMETYPE "application/x-lmms-project")
set(MACOS_MIMETYPE_ICON "project.icns")
set(MACOS_MIMETYPE_ID "io.lmms")
configure_file("lmms.plist.in" "Info.plist" @ONLY)

# "Bundle"-specific CPack options
set(CPACK_GENERATOR "Bundle" PARENT_SCOPE)
set(CPACK_BUNDLE_NAME "${PROJECT_NAME_UCASE}" PARENT_SCOPE)
set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" PARENT_SCOPE)
set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.icns" PARENT_SCOPE)

# Standard CPack options
# suppress license popup
unset(CPACK_RESOURCE_FILE_LICENSE PARENT_SCOPE)
# disable cpack's strip: causes missing symbols on macOS
set(CPACK_STRIP_FILES_ORIG "${CPACK_STRIP_FILES}" PARENT_SCOPE)
set(CPACK_STRIP_FILES false PARENT_SCOPE)
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${VERSION}-mac${APPLE_OS_VER}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}" PARENT_SCOPE)
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/MacDeployQt.cmake" PARENT_SCOPE)
set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME_UCASE} ${VERSION}" PARENT_SCOPE)
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}/background.tiff" PARENT_SCOPE)
set(CPACK_DMG_DS_STORE "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/DS_Store" PARENT_SCOPE)

# Custom vars to expose to Cpack
# must be prefixed with "CPACK_" per https://stackoverflow.com/a/46526757/3196753)
set(CPACK_CURRENT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
set(CPACK_CURRENT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)
set(CPACK_BINARY_DIR "${CMAKE_BINARY_DIR}" PARENT_SCOPE)
set(CPACK_SOURCE_DIR "${CMAKE_SOURCE_DIR}" PARENT_SCOPE)
set(CPACK_QMAKE_EXECUTABLE "${QT_QMAKE_EXECUTABLE}" PARENT_SCOPE)
set(CPACK_CARLA_LIBRARIES "${CARLA_LIBRARIES}" PARENT_SCOPE)
set(CPACK_PROJECT_NAME "${PROJECT_NAME}" PARENT_SCOPE)
set(CPACK_PROJECT_NAME_UCASE "${PROJECT_NAME_UCASE}" PARENT_SCOPE)

# appdmg is no longer required unless a new .DS_Store file is needed
# Usage:
#    make package
#    make appdmg
#    ... manually copy .DS_Store file
# appdmg won't allow volume names > 27 char https://github.com/LinusU/node-alias/issues/7
string(SUBSTRING "${PROJECT_NAME_UCASE} ${VERSION}" 0 27 APPDMG_VOLUME_NAME)
# We'll configure this file twice (again in MacDeployQt.cmake once we know CPACK_TEMPORARY_INSTALL_DIRECTORY)
configure_file("appdmg.json.in" "_appdmg.json.in" @ONLY)
add_custom_target(appdmg
	COMMAND touch "${CMAKE_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}-appdmg.dmg"
	COMMAND rm "${CMAKE_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}-appdmg.dmg"
	COMMAND appdmg appdmg.json "${CMAKE_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}-appdmg.dmg"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/appdmg.json"
	COMMENT "Generating DMG")

if(CMAKE_VERSION VERSION_LESS "3.19")
    message(WARNING "DMG creation requires at least CMake 3.19")
endif()
