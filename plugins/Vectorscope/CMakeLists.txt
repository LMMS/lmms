INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(${FFTW3F_INCLUDE_DIRS})
LINK_LIBRARIES(${FFTW3F_LIBRARIES})
BUILD_PLUGIN(vectorscope Vectorscope.cpp VecControls.cpp VecControlsDialog.cpp VectorView.cpp
MOCFILES VecControls.h VecControlsDialog.h VectorView.h EMBEDDED_RESOURCES *.svg logo.png)
