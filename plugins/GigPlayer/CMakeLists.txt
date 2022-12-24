if(LMMS_HAVE_GIG)
	INCLUDE(BuildPlugin)
	INCLUDE_DIRECTORIES(${GIG_INCLUDE_DIRS})

	# disable deprecated check for mingw-x-libgig
	if(LMMS_BUILD_WIN32 AND MINGW)
		SET(GCC_GIG_COMPILE_FLAGS "-Wno-deprecated")
		add_definitions(${GCC_GIG_COMPILE_FLAGS})
	endif()

	LINK_DIRECTORIES(${GIG_LIBRARY_DIRS} ${SAMPLERATE_LIBRARY_DIRS})
	LINK_LIBRARIES(${GIG_LIBRARIES} ${SAMPLERATE_LIBRARIES})
	BUILD_PLUGIN(gigplayer GigPlayer.cpp GigPlayer.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui MOCFILES GigPlayer.h PatchesDialog.h UICFILES PatchesDialog.ui EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
endif()

