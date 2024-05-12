# Create LMMS.app bundle using package_apple.sh
# TODO: Port this script to CMake
execute_process(COMMAND chmod u+x ${CPACK_BINARY_DIR}/package_apple.sh)
execute_process(COMMAND ${CPACK_BINARY_DIR}/install_apple.sh ${CPACK_TEMPORARY_INSTALL_DIRECTORY} RESULT_VARIABLE EXIT_CODE)
if(NOT EXIT_CODE EQUAL 0)
	message(FATAL_ERROR "Execution of package_apple.sh failed")
endif()

# Create tiff from two pngs
execute_process(COMMAND convert "${CPACK_DMG_BACKGROUND_PNGS}" "${CPACK_DMG_BACKGROUND_IMAGE}")