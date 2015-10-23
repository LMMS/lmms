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

BUILD_PLUGIN(ladspaeffect LadspaEffect.cpp LadspaControls.cpp LadspaControlDialog.cpp LadspaSubPluginFeatures.cpp LadspaEffect.h LadspaControls.h LadspaControlDialog.h LadspaSubPluginFeatures.h MOCFILES LadspaEffect.h LadspaControls.h LadspaControlDialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")


