INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(${FFTW3F_INCLUDE_DIRS})
LINK_LIBRARIES(${FFTW3F_LIBRARIES})
BUILD_PLUGIN(analyzer Analyzer.cpp SaProcessor.cpp SaControls.cpp SaControlsDialog.cpp SaSpectrumView.h SaSpectrumView.cpp
MOCFILES SaProcessor.h SaControls.h SaControlsDialog.h SaSpectrumView.h EMBEDDED_RESOURCES *.png)
