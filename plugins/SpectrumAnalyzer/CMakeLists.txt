INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(${FFTW3F_INCLUDE_DIRS})

# This is required so that libanalyzer.so can find libringbuffer.so
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

LINK_LIBRARIES(${FFTW3F_LIBRARIES} ringbuffer)

BUILD_PLUGIN(analyzer Analyzer.cpp SaProcessor.cpp SaControls.cpp SaControlsDialog.cpp SaSpectrumView.cpp SaWaterfallView.cpp
MOCFILES SaProcessor.h SaControls.h SaControlsDialog.h SaSpectrumView.h SaWaterfallView.h DataprocLauncher.h EMBEDDED_RESOURCES *.svg logo.png)
