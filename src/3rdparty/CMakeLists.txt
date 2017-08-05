include(ExternalProject)

IF(QT5 AND LMMS_BUILD_LINUX)
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory(qt5-x11embed)
ELSE()
	add_library(qx11embedcontainer STATIC /dev/null)
ENDIF()
