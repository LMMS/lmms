# TODO: Move submodules to dedicated directories for flag control
IF(QT5 AND LMMS_BUILD_LINUX AND WANT_VST)
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory(qt5-x11embed)
ENDIF()

ADD_SUBDIRECTORY(rpmalloc)
