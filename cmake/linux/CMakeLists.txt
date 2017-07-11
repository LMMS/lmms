INSTALL(FILES lmms.svg                  DESTINATION "${DATA_DIR}/icons/hicolor/scalable/apps")
INSTALL(FILES project.svg               DESTINATION "${DATA_DIR}/icons/hicolor/scalable/mimetypes/application-x-lmms-project.svg")
INSTALL(FILES lmms                      DESTINATION "${DATA_DIR}/menu")
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
	COMMENT "Generating AppImage")
ADD_DEPENDENCIES(appimage removeappimage)
