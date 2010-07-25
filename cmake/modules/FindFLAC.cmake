# Find libflac++
# find the native libflac++ includes and library
#
#
#  FLAC_INCLUDE_DIRS    - where to find the .h files
#  FLAC_LIBRARIES       - list of libraries when using libflac++
#  FLAC_FOUND           - True if libflac++ found.

FIND_PATH(FLAC_INCLUDE_DIRS decoder.h /usr/include/FLAC++ /usr/local/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/local/include/FLAC++)
FIND_PATH(FLAC_INCLUDE_DIRS encoder.h /usr/include/FLAC++ /usr/local/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/local/include/FLAC++)
FIND_PATH(FLAC_INCLUDE_DIRS export.h /usr/include/FLAC++ /usr/local/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/local/include/FLAC++)
FIND_PATH(FLAC_INCLUDE_DIRS metadata.h /usr/include/FLAC++ /usr/local/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/local/include/FLAC++)
FIND_PATH(FLAC_INCLUDE_DIRS all.h /usr/include/FLAC++ /usr/local/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/include/FLAC++ ${CMAKE_INSTALL_PREFIX}/local/include/FLAC++)
FIND_LIBRARY(FLAC_LIBRARIES NAMES FLAC PATH /usr/lib /usr/local/lib ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/local/lib)
FIND_LIBRARY(FLACPP_LIBRARIES NAMES FLAC++ PATH /usr/lib /usr/local/lib ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/local/lib)

SET(FLAC_LIBRARIES ${FLAC_LIBRARIES} ${FLACPP_LIBRARIES})

IF(FLAC_INCLUDE_DIRS AND FLAC_LIBRARIES)
    SET(FLAC_FOUND TRUE)
ENDIF(FLAC_INCLUDE_DIRS AND FLAC_LIBRARIES)

IF(FLAC_FOUND)
    IF(NOT FLAC_FIND_QUIETLY)
        MESSAGE(STATUS "Found libflac++: ${FLAC_LIBRARIES}")
    ENDIF(NOT FLAC_FIND_QUIETLY)
ELSE(FLAC_FOUND)
    SET(FLAC_LIBRARIES "")
    SET(FLAC_INCLUDE_DIRS "")
    MESSAGE(STATUS "Could not find libflac++")
ENDIF(FLAC_FOUND)

MARK_AS_ADVANCED( FLAC_LIBRARIES FLAC_INCLUDE_DIRS )

