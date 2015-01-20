INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(${FFTW3F_INCLUDE_DIRS})
LINK_DIRECTORIES(${FFTW3F_LIBRARY_DIRS})
LINK_LIBRARIES(${FFTW3F_LIBRARIES})
BUILD_PLUGIN(spectrumanalyzer
	SpectrumAnalyzer.cpp SpectrumAnalyzerControls.cpp SpectrumAnalyzerControlDialog.cpp SpectrumAnalyzer.h SpectrumAnalyzerControls.h SpectrumAnalyzerControlDialog.h
	MOCFILES SpectrumAnalyzerControlDialog.h SpectrumAnalyzerControls.h
	QRESOURCES background.png log_x_axis.png log_y_axis.png logo.png spectrum_background.png spectrum_background_plain.png
)
