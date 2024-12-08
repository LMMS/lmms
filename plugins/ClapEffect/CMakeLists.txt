IF(LMMS_HAVE_CLAP)
	INCLUDE(BuildPlugin)
	BUILD_PLUGIN(clapeffect ClapEffect.cpp ClapFxControls.cpp ClapFxControlDialog.cpp ClapEffect.h ClapFxControls.h ClapFxControlDialog.h
		MOCFILES ClapEffect.h ClapFxControls.h ClapFxControlDialog.h
		EMBEDDED_RESOURCES logo.png)
	target_link_libraries(clapeffect clap)
ENDIF(LMMS_HAVE_CLAP)
