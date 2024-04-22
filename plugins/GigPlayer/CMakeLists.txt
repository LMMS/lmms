if(LMMS_HAVE_GIG)
	INCLUDE(BuildPlugin)
	INCLUDE_DIRECTORIES(${GIG_INCLUDE_DIRS})
	SET(CMAKE_AUTOUIC ON)

	string(REPLACE "." ";" GIG_VERSION_LIST "${Gig_VERSION}")
	list(LENGTH GIG_VERSION_LIST GIG_VERSION_LIST_LENGTH)
	list(GET GIG_VERSION_LIST 0 GIG_VERSION_MAJOR)
	list(GET GIG_VERSION_LIST 1 GIG_VERSION_MINOR)
	list(GET GIG_VERSION_LIST 2 GIG_VERSION_PATCH)
	if(GIG_VERSION_LIST_LENGTH GREATER 3)
		list(GET GIG_VERSION_LIST 3 GIG_VERSION_REVISION)
	endif()
	configure_file("lmms_gig.h.in" "${CMAKE_BINARY_DIR}/lmms_gig.h")

	# FIXME: Make sure we build globally with exceptions enabled and we can remove this
	# Required for not crashing loading files with libgig
	add_compile_options("-fexceptions")

	# disable deprecated check for mingw-x-libgig
	if(LMMS_BUILD_WIN32)
		add_compile_options("-Wno-deprecated")
	endif(LMMS_BUILD_WIN32)

	build_plugin(gigplayer
		GigPlayer.cpp GigPlayer.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui
		MOCFILES GigPlayer.h PatchesDialog.h
		EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
	)
	target_link_libraries(gigplayer SampleRate::samplerate libgig::libgig)
endif(LMMS_HAVE_GIG)
