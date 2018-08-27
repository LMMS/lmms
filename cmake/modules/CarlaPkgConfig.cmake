# Makes carla submodule available to PKG_CHECK_MODULES
SET(CARLA_SUBMODULE_SOURCE ${CMAKE_SOURCE_DIR}/plugins/carlabase/carla)
SET(CARLA_PKGCONFIG_FILE ${CARLA_SUBMODULE_SOURCE}/data/carla-standalone.pc)

# Look for carla-standalone pkg-config file
IF(EXISTS ${CARLA_PKGCONFIG_FILE})
	SET(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH};${CMAKE_BINARY_DIR}")
	FILE(READ ${CARLA_PKGCONFIG_FILE} CARLA_PKGCONFIG_TEXT)
	# add source/includes and source/backend to include location
	STRING(REPLACE "X-INCLUDEDIR-X/carla" "${CARLA_SUBMODULE_SOURCE}/source/includes" CARLA_PKGCONFIG_TEXT "${CARLA_PKGCONFIG_TEXT}")
	IF(LMMS_BUILD_APPLE)
		# Remove linker flags
		STRING(REPLACE "-Wl,rpath=\${libdir}" "" CARLA_PKGCONFIG_TEXT "${CARLA_PKGCONFIG_TEXT}")
		# Add library path and prefix
		STRING(REPLACE "X-PREFIX-X" "/Applications/Carla.app/Contents/MacOS" CARLA_PKGCONFIG_TEXT "${CARLA_PKGCONFIG_TEXT}")
		STRING(REPLACE "X-LIBDIR-X/carla" "/Applications/Carla.app/Contents/MacOS" CARLA_PKGCONFIG_TEXT "${CARLA_PKGCONFIG_TEXT}")
	ENDIF()
	FILE(WRITE "${CMAKE_BINARY_DIR}/carla-standalone.pc" "${CARLA_PKGCONFIG_TEXT}")
	SET(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_BINARY_DIR}")
	MESSAGE("-- Using carla headers from ${CARLA_SUBMODULE_SOURCE}/source/includes")
	MESSAGE("--  DEBUG:\n${CARLA_PKGCONFIG_TEXT}")
ENDIF()
