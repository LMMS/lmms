INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(${FFTW3F_INCLUDE_DIRS})
LINK_DIRECTORIES(${FFTW3F_LIBRARY_DIRS})
LINK_LIBRARIES(${FFTW3F_LIBRARIES})
BUILD_PLUGIN(eq eqeffect.cpp eqcontrols.cpp eqcontrolsdialog.cpp eqfilter.h eqparameterwidget.cpp eqfader.h eqspectrumview.h ../../src/gui/widgets/Fader.cpp
MOCFILES eqcontrols.h eqparameterwidget.h eqfader.h ../../include/Fader.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
