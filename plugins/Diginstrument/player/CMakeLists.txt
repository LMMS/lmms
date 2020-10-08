INCLUDE(BuildPlugin)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Qt5DataVisualization REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

BUILD_PLUGIN(diginstrument 
../common/PeakMatcher.cpp ../common/InstrumentVisualizationWindow.cpp DiginstrumentPlugin.cpp DiginstrumentView.cpp Synthesizer.cpp Interpolator.cpp ../common/Qt/LabeledSlider.cpp ../common/Qt/LabeledFieldSlider.cpp
 MultidimensionalNeighbourMap.hpp ../common/toJSON.hpp
 DiginstrumentPlugin.h DiginstrumentView.h Synthesizer.h Interpolator.h ../common/Qt/LabeledSlider.h ../common/Qt/LabeledFieldSlider.h
 MOCFILES DiginstrumentPlugin.h DiginstrumentView.h ../common/InstrumentVisualizationWindow.h EMBEDDED_RESOURCES *.png)
set_property(TARGET diginstrument PROPERTY CXX_STANDARD 17)
target_link_libraries(diginstrument nlohmann_json::nlohmann_json)
qt5_use_modules(diginstrument DataVisualization)