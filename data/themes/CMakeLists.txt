INCLUDE(InstallHelpers)

INSTALL_DATA_SUBDIRS("themes" "*.png;*.css")

# Handle custom branding
INSTALL(FILES "${CMAKE_BINARY_DIR}/cmake/branding/png/splash.png" DESTINATION "${LMMS_DATA_DIR}/themes/default" OPTIONAL)
INSTALL(FILES "${CMAKE_BINARY_DIR}/cmake/branding/png/icon_32x32.png" DESTINATION "${LMMS_DATA_DIR}/themes/default" RENAME icon_small.png OPTIONAL)
INSTALL(FILES "${CMAKE_BINARY_DIR}/cmake/branding/png/icon_128x128.png" DESTINATION "${LMMS_DATA_DIR}/themes/default" RENAME icon.png OPTIONAL)