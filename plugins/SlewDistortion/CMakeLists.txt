INCLUDE(BuildPlugin)

BUILD_PLUGIN(slewdistortion SlewDistortion.cpp SlewDistortionControls.cpp SlewDistortionControlDialog.cpp MOCFILES SlewDistortion.h SlewDistortionControls.h SlewDistortionControlDialog.h EMBEDDED_RESOURCES *.png)
TARGET_LINK_LIBRARIES(slewdistortion hiir)
