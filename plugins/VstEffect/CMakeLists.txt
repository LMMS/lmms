IF(LMMS_SUPPORT_VST)
INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../vst_base")
LINK_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}/../vst_base")
LINK_LIBRARIES(vstbase)
BUILD_PLUGIN(vsteffect VstEffect.cpp VstEffectControls.cpp VstEffectControlDialog.cpp VstSubPluginFeatures.cpp VstEffect.h VstEffectControls.h VstEffectControlDialog.h VstSubPluginFeatures.h MOCFILES VstEffectControlDialog.h VstEffectControls.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
SET_TARGET_PROPERTIES(vsteffect PROPERTIES COMPILE_FLAGS "-Wno-attributes")

ENDIF(LMMS_SUPPORT_VST)

