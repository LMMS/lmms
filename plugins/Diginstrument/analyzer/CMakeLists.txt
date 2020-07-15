INCLUDE(BuildPlugin)

BUILD_PLUGIN(diginstrument_spectral_analyzer PeakApproximation.cpp ../common/PeakMatcher.cpp AnalyzerPlugin.cpp AnalyzerView.cpp CWT.hpp Extrema.hpp SpectrumFitter.hpp SplineFitter.hpp AnalyzerPlugin.h AnalyzerView.h MOCFILES AnalyzerPlugin.h AnalyzerView.h EMBEDDED_RESOURCES *.png)
set_property(TARGET diginstrument_spectral_analyzer PROPERTY CXX_STANDARD 17)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
target_link_libraries(diginstrument_spectral_analyzer wavelib)
target_link_libraries (diginstrument_spectral_analyzer Eigen3::Eigen)