if(LMMS_HAVE_GIG)
	INCLUDE(BuildPlugin)
	INCLUDE_DIRECTORIES(${GIG_INCLUDE_DIRS})

	# Required for not crashing loading files with libgig
	SET(GCC_COVERAGE_COMPILE_FLAGS "-fexceptions")
	add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

    LINK_DIRECTORIES(${GIG_LIBRARY_DIRS})
    LINK_LIBRARIES(${GIG_LIBRARIES})
	BUILD_PLUGIN(gigplayer GigPlayer.cpp GigPlayer.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui MOCFILES GigPlayer.h PatchesDialog.h UICFILES PatchesDialog.ui EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
endif(LMMS_HAVE_GIG)

