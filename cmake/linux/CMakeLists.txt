INSTALL(FILES lmms.png                  DESTINATION "${DATA_DIR}/pixmaps")
INSTALL(FILES lmms                      DESTINATION "${DATA_DIR}/menu")
INSTALL(FILES lmms.desktop              DESTINATION "${DATA_DIR}/applications")
INSTALL(FILES lmms.xml                  DESTINATION "${DATA_DIR}/mime/packages")

CONFIGURE_FILE("package_linux.sh.in"    "${CMAKE_BINARY_DIR}/package_linux.sh" @ONLY)

# AppImage creation target
SET(APPIMAGE_FILE "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}-linux${CMAKE_SYSTEM_PROCESSOR}.AppImage")
FILE(REMOVE "${APPIMAGE_FILE}")
ADD_CUSTOM_TARGET(removeappimage
	COMMAND touch "\"${APPIMAGE_FILE}\"" && rm "\"${APPIMAGE_FILE}\""
	COMMENT "Removing old AppImage")
ADD_CUSTOM_TARGET(appimage 
	COMMAND "\"${CMAKE_BINARY_DIR}/package_linux.sh\""
	COMMENT "Generating AppImage")
ADD_DEPENDENCIES(appimage removeappimage)

# see also ../postinstall/CMakeLists.txt
