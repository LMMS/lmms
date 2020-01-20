INCLUDE(BuildPlugin)

BUILD_PLUGIN(diginstrument DiginstrumentPlugin.cpp DiginstrumentView.cpp Synthesizer.cpp Interpolator.cpp MultidimensionalNeighbourMap.hpp CWT.hpp DiginstrumentPlugin.h DiginstrumentView.h Synthesizer.h Interpolator.h MOCFILES DiginstrumentPlugin.h DiginstrumentView.h EMBEDDED_RESOURCES *.png)
set_property(TARGET diginstrument PROPERTY CXX_STANDARD 14)
target_link_libraries(diginstrument wavelib)

subdirs(test)