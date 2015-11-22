INCLUDE(BuildPlugin)

BUILD_PLUGIN(sid sid_instrument.cpp sid_instrument.h envelope.h extfilt.h filter.h pot.h siddefs.h sid.h spline.h voice.h wave.h envelope.cc extfilt.cc filter.cc pot.cc sid.cc version.cc voice.cc wave6581_PS_.cc wave6581_PST.cc wave6581_P_T.cc wave6581__ST.cc wave8580_PS_.cc wave8580_PST.cc wave8580_P_T.cc wave8580__ST.cc wave.cc  MOCFILES sid_instrument.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")


