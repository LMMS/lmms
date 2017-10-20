IF(LMMS_BUILD_MSYS)
	SET(MINGW_PREFIX                /mingw64)
	SET(MINGW_PREFIX32              /mingw32)
ELSE()
	SET(MINGW_PREFIX                /opt/mingw64)
	SET(MINGW_PREFIX32              /opt/mingw32)
ENDIF()

SET(CMAKE_SYSTEM_PROCESSOR          x86_64)
SET(CMAKE_SYSTEM_PROCESSOR32        i686)

SET(WIN64                           TRUE)


INCLUDE(MinGWCrossCompile)

