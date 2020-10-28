INCLUDE(BuildPlugin)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Qt5DataVisualization REQUIRED)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

BUILD_PLUGIN(diginstrument_spectral_analyzer
 PeakApproximation.cpp ../common/PeakMatcher.cpp AnalyzerPlugin.cpp AnalyzerView.cpp  ../common/InstrumentVisualizationWindow.cpp ../common/Qt/LabeledSlider.cpp ../common/Qt/LabeledFieldSlider.cpp
 CWT.hpp Extrema.hpp SpectrumFitter.hpp SplineFitter.hpp ../common/SplineSpectrum.hpp ../common/Spectrum.hpp ../common/Spectrum.hpp ../common/Instrument.hpp ../common/Qt/DimensionField.hpp
 AnalyzerPlugin.h AnalyzerView.h ../common/InstrumentVisualizationWindow.h ../common/Qt/LabeledSlider.h ../common/Qt/LabeledFieldSlider.h
 MOCFILES AnalyzerPlugin.h AnalyzerView.h EMBEDDED_RESOURCES *.png)
set_property(TARGET diginstrument_spectral_analyzer PROPERTY CXX_STANDARD 17)
target_link_libraries(diginstrument_spectral_analyzer wavelib)
target_link_libraries (diginstrument_spectral_analyzer Eigen3::Eigen)
target_link_libraries(diginstrument_spectral_analyzer nlohmann_json::nlohmann_json)
target_link_libraries(diginstrument_spectral_analyzer fftw3)
qt5_use_modules(diginstrument_spectral_analyzer DataVisualization)