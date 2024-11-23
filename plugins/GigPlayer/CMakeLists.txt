if(LMMS_HAVE_GIG)
	INCLUDE(BuildPlugin)
	include_directories(SYSTEM ${GIG_INCLUDE_DIRS})
	SET(CMAKE_AUTOUIC ON)

	# Required for not crashing loading files with libgig
	add_compile_options("-fexceptions")

	link_directories(${GIG_LIBRARY_DIRS})
	link_libraries(${GIG_LIBRARIES})
	build_plugin(gigplayer
		GigPlayer.cpp GigPlayer.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui
		MOCFILES GigPlayer.h PatchesDialog.h
		EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
	)
	target_link_libraries(gigplayer SampleRate::samplerate)
endif(LMMS_HAVE_GIG)
