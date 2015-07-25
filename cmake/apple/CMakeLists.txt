SET(MACOSX_BUNDLE_ICON_FILE             "lmms.icns")
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

CONFIGURE_FILE("lmms.plist.in"          "${CMAKE_INSTALL_PREFIX}/Info.plist")
CONFIGURE_FILE("install_apple.sh.in"    "${CMAKE_BINARY_DIR}/install_apple.sh" @ONLY)
CONFIGURE_FILE("package_apple.sh.in"    "${CMAKE_BINARY_DIR}/package_apple.sh" @ONLY)

# Add execute permissions to install script
EXECUTE_PROCESS(COMMAND chmod u+x ${CMAKE_BINARY_DIR}/install_apple.sh)
INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_BINARY_DIR}/install_apple.sh)")

# TODO: Add PACKAGE target using package_apple_dmg.sh



