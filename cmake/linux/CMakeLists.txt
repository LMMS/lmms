INSTALL(FILES lmms.png                  DESTINATION "${DATA_DIR}/pixmaps")
INSTALL(FILES lmms                      DESTINATION "${DATA_DIR}/menu")
INSTALL(FILES lmms.desktop              DESTINATION "${DATA_DIR}/applications")
INSTALL(FILES lmms.xml                  DESTINATION "${DATA_DIR}/mime/packages")

CONFIGURE_FILE("build-installer.in" "${CMAKE_BINARY_DIR}/build-installer" @ONLY)

IF(LMMS_HOST_X86)
	SET(HOST_ARCH i386-linux-gnu)
ELSEIF(LMMS_HOST_X86_64)
	SET(HOST_ARCH x86_64-linux-gnu)
ELSE()
	SET(HOST_ARCH unknown-linux-gnu)
ENDIF()
SET(RUN_FILE "${CMAKE_PROJECT_NAME}-${VERSION}-${HOST_ARCH}.run")
ADD_CUSTOM_TARGET(makeself-linux
	COMMAND "${CMAKE_BINARY_DIR}/build-installer" "\"${CMAKE_BINARY_DIR}\""
		"\"${RUN_FILE}\""
	COMMENT "Generating installer")

SET(BUNDLE_FILE "${CMAKE_PROJECT_NAME}-${VERSION}-bundled-source-linux.tar.gz")
ADD_CUSTOM_TARGET(bundled-source-linux
	COMMAND "${CMAKE_SOURCE_DIR}/cmake/linux/fetch-sources"
		"\"${CMAKE_BINARY_DIR}\"" "\"${BUNDLE_FILE}\""
	COMMENT "Fetching source for bundled libraries")
