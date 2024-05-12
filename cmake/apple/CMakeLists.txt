# TODO: Re-evaluate these
set(MACOSX_BUNDLE_ICON_FILE             "icon.icns")
set(MACOSX_BUNDLE_GUI_IDENTIFIER        "${PROJECT_NAME_UCASE}")
set(MACOSX_BUNDLE_LONG_VERSION_STRING   "${VERSION}")
set(MACOSX_BUNDLE_BUNDLE_NAME           "${PROJECT_NAME_UCASE}")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING  "${VERSION}")
set(MACOSX_BUNDLE_BUNDLE_VERSION        "${VERSION}")
set(MACOSX_BUNDLE_COPYRIGHT             "${PROJECT_COPYRIGHT}")
set(MACOSX_BUNDLE_MIMETYPE              "application/x-lmms-project")
set(MACOSX_BUNDLE_MIMETYPE_ICON         "project.icns")
set(MACOSX_BUNDLE_MIMETYPE_ID           "io.lmms")
set(MACOSX_BUNDLE_PROJECT_URL           "${PROJECT_URL}")
set(MACOSX_BUNDLE_DMG_TITLE             "${MACOSX_BUNDLE_BUNDLE_NAME} ${MACOSX_BUNDLE_LONG_VERSION_STRING}")
# appdmg won't allow volume names > 27 char https://github.com/LinusU/node-alias/issues/7
string(SUBSTRING "${MACOSX_BUNDLE_DMG_TITLE}" 0 27 MACOSX_BUNDLE_DMG_TITLE)

# Resource files
configure_file("lmms.plist.in"          "Info.plist")
configure_file("package_apple.sh.in"    "package_apple.sh" @ONLY)
configure_file("appdmg.json.in"         "_appdmg.json.in" @ONLY)

# CPack settings/scripts
configure_file("CPackOptions.cmake.in"  "CPackOptions.cmake" @ONLY)
configure_file("CPackPreBuild.cmake.in" "CPackPreBuild.cmake" @ONLY)

set(CPACK_GENERATOR "DragNDrop" PARENT_SCOPE)
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake" PARENT_SCOPE)
