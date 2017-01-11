# Importer for SMF-like file format, such as .mid, .ove, .wrk

# check for drumstick-file
PKG_CHECK_MODULES(DRUMSTICK_FILE REQUIRED drumstick-file>=0.5.0)
IF(NOT DRUMSTICK_FILE_FOUND)
	MESSAGE(FATAL_ERROR "LMMS requires libdrumstick-file and drumstick-dev >= 0.5.0 - please install, remove CMakeCache.txt and try again!")
ENDIF(NOT DRUMSTICK_FILE_FOUND)

INCLUDE(BuildPlugin)

LINK_DIRECTORIES(${DRUMSTICK_FILE_LIBDIR})
INCLUDE_DIRECTORIES(${DRUMSTICK_FILE_INCLUDEDIR})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

BUILD_PLUGIN(smfimport SmfImport.cpp commonReader.cpp midiReader.cpp oveReader.cpp wrkReader.cpp
    MOCFILES SmfImport.h commonReader.h midiReader.h oveReader.h wrkReader.h )

TARGET_LINK_LIBRARIES(smfimport ${DRUMSTICK_LIBRARIES})
