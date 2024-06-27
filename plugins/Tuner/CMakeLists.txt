if(NOT LMMS_HAVE_TUNER)
	return()
endif()

include(BuildPlugin)

include_directories(${AUBIO_INCLUDE_DIRS})
link_libraries(${AUBIO_LIBRARY})

build_plugin(tuner Tuner.cpp TunerControls.cpp TunerControlDialog.cpp MOCFILES Tuner.h TunerControls.h TunerControlDialog.h)
