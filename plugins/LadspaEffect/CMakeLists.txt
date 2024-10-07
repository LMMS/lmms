INCLUDE(BuildPlugin)

BUILD_PLUGIN(ladspaeffect LadspaEffect.cpp LadspaControls.cpp LadspaControlDialog.cpp LadspaMatrixControlDialog.cpp LadspaSubPluginFeatures.cpp LadspaWidgetFactory.cpp LadspaEffect.h LadspaControls.h LadspaControlDialog.h LadspaMatrixControlDialog.h LadspaSubPluginFeatures.h LadspaWidgetFactory.h MOCFILES LadspaEffect.h LadspaControls.h LadspaControlDialog.h LadspaMatrixControlDialog.h EMBEDDED_RESOURCES logo.png)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ladspa")

IF(LMMS_HAVE_CAPS)
ADD_SUBDIRECTORY(caps)
ENDIF()

IF(LMMS_HAVE_TAP)
ADD_SUBDIRECTORY(tap)
ENDIF()

IF(LMMS_HAVE_SWH)
ADD_SUBDIRECTORY(swh)
ENDIF()

IF(LMMS_HAVE_CMT)
ADD_SUBDIRECTORY(cmt)
ENDIF()

IF(LMMS_HAVE_CALF)
ADD_SUBDIRECTORY(calf)
ENDIF()
