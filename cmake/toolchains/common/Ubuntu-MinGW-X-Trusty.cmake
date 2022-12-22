IF(WIN64)
	INCLUDE(${CMAKE_CURRENT_LIST_DIR}/Win64.cmake)
ELSE()
	INCLUDE(${CMAKE_CURRENT_LIST_DIR}/Win32.cmake)
ENDIF()
INCLUDE(${CMAKE_CURRENT_LIST_DIR}/WinCrossCompile.cmake)

# The target environment
SET(CMAKE_FIND_ROOT_PATH            ${MINGW_PREFIX})
SET(CMAKE_INSTALL_PREFIX            ${MINGW_PREFIX})

# Linux mingw requires explicitly defined tools to prevent clash with native system tools
SET(MINGW_TOOL_PREFIX           ${MINGW_PREFIX}/bin/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32-)

# Specify the cross compiler
SET(CMAKE_C_COMPILER            ${MINGW_TOOL_PREFIX}gcc)
SET(CMAKE_CXX_COMPILER          ${MINGW_TOOL_PREFIX}g++)
SET(CMAKE_RC_COMPILER           ${MINGW_TOOL_PREFIX}windres)

# Mingw tools
SET(STRIP                       ${MINGW_TOOL_PREFIX}strip)
SET(ENV{PKG_CONFIG}             ${MINGW_TOOL_PREFIX}pkg-config)

# For 32-bit vst support	
IF(WIN64)
	# Specify the 32-bit cross compiler
	SET(MINGW_TOOL_PREFIX32     ${MINGW_PREFIX32}/bin/${CMAKE_SYSTEM_PROCESSOR32}-w64-mingw32-)
	SET(CMAKE_C_COMPILER32      ${MINGW_TOOL_PREFIX32}gcc)
	SET(CMAKE_CXX_COMPILER32    ${MINGW_TOOL_PREFIX32}g++)
ENDIF()

INCLUDE_DIRECTORIES(${MINGW_PREFIX}/include)

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
	MESSAGE("* ENV{PKG_CONFIG}                      : $ENV{PKG_CONFIG}")
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
