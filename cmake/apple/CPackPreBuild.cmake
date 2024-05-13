# Variables must be prefixed with "CPACK_" to be visible here
# See also CpackOptions.cmake.in
execute_process(COMMAND convert
	"${CPACK_CURRENT_SOURCE_DIR}/*.png"
	"${CPACK_CURRENT_BINARY_DIR}/background.tiff"
	COMMAND_ERROR_IS_FATAL ANY)