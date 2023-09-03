INCLUDE(BuildPlugin)

BUILD_PLUGIN(slicert SlicerT.cpp SlicerT.h SlicerTUI.cpp SlicerTUI.h WaveForm.cpp WaveForm.h MOCFILES SlicerT.h SlicerTUI.h WaveForm.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")


