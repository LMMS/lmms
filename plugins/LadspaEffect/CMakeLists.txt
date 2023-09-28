INCLUDE(BuildPlugin)

BUILD_PLUGIN(ladspaeffect LadspaEffect.cpp LadspaControls.cpp LadspaControlDialog.cpp LadspaSubPluginFeatures.cpp LadspaEffect.h LadspaControls.h LadspaControlDialog.h LadspaSubPluginFeatures.h MOCFILES LadspaEffect.h LadspaControls.h LadspaControlDialog.h EMBEDDED_RESOURCES logo.png)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ladspa")

IF(LMMS_HAVE_CAPS)
ADD_SUBDIRECTORY(caps)
ENDIF()

IF(LMMS_HAVE_CAPS)
ADD_SUBDIRECTORY(tap)
ENDIF()

IF(LMMS_HAVE_SWH)
ADD_SUBDIRECTORY(swh)
ENDIF()

IF(LMMS_HAVE_SWH)
ADD_SUBDIRECTORY(cmt)
ENDIF()

IF(LMMS_HAVE_SWH)
ADD_SUBDIRECTORY(calf)
ENDIF()
