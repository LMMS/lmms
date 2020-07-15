INCLUDE(BuildPlugin)

BUILD_PLUGIN(diginstrument ../common/PeakMatcher.cpp DiginstrumentPlugin.cpp DiginstrumentView.cpp Synthesizer.cpp Interpolator.cpp MultidimensionalNeighbourMap.hpp DiginstrumentPlugin.h DiginstrumentView.h Synthesizer.h Interpolator.h MOCFILES DiginstrumentPlugin.h DiginstrumentView.h EMBEDDED_RESOURCES *.png)
set_property(TARGET diginstrument PROPERTY CXX_STANDARD 17)