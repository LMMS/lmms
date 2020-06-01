# The target environment
SET(CMAKE_FIND_ROOT_PATH            ${MINGW_PREFIX})
SET(CMAKE_INSTALL_PREFIX            ${MINGW_PREFIX})

# Windows msys mingw ships with a mostly-suitable preconfigured environment
SET(STRIP                       ${MINGW_PREFIX}/bin/strip)
SET(CMAKE_RC_COMPILER           ${MINGW_PREFIX}/bin/windres)
SET(CMAKE_C_COMPILER            ${MINGW_PREFIX}/bin/gcc)
SET(CMAKE_CXX_COMPILER          ${MINGW_PREFIX}/bin/g++)

# For 32-bit vst support
IF(WIN64)
	# Specify the 32-bit cross compiler
	SET(CMAKE_C_COMPILER32      ${MINGW_PREFIX32}/bin/gcc)
	SET(CMAKE_CXX_COMPILER32    ${MINGW_PREFIX32}/bin/g++)
ENDIF()

# Msys compiler does not support @CMakeFiles/Include syntax
SET(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES   OFF)
SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

SET(LMMS_BUILD_MSYS 1)
