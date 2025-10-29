INCLUDE(BuildPlugin)

BUILD_PLUGIN(stereomatrix StereoMatrix.cpp StereoMatrixControls.cpp StereoMatrixControlDialog.cpp StereoMatrix.h StereoMatrixControls.h StereoMatrixControlDialog.h MOCFILES StereoMatrixControls.h StereoMatrixControlDialog.h EMBEDDED_RESOURCES artwork.svg logo.svg)

