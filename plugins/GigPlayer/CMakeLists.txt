if(LMMS_HAVE_GIG)
	INCLUDE(BuildPlugin)
	INCLUDE_DIRECTORIES(${GIG_INCLUDE_DIRS})

	# Required for not crashing loading files with libgig
	SET(GCC_COVERAGE_COMPILE_FLAGS "-fexceptions")
	add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

	# disable deprecated check for mingw-x-libgig
	if(LMMS_BUILD_WIN32)
		SET(GCC_GIG_COMPILE_FLAGS "-Wno-deprecated")
		add_definitions(${GCC_GIG_COMPILE_FLAGS})
	endif(LMMS_BUILD_WIN32)

	link_directories(${GIG_LIBRARY_DIRS})
	link_libraries(${GIG_LIBRARIES})
	build_plugin(gigplayer
		GigPlayer.cpp GigPlayer.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui
		MOCFILES GigPlayer.h PatchesDialog.h
		UICFILES PatchesDialog.ui
		EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
	)
	target_link_libraries(gigplayer SampleRate::samplerate)
endif(LMMS_HAVE_GIG)
