IF(WANT_CAPS)
ADD_SUBDIRECTORY(caps)
ENDIF(WANT_CAPS)

IF(WANT_TAP)
ADD_SUBDIRECTORY(tap)
ENDIF(WANT_TAP)

IF(WANT_SWH)
ADD_SUBDIRECTORY(swh)
ENDIF(WANT_SWH)

IF(WANT_CMT)
ADD_SUBDIRECTORY(cmt)
ENDIF(WANT_CMT)

IF(WANT_CALF)
ADD_SUBDIRECTORY(calf)
ENDIF(WANT_CALF)


INCLUDE(BuildPlugin)

BUILD_PLUGIN(ladspaeffect ladspa_effect.cpp ladspa_controls.cpp ladspa_control_dialog.cpp ladspa_subplugin_features.cpp ladspa_effect.h ladspa_controls.h ladspa_control_dialog.h ladspa_subplugin_features.h MOCFILES ladspa_effect.h ladspa_controls.h ladspa_control_dialog.h EMBEDDED_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.png)


