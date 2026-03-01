IF(LMMS_HAVE_CLAP)
	INCLUDE(BuildPlugin)
	BUILD_PLUGIN(clapinstrument ClapInstrument.cpp ClapInstrument.h
		MOCFILES ClapInstrument.h
		EMBEDDED_RESOURCES logo.png)
	target_link_libraries(clapinstrument clap)
ENDIF()
