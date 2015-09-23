INSTALL(FILES lmms.png                  DESTINATION "${DATA_DIR}/pixmaps")
INSTALL(FILES lmms                      DESTINATION "${DATA_DIR}/menu")
INSTALL(FILES lmms.desktop              DESTINATION "${DATA_DIR}/applications")
INSTALL(FILES lmms.xml                  DESTINATION "${DATA_DIR}/mime/packages")

SET(LINUX_DESKTOP_EXEC "env QT_X11_NO_NATIVE_MENUBAR=1 lmms %f")
SET(LINUX_MENU_ICON "${DATA_DIR}/pixmaps/lmms.xpm")
CONFIGURE_FILE(lmms.desktop.in ${CMAKE_PROJECT_NAME}.desktop)
CONFIGURE_FILE(menu.in ${CMAKE_PROJECT_NAME}.1)
