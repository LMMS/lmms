if(LMMS_HAVE_FLUIDSYNTH)
	SET(CMAKE_AUTOUIC ON)
	include(BuildPlugin)
	build_plugin(soundfontplayer
		SoundFontPlayer.cpp SoundFontPlayer.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui
		MOCFILES SoundFontPlayer.h PatchesDialog.h
		EMBEDDED_RESOURCES *.png
	)
	target_link_libraries(soundfontplayer fluidsynth SampleRate::samplerate)
endif()
