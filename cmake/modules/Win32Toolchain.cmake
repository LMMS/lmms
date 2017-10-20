IF(LMMS_BUILD_MSYS)
	SET(MINGW_PREFIX                /mingw32)
ELSE()
	SET(MINGW_PREFIX                /opt/mingw32)
ENDIF()

SET(CMAKE_SYSTEM_PROCESSOR          i686)

INCLUDE(MinGWCrossCompile)

