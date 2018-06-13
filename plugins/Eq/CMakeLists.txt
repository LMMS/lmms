INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(${FFTW3F_INCLUDE_DIRS})
LINK_LIBRARIES(${FFTW3F_LIBRARIES})
BUILD_PLUGIN(eq EqEffect.cpp EqCurve.cpp EqCurve.h EqControls.cpp EqControlsDialog.cpp EqFilter.h EqParameterWidget.cpp EqFader.h EqSpectrumView.h EqSpectrumView.cpp
MOCFILES EqControls.h EqControlsDialog.h EqCurve.h EqParameterWidget.h EqFader.h EqSpectrumView.h EMBEDDED_RESOURCES *.png)
