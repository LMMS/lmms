INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(resid)

BUILD_PLUGIN(sid
	sid_instrument.cpp
	sid_instrument.h
	resid/envelope.h
	resid/extfilt.h
	resid/filter.h
	resid/pot.h
	resid/siddefs.h
	resid/sid.h
	resid/spline.h
	resid/voice.h
	resid/wave.h
	resid/envelope.cc
	resid/extfilt.cc
	resid/filter.cc
	resid/pot.cc
	resid/sid.cc
	resid/version.cc
	resid/voice.cc
	resid/wave6581_PS_.cc
	resid/wave6581_PST.cc
	resid/wave6581_P_T.cc
	resid/wave6581__ST.cc
	resid/wave8580_PS_.cc
	resid/wave8580_PST.cc
	resid/wave8580_P_T.cc
	resid/wave8580__ST.cc
	resid/wave.cc
	MOCFILES sid_instrument.h
	EMBEDDED_RESOURCES *.png)

# Parse VERSION
FILE(READ "resid/CMakeLists.txt" lines)
STRING(REGEX MATCH "set\\(MAJOR_VER [A-Za-z0-9_]*\\)" MAJOR_RAW ${lines})
STRING(REGEX MATCH "set\\(MINOR_VER [A-Za-z0-9_]*\\)" MINOR_RAW ${lines})
STRING(REGEX MATCH "set\\(PATCH_VER [A-Za-z0-9_]*\\)" PATCH_RAW ${lines})
SEPARATE_ARGUMENTS(MAJOR_RAW)
SEPARATE_ARGUMENTS(MINOR_RAW)
SEPARATE_ARGUMENTS(PATCH_RAW)
LIST(GET MAJOR_RAW 1 MAJOR_RAW)
LIST(GET MINOR_RAW 1 MINOR_RAW)
LIST(GET PATCH_RAW 1 PATCH_RAW)
STRING(REPLACE ")" "" MAJOR_VER "${MAJOR_RAW}")
STRING(REPLACE ")" "" MINOR_VER "${MINOR_RAW}")
STRING(REPLACE ")" "" PATCH_VER "${PATCH_RAW}")

TARGET_COMPILE_DEFINITIONS(sid PRIVATE VERSION="${MAJOR_VER}.${MINOR_VER}.${PATCH_VER}")
