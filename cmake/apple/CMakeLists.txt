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

CONFIGURE_FILE("lmms.plist.in"          "${CMAKE_BINARY_DIR}/Info.plist")
CONFIGURE_FILE("install_apple.sh.in"    "${CMAKE_BINARY_DIR}/install_apple.sh" @ONLY)
CONFIGURE_FILE("package_apple.json.in"  "${CMAKE_BINARY_DIR}/package_apple.json" @ONLY)

# DMG creation target
SET(DMG_FILE "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}-mac${APPLE_OS_VER}.dmg")
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

