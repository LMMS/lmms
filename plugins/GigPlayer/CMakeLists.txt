if(LMMS_HAVE_GIG)
	INCLUDE(BuildPlugin)
	INCLUDE_DIRECTORIES(${GIG_INCLUDE_DIRS})

	# Required for not crashing loading files with libgig
	IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		SET(GCC_COVERAGE_COMPILE_FLAGS "-fexceptions")
		add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})
	ENDIF()

	# disable deprecated check for mingw-x-libgig
	if(LMMS_BUILD_WIN32 AND MINGW)
		SET(GCC_GIG_COMPILE_FLAGS "-Wno-deprecated")
		add_definitions(${GCC_GIG_COMPILE_FLAGS})
	endif()

	build_plugin(gigplayer
		GigPlayer.cpp GigPlayer.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui
		MOCFILES GigPlayer.h PatchesDialog.h
		UICFILES PatchesDialog.ui
		EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
	)
	target_link_libraries(gigplayer SampleRate::samplerate libgig::libgig)
endif(LMMS_HAVE_GIG)
