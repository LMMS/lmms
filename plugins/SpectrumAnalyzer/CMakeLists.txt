INCLUDE(BuildPlugin)
include_directories(SYSTEM ${FFTW3F_INCLUDE_DIRS})

LINK_LIBRARIES(${FFTW3F_LIBRARIES})

BUILD_PLUGIN(analyzer Analyzer.cpp SaProcessor.cpp SaControls.cpp SaControlsDialog.cpp SaSpectrumView.cpp SaWaterfallView.cpp
MOCFILES SaProcessor.h SaControls.h SaControlsDialog.h SaSpectrumView.h SaWaterfallView.h DataprocLauncher.h EMBEDDED_RESOURCES *.svg)
