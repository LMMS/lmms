SET(CPACK_PACKAGE_ICON                  "${CMAKE_SOURCE_DIR}/cmake/nsis/nsis_branding.bmp" PARENT_SCOPE)
SET(CPACK_NSIS_MUI_ICON                 "${CMAKE_SOURCE_DIR}/cmake/nsis/lmms.ico" PARENT_SCOPE)
SET(CPACK_NSIS_INSTALLED_ICON_NAME      "${CMAKE_PROJECT_NAME}.exe" PARENT_SCOPE)
SET(CPACK_NSIS_DISPLAY_NAME             "${PROJECT_NAME_UCASE} ${VERSION}" PARENT_SCOPE)
SET(CPACK_NSIS_HELP_LINK                "${PROJECT_URL}" PARENT_SCOPE)
SET(CPACK_NSIS_URL_INFO_ABOUT           "${PROJECT_URL}" PARENT_SCOPE)
SET(CPACK_NSIS_CONTACT                  "${PROJECT_EMAIL}" PARENT_SCOPE)
SET(CPACK_PACKAGE_EXECUTABLES           "${CMAKE_PROJECT_NAME}.exe;${PROJECT_NAME_UCASE}" PARENT_SCOPE)
SET(CPACK_NSIS_MENU_LINKS               "${CMAKE_PROJECT_NAME}.exe;${PROJECT_NAME_UCASE}" PARENT_SCOPE)
SET(CPACK_NSIS_DEFINES                  "!include ${CMAKE_SOURCE_DIR}/cmake/nsis/FileAssociation.nsh")
SET(CPACK_NSIS_DEFINES                  "${CPACK_NSIS_DEFINES}" PARENT_SCOPE)
SET(CPACK_PACKAGE_FILE_NAME             "${CMAKE_PROJECT_NAME}-${VERSION}-win32" PARENT_SCOPE)
SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  \\\${registerExtension} \\\"$INSTDIR\\\\${CMAKE_PROJECT_NAME}.exe\\\" \\\".mmp\\\" \\\"${PROJECT_NAME_UCASE} Project\\\"
  \\\${registerExtension} \\\"$INSTDIR\\\\${CMAKE_PROJECT_NAME}.exe\\\" \\\".mmpz\\\" \\\"${PROJECT_NAME_UCASE} Project (compressed)\\\"
  " PARENT_SCOPE)
SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
  \\\${unregisterExtension} \\\".mmp\\\" \\\"${PROJECT_NAME_UCASE} Project\\\"
  \\\${unregisterExtension} \\\".mmpz\\\" \\\"${PROJECT_NAME_UCASE} Project (compressed)\\\"
  DeleteRegKey HKCR \\\"${PROJECT_NAME_UCASE} Project\\\"
  DeleteRegKey HKCR \\\"${PROJECT_NAME_UCASE} Project (compressed)\\\"
  " PARENT_SCOPE)

IF(WIN64)
	SET(CPACK_PACKAGE_FILE_NAME      "${CMAKE_PROJECT_NAME}-${VERSION}-win64" PARENT_SCOPE)
	SET(CPACK_NSIS_DEFINES           "${CPACK_NSIS_DEFINES}
		InstallDir \\\"\\\$PROGRAMFILES64\\\\${CPACK_PACKAGE_INSTALL_DIRECTORY}\\\"" PARENT_SCOPE)
ENDIF()

# Windows resource compilers
CONFIGURE_FILE("lmms.rc.in"             "${CMAKE_BINARY_DIR}/lmms.rc")
CONFIGURE_FILE("zynaddsubfx.rc.in"      "${CMAKE_BINARY_DIR}/plugins/zynaddsubfx/zynaddsubfx.rc")

FILE(GLOB RAWWAVES "${CMAKE_INSTALL_PREFIX}/share/stk/rawwaves/*.raw")
INSTALL(FILES ${RAWWAVES} DESTINATION "${DATA_DIR}/stk/rawwaves")

