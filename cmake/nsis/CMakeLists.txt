SET(WIN_PLATFORM mingw)
if(LMMS_MSVC_YEAR)
	SET(WIN_PLATFORM "msvc${LMMS_MSVC_YEAR}")
endif()

SET(CPACK_PACKAGE_ICON                  "${CMAKE_SOURCE_DIR}/cmake/nsis/nsis_branding.bmp")
IF(MSVC)
	STRING(REPLACE "/" "\\\\" CPACK_PACKAGE_ICON ${CPACK_PACKAGE_ICON})
ENDIF(MSVC)
SET(CPACK_NSIS_MUI_ICON                 "${CMAKE_SOURCE_DIR}/cmake/nsis/icon.ico")
SET(CPACK_NSIS_INSTALLED_ICON_NAME      "${CMAKE_PROJECT_NAME}.exe" PARENT_SCOPE)
SET(CPACK_NSIS_DISPLAY_NAME             "${PROJECT_NAME_UCASE} ${VERSION}" PARENT_SCOPE)
SET(CPACK_NSIS_HELP_LINK                "${PROJECT_URL}" PARENT_SCOPE)
SET(CPACK_NSIS_URL_INFO_ABOUT           "${PROJECT_URL}" PARENT_SCOPE)
SET(CPACK_NSIS_CONTACT                  "${PROJECT_EMAIL}" PARENT_SCOPE)
SET(CPACK_PACKAGE_EXECUTABLES           "${CMAKE_PROJECT_NAME}.exe;${PROJECT_NAME_UCASE}" PARENT_SCOPE)
SET(CPACK_NSIS_MENU_LINKS               "${CMAKE_PROJECT_NAME}.exe;${PROJECT_NAME_UCASE}" PARENT_SCOPE)
SET(CPACK_NSIS_DEFINES                  "
  !addincludedir ${CMAKE_SOURCE_DIR}/cmake/nsis
  !include FileAssociation.nsh
  !include LogicLib.nsh
  !include WinVer.nsh")
SET(CPACK_PACKAGE_FILE_NAME             "${CMAKE_PROJECT_NAME}-${VERSION}-${WIN_PLATFORM}-win32")
SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS   "
  \\\${registerExtension} \\\"$INSTDIR\\\\${CMAKE_PROJECT_NAME}.exe\\\" \\\".mmp\\\" \\\"${PROJECT_NAME_UCASE} Project\\\"
  \\\${registerExtension} \\\"$INSTDIR\\\\${CMAKE_PROJECT_NAME}.exe\\\" \\\".mmpz\\\" \\\"${PROJECT_NAME_UCASE} Project (compressed)\\\"
  \\\${IfNot} \\\${AtMostWin7}
    WriteRegDWORD HKLM \\\"Software\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\SideBySide\\\" \\\"PreferExternalManifest\\\" \\\"1\\\"
  \\\${EndIf}
  " PARENT_SCOPE)
SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
  \\\${unregisterExtension} \\\".mmp\\\" \\\"${PROJECT_NAME_UCASE} Project\\\"
  \\\${unregisterExtension} \\\".mmpz\\\" \\\"${PROJECT_NAME_UCASE} Project (compressed)\\\"
  DeleteRegKey HKCR \\\"${PROJECT_NAME_UCASE} Project\\\"
  DeleteRegKey HKCR \\\"${PROJECT_NAME_UCASE} Project (compressed)\\\"
  " PARENT_SCOPE)

IF(WIN64)
	SET(CPACK_PACKAGE_FILE_NAME      "${CMAKE_PROJECT_NAME}-${VERSION}-${WIN_PLATFORM}-win64")
	SET(CPACK_INSTALL_FIX "$PROGRAMFILES64\\\\${CPACK_PACKAGE_INSTALL_DIRECTORY}\\\\")
	SET(CPACK_NSIS_DEFINES "
		${CPACK_NSIS_DEFINES} 
		InstallDir ${CPACK_INSTALL_FIX}
		")
ENDIF()

# Fix windows paths for msys
IF(LMMS_BUILD_MSYS)
	STRING(REPLACE "/" "\\\\" CPACK_PACKAGE_ICON "${CPACK_PACKAGE_ICON}")
	STRING(REPLACE "/" "\\\\" CPACK_NSIS_MUI_ICON "${CPACK_NSIS_MUI_ICON}")
	STRING(REPLACE "/" "\\\\" CPACK_NSIS_DEFINES "${CPACK_NSIS_DEFINES}")
	STRING(REPLACE "/" "\\\\" CMAKE_BINARY_DIR_FIX "${CMAKE_BINARY_DIR}")
	
	# FIXME: there's no easy way to fix $INST_DIR, so we'll redefine it manually
	IF(WIN64)
		SET(NSIS_ARCH "win64")
	ELSE()
		SET(NSIS_ARCH "win32")
	ENDIF()
	SET(CPACK_NSIS_DEFINES "
		${CPACK_NSIS_DEFINES} 
		!define /redef INST_DIR ${CMAKE_BINARY_DIR_FIX}\\\\_CPack_Packages\\\\${NSIS_ARCH}\\\\NSIS\\\\${CPACK_PACKAGE_FILE_NAME}
	")
ENDIF()

# Setup missing parent scopes
SET(CPACK_PACKAGE_FILE_NAME             "${CPACK_PACKAGE_FILE_NAME}" PARENT_SCOPE)
SET(CPACK_NSIS_DEFINES                  "${CPACK_NSIS_DEFINES}" PARENT_SCOPE)
SET(CPACK_PACKAGE_ICON                  "${CPACK_PACKAGE_ICON}" PARENT_SCOPE)
SET(CPACK_NSIS_MUI_ICON                 "${CPACK_NSIS_MUI_ICON}" PARENT_SCOPE)

# Windows resource compilers
CONFIGURE_FILE("lmms.rc.in"             "${CMAKE_BINARY_DIR}/lmms.rc")
CONFIGURE_FILE("zynaddsubfx.rc.in"      "${CMAKE_BINARY_DIR}/plugins/ZynAddSubFx/zynaddsubfx.rc")

IF(LMMS_HAVE_STK)
	FILE(GLOB RAWWAVES "${MINGW_PREFIX}/share/stk/rawwaves/*.raw")
	LIST(SORT RAWWAVES)
	INSTALL(FILES ${RAWWAVES} DESTINATION "${DATA_DIR}/stk/rawwaves")
ENDIF()

INSTALL(FILES "lmms.exe.manifest" DESTINATION .)
INSTALL(FILES "lmms.VisualElementsManifest.xml" DESTINATION .)
INSTALL(DIRECTORY "assets" DESTINATION .)
