INSTALL(DIRECTORY icons/                DESTINATION "${DATA_DIR}/icons/hicolor")
INSTALL(FILES lmms.desktop              DESTINATION "${DATA_DIR}/applications")
INSTALL(FILES lmms.xml                  DESTINATION "${DATA_DIR}/mime/packages")

# AppImage creation target
SET(APPIMAGE_FILE "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}.AppImage")

CONFIGURE_FILE("package_linux.sh.in"    "${CMAKE_BINARY_DIR}/package_linux.sh" @ONLY)

FILE(REMOVE "${APPIMAGE_FILE}")
ADD_CUSTOM_TARGET(removeappimage
	COMMAND rm -f "${APPIMAGE_FILE}"
	COMMENT "Removing old AppImage")
ADD_CUSTOM_TARGET(appimage 
	COMMAND chmod +x "${CMAKE_BINARY_DIR}/package_linux.sh"
	COMMAND "${CMAKE_BINARY_DIR}/package_linux.sh"
	COMMENT "Generating AppImage"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
ADD_DEPENDENCIES(appimage removeappimage)
