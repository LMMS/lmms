INCLUDE(BuildPlugin)

if(NOT LMMS_HAVE_TUNER)
	return()
endif()

INCLUDE_DIRECTORIES(${AUBIO_INCLUDE_DIRS})
LINK_LIBRARIES(${AUBIO_LIBRARY})

BUILD_PLUGIN(tuner Tuner.cpp TunerControls.cpp TunerControlDialog.cpp MOCFILES Tuner.h TunerControls.h TunerControlDialog.h)
