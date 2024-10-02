SET(MACOSX_BUNDLE_ICON_FILE             "icon.icns")
SET(MACOSX_BUNDLE_GUI_IDENTIFIER        "${PROJECT_NAME_UCASE}")
SET(MACOSX_BUNDLE_LONG_VERSION_STRING   "${VERSION}")
SET(MACOSX_BUNDLE_BUNDLE_NAME           "${PROJECT_NAME_UCASE}")
SET(MACOSX_BUNDLE_SHORT_VERSION_STRING  "${VERSION}")
SET(MACOSX_BUNDLE_BUNDLE_VERSION        "${VERSION}")
SET(MACOSX_BUNDLE_COPYRIGHT             "${PROJECT_COPYRIGHT}")
SET(MACOSX_BUNDLE_MIMETYPE              "application/x-lmms-project")
SET(MACOSX_BUNDLE_MIMETYPE_ICON         "project.icns")
SET(MACOSX_BUNDLE_MIMETYPE_ID           "io.lmms")
SET(MACOSX_BUNDLE_PROJECT_URL           "${PROJECT_URL}")
SET(MACOSX_BUNDLE_DMG_TITLE             "${MACOSX_BUNDLE_BUNDLE_NAME} ${MACOSX_BUNDLE_LONG_VERSION_STRING}")

# FIXME: appdmg won't allow volume names > 27 char
# See also https://github.com/LinusU/node-appdmg/issues/48
STRING(SUBSTRING "${MACOSX_BUNDLE_DMG_TITLE}" 0 27 MACOSX_BUNDLE_DMG_TITLE)

CONFIGURE_FILE("lmms.plist.in"          "${CMAKE_BINARY_DIR}/Info.plist")
CONFIGURE_FILE("install_apple.sh.in"    "${CMAKE_BINARY_DIR}/install_apple.sh" @ONLY)
CONFIGURE_FILE("package_apple.json.in"  "${CMAKE_BINARY_DIR}/package_apple.json" @ONLY)

IF(CMAKE_OSX_ARCHITECTURES)
	SET(DMG_ARCH "${CMAKE_OSX_ARCHITECTURES}")
ELSEIF(IS_ARM64)
    # Target arch is host arch
	SET(DMG_ARCH "arm64")
ELSE()
	# Fallback to Intel
	SET(DMG_ARCH "x86_64")
ENDIF()

# DMG creation target
SET(DMG_FILE "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}-mac${APPLE_OS_VER}-${DMG_ARCH}.dmg")

FILE(REMOVE "${DMG_FILE}")
ADD_CUSTOM_TARGET(removedmg
	COMMAND touch "\"${DMG_FILE}\"" && rm "\"${DMG_FILE}\""
	COMMENT "Removing old DMG")
ADD_CUSTOM_TARGET(dmg 
	COMMAND appdmg "\"${CMAKE_BINARY_DIR}/package_apple.json\"" "\"${DMG_FILE}\""
	DEPENDS "${CMAKE_BINARY_DIR}/package_apple.json"
	COMMENT "Generating DMG")
ADD_DEPENDENCIES(dmg removedmg)

# see also ../postinstall/CMakeLists.txt

