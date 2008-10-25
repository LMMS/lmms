IF(LMMS_SUPPORT_VST)
INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(../vst_base)
LINK_DIRECTORIES(../vst_base)
LINK_LIBRARIES(vstbase)

BUILD_PLUGIN(vsteffect vst_effect.cpp vst_effect_controls.cpp vst_effect_control_dialog.cpp vst_subplugin_features.cpp vst_effect.h vst_effect_controls.h vst_effect_control_dialog.h vst_subplugin_features.h MOCFILES vst_effect_controls.h EMBEDDED_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.png)

SET_TARGET_PROPERTIES(vsteffect PROPERTIES COMPILE_FLAGS "-Wno-attributes")

ENDIF(LMMS_SUPPORT_VST)

