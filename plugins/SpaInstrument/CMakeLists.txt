IF(LMMS_HAVE_SPA)
	INCLUDE(BuildPlugin)
	BUILD_PLUGIN(spainstrument SpaInstrument.cpp SpaInstrument.h MOCFILES SpaInstrument.h EMBEDDED_RESOURCES logo.png)
ENDIF(LMMS_HAVE_SPA)
