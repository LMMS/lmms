# Required by cmake if `uname -s` is inadaquate
SET(CMAKE_SYSTEM_NAME               Windows)
SET(CMAKE_SYSTEM_VERSION            1)

# The target environment
SET(CMAKE_FIND_ROOT_PATH            ${MINGW_PREFIX})
SET(CMAKE_INSTALL_PREFIX            ${MINGW_PREFIX})

# Windows msys mingw ships with a mostly-suitable preconfigured environment
IF(LMMS_BUILD_MSYS)
	SET(STRIP                       ${MINGW_PREFIX}/bin/strip)
	SET(WINDRES                     ${MINGW_PREFIX}/bin/windres)
	SET(CMAKE_RC_COMPILER           ${WINDRES})
	SET(CMAKE_C_COMPILER            ${MINGW_PREFIX}/bin/gcc)
	SET(CMAKE_CXX_COMPILER          ${MINGW_PREFIX}/bin/g++)

	# Force pkg-config to look for .pc files in $MINGW_PREFIX
	SET(ENV{PKG_CONFIG_PATH}        ${MINGW_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH})
	SET(ENV{PKG_CONFIG_PATH}	${MINGW_PREFIX}/share/pkgconfig:$ENV{PKG_CONFIG_PATH})

	# For 32-bit vst support
	IF(WIN64)
		# Specify the 32-bit cross compiler
		SET(CMAKE_C_COMPILER32      ${MINGW_PREFIX32}/bin/gcc)
		SET(CMAKE_CXX_COMPILER32    ${MINGW_PREFIX32}/bin/g++)
	ENDIF()
	
	# Msys compiler does not support @CMakeFiles/Include syntax
	SET(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES   OFF)
	SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

	# Variable to assist override Qt debug libraries with release versions
	SET(QT_OVERRIDE_LIBRARIES
		optimized;${MINGW_PREFIX}/bin/QtGui4.dll;
		optimized;${MINGW_PREFIX}/bin/QtCore4.dll;
		optimized;${MINGW_PREFIX}/bin/QtXml4.dll;
		debug;${MINGW_PREFIX}/bin/QtGui4.dll;
		debug;${MINGW_PREFIX}/bin/QtCore4.dll;
		debug;${MINGW_PREFIX}/bin/QtXml4.dll;
	)
	IF(LMMS_BUILD_MSYS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
		# Override Qt debug libraries with release versions
		SET(QT_LIBRARIES "${QT_OVERRIDE_LIBRARIES}")
	ENDIF()
# Linux mingw requires explicitly defined tools to prevent clash with native system tools
ELSE()
	SET(MINGW_TOOL_PREFIX           ${MINGW_PREFIX}/bin/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32-)

	# Specify the cross compiler
	SET(CMAKE_C_COMPILER            ${MINGW_TOOL_PREFIX}gcc)
	SET(CMAKE_CXX_COMPILER          ${MINGW_TOOL_PREFIX}g++)
	SET(CMAKE_RC_COMPILER           ${MINGW_TOOL_PREFIX}gcc)

	# Mingw tools
	SET(STRIP                       ${MINGW_TOOL_PREFIX}strip)
	SET(WINDRES                     ${MINGW_TOOL_PREFIX}windres)
	SET(ENV{PKG_CONFIG}             ${MINGW_TOOL_PREFIX}pkg-config)
	
	# Search for programs in the build host directories
	SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	# For libraries and headers in the target directories
	SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

	# For 32-bit vst support	
	IF(WIN64)
		# Specify the 32-bit cross compiler
		SET(MINGW_TOOL_PREFIX32     ${MINGW_PREFIX32}/bin/${CMAKE_SYSTEM_PROCESSOR32}-w64-mingw32-)
		SET(CMAKE_C_COMPILER32      ${MINGW_TOOL_PREFIX32}gcc)
		SET(CMAKE_CXX_COMPILER32    ${MINGW_TOOL_PREFIX32}g++)
	ENDIF()
	
	INCLUDE_DIRECTORIES(${MINGW_PREFIX}/include)
ENDIF()

LINK_DIRECTORIES(${MINGW_PREFIX}/lib ${MINGW_PREFIX}/bin)

# Qt tools
SET(QT_BINARY_DIR				    ${MINGW_PREFIX}/bin)
SET(QT_QMAKE_EXECUTABLE			    ${QT_BINARY_DIR}/qmake)

# Echo modified cmake vars to screen for debugging purposes
IF(NOT DEFINED ENV{MINGW_DEBUG_INFO})
	MESSAGE("")
	MESSAGE("Custom cmake vars: (blank = system default)")
	MESSAGE("-----------------------------------------")
	MESSAGE("* CMAKE_C_COMPILER                     : ${CMAKE_C_COMPILER}")
	MESSAGE("* CMAKE_CXX_COMPILER                   : ${CMAKE_CXX_COMPILER}")
	MESSAGE("* CMAKE_RC_COMPILER                    : ${CMAKE_RC_COMPILER}")
	MESSAGE("* WINDRES                              : ${WINDRES}")
	MESSAGE("* ENV{PKG_CONFIG}                      : $ENV{PKG_CONFIG}")
	MESSAGE("* ENV{PKG_CONFIG_PATH}                 : $ENV{PKG_CONFIG_PATH}")
	MESSAGE("* MINGW_TOOL_PREFIX32                  : ${MINGW_TOOL_PREFIX32}")
	MESSAGE("* CMAKE_C_COMPILER32                   : ${CMAKE_C_COMPILER32}")
	MESSAGE("* CMAKE_CXX_COMPILER32                 : ${CMAKE_CXX_COMPILER32}")
	MESSAGE("* STRIP                                : ${STRIP}")
	MESSAGE("* QT_BINARY_DIR                        : ${QT_BINARY_DIR}")
	MESSAGE("* QT_QMAKE_EXECUTABLE                  : ${QT_QMAKE_EXECUTABLE}")
	MESSAGE("")
	# So that the debug info only appears once
	SET(ENV{MINGW_DEBUG_INFO} SHOWN)
ENDIF()

