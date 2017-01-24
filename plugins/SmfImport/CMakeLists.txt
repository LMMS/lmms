# Importer for SMF-like file format, such as .mid, .ove, .wrk

# check for drumstick-file
IF(WANT_QT5)
    PKG_CHECK_MODULES(DRUMSTICK_FILE REQUIRED drumstick-file>=1.0.0)
ELSE()
    PKG_CHECK_MODULES(DRUMSTICK_FILE REQUIRED drumstick-file>=0.5.0)
ENDIF()
IF(NOT DRUMSTICK_FILE_FOUND)
    IF(WANT_QT5)
	    MESSAGE(FATAL_ERROR "LMMS requires libdrumstick-file and libdrumstick-dev >= 1.0.0 with Qt5 - please install, remove CMakeCache.txt and try again!")
    ELSE()
	    MESSAGE(FATAL_ERROR "LMMS requires libdrumstick-file and libdrumstick-dev >= 0.5.0 with Qt4 - please install, remove CMakeCache.txt and try again!")
    ENDIF()
ENDIF(NOT DRUMSTICK_FILE_FOUND)

INCLUDE(BuildPlugin)

LINK_DIRECTORIES(${DRUMSTICK_FILE_LIBDIR})
INCLUDE_DIRECTORIES(${DRUMSTICK_FILE_INCLUDEDIR})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

BUILD_PLUGIN(smfimport SmfImport.cpp commonReader.cpp midiReader.cpp oveReader.cpp wrkReader.cpp
    MOCFILES SmfImport.h commonReader.h midiReader.h oveReader.h wrkReader.h )

TARGET_LINK_LIBRARIES(smfimport ${DRUMSTICK_FILE_LIBRARIES})
