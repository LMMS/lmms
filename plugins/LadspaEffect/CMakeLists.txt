INCLUDE(BuildPlugin)

BUILD_PLUGIN(ladspaeffect LadspaEffect.cpp LadspaControls.cpp LadspaControlDialog.cpp LadspaSubPluginFeatures.cpp LadspaEffect.h LadspaControls.h LadspaControlDialog.h LadspaSubPluginFeatures.h MOCFILES LadspaEffect.h LadspaControls.h LadspaControlDialog.h EMBEDDED_RESOURCES logo.png)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ladspa")

# Enable C++11 for all except mingw builds
# TODO: Validate against modern mingw compiler
IF(NOT MINGW_PREFIX)
	SET(CMAKE_CXX_STANDARD 11)
ELSE()
	SET(CMAKE_CXX_STANDARD 98)
ENDIF()

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
