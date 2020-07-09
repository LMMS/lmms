INCLUDE(BuildPlugin)

BUILD_PLUGIN(diginstrument PeakApproximation.cpp PeakMatcher.cpp DiginstrumentPlugin.cpp DiginstrumentView.cpp Synthesizer.cpp Interpolator.cpp MultidimensionalNeighbourMap.hpp CWT.hpp Extrema.hpp SpectrumFitter.hpp SplineFitter.hpp DiginstrumentPlugin.h DiginstrumentView.h Synthesizer.h Interpolator.h MOCFILES DiginstrumentPlugin.h DiginstrumentView.h EMBEDDED_RESOURCES *.png)
set_property(TARGET diginstrument PROPERTY CXX_STANDARD 17)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
target_link_libraries(diginstrument wavelib)
target_link_libraries (diginstrument Eigen3::Eigen)

subdirs(test)