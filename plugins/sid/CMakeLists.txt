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
