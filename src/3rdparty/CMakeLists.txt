# TODO: Move submodules to dedicated directories for flag control
IF(QT5 AND LMMS_BUILD_LINUX AND WANT_VST)
	set(BUILD_SHARED_LIBS OFF)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
	add_subdirectory(qt5-x11embed)
ENDIF()

ADD_SUBDIRECTORY(rpmalloc)
