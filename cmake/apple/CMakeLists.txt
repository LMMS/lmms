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

# appdmg won't allow volume names > 27 char https://github.com/LinusU/node-alias/issues/7
STRING(SUBSTRING "${MACOSX_BUNDLE_DMG_TITLE}" 0 27 MACOSX_BUNDLE_DMG_TITLE)

CONFIGURE_FILE("lmms.plist.in"          "${CMAKE_BINARY_DIR}/Info.plist")
CONFIGURE_FILE("package_apple.sh.in"    "${CMAKE_BINARY_DIR}/package_apple.sh" @ONLY)
CONFIGURE_FILE("appdmg.json.in"  "${CMAKE_BINARY_DIR}/appdmg.json" @ONLY)

# CPack vars
set(CPACK_GENERATOR "DragNDrop" PARENT_SCOPE)
# Strip causes missing symbols on macOS
set(CPACK_STRIP_FILES false PARENT_SCOPE)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION}" PARENT_SCOPE)
# Creates a staging directory to create our bundle from
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/CPackApple.cmake" PARENT_SCOPE)
set(CPACK_DMG_VOLUME_NAME "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}-mac${APPLE_OS_VER}.dmg" PARENT_SCOPE)
# tiff file is multi-resource (1x & 2x) created programatically using convert
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_BINARY_DIR}/dmg_branding.tiff" PARENT_SCOPE)
# DS_Store contains the Finder layout
# this file can be recreated using appdmg: Copy .DS_Store from the root of the DMG file
set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/cmake/apple/DS_Store" PARENT_SCOPE)
unset(CPACK_RESOURCE_FILE_LICENSE PARENT_SCOPE)

# Additional vars otherwise unexposed to CPack
set(CPACK_DMG_BACKGROUND_PNGS "${CMAKE_SOURCE_DIR}/cmake/apple/*.png" PARENT_SCOPE)
set(CPACK_BINARY_DIR "${CMAKE_BINARY_DIR}" PARENT_SCOPE)
set(CPACK_DMG_FILE "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}-mac${APPLE_OS_VER}.dmg" PARENT_SCOPE)
