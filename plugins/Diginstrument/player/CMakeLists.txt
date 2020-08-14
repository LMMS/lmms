INCLUDE(BuildPlugin)
find_package(nlohmann_json 3.2.0 REQUIRED)

BUILD_PLUGIN(diginstrument ../common/PeakMatcher.cpp DiginstrumentPlugin.cpp DiginstrumentView.cpp Synthesizer.cpp Interpolator.cpp MultidimensionalNeighbourMap.hpp DiginstrumentPlugin.h DiginstrumentView.h Synthesizer.h Interpolator.h MOCFILES DiginstrumentPlugin.h DiginstrumentView.h EMBEDDED_RESOURCES *.png)
set_property(TARGET diginstrument PROPERTY CXX_STANDARD 17)
target_link_libraries(diginstrument nlohmann_json::nlohmann_json)