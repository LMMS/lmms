if(LMMS_HAVE_FLUIDSYNTH)
	include(BuildPlugin)
	build_plugin(sf2player
		Sf2Player.cpp Sf2Player.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui
		MOCFILES Sf2Player.h PatchesDialog.h
		EMBEDDED_RESOURCES *.png
	)
	set_target_properties(sf2player PROPERTIES AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})
	target_link_libraries(sf2player fluidsynth SampleRate::samplerate)	
endif()
