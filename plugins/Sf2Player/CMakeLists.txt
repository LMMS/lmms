if(LMMS_HAVE_FLUIDSYNTH)
	include(BuildPlugin)
	build_plugin(sf2player
		Sf2Player.cpp Sf2Player.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui
		MOCFILES Sf2Player.h PatchesDialog.h
		UICFILES PatchesDialog.ui
		EMBEDDED_RESOURCES *.png
	)
	target_link_libraries(sf2player fluidsynth SampleRate::samplerate)
endif()
