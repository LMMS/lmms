# Create LMMS.app bundle using install_apple.sh
# TODO: Port this script to CMake
execute_process(COMMAND chmod u+x ${CPACK_BINARY_DIR}/install_apple.sh)
execute_process(COMMAND ${CPACK_BINARY_DIR}/install_apple.sh ${CPACK_TEMPORARY_INSTALL_DIRECTORY} RESULT_VARIABLE EXIT_CODE)
if(NOT EXIT_CODE EQUAL 0)
	message(FATAL_ERROR "Execution of install_apple.sh failed")
endif()

# Create DMG
message("Generating DMG")
# REMOVE_RECURSE supresses missing file warnings
file(REMOVE_RECURSE "${CPACK_DMG_FILE}")
execute_process(
	COMMAND appdmg "${CPACK_BINARY_DIR}/package_apple.json" "${CPACK_DMG_FILE}"
)
