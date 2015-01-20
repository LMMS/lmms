INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(${FFTW3F_INCLUDE_DIRS})
LINK_DIRECTORIES(${FFTW3F_LIBRARY_DIRS})
LINK_LIBRARIES(${FFTW3F_LIBRARIES})
BUILD_PLUGIN(eq
	EqEffect.cpp EqControls.cpp EqControlsDialog.cpp EqFilter.h EqParameterWidget.cpp EqFader.h EqSpectrumView.h
	MOCFILES EqControls.h EqParameterWidget.h EqFader.h
	QRESOURCES artwork.png logo.png)
