INCLUDE(BuildPlugin)
BUILD_PLUGIN(vectorscope Vectorscope.cpp VecControls.cpp VecControlsDialog.cpp VectorView.cpp
MOCFILES VecControls.h VecControlsDialog.h VectorView.h EMBEDDED_RESOURCES logo.png)
