SET(BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}/../../")
IF(LMMS_BUILD_APPLE)
   CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/data/scripts/create_apple_bundle.sh.in"
      "${BUILD_ROOT}/create_apple_bundle.sh" @ONLY)
   CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/data/scripts/create_apple_dmg.sh.in"
      "${BUILD_ROOT}/create_apple_dmg.sh" @ONLY)
   # Add execute permissions to bundle script
   SET(CHMOD_CMD "chmod")
   SET(CHMOD_ARG1 "u+x")
   SET(CHMOD_ARG2 "${BUILD_ROOT}/create_apple_bundle.sh")
   EXECUTE_PROCESS(COMMAND ${CHMOD_CMD} ${CHMOD_ARG1} ${CHMOD_ARG2})
   INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${BUILD_ROOT}/create_apple_bundle.sh)")
ENDIF(LMMS_BUILD_APPLE)