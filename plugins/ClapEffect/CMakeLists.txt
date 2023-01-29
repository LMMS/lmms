IF(LMMS_HAVE_CLAP)
	INCLUDE(BuildPlugin)
	BUILD_PLUGIN(clapeffect ClapEffect.cpp ClapEffect.h
		MOCFILES ClapEffect.h
		EMBEDDED_RESOURCES logo.png)
	target_link_libraries(clapeffect clap)
ENDIF(LMMS_HAVE_CLAP)
