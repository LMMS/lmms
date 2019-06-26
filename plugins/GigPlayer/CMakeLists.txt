if(LMMS_HAVE_GIG)
	INCLUDE(BuildPlugin)
	INCLUDE_DIRECTORIES(${GIG_INCLUDE_DIRS})

	# Disable C++11 on Clang until gig.h is patched
	IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		SET(CMAKE_CXX_STANDARD 98)
	ENDIF()

	# Required for not crashing loading files with libgig
	SET(GCC_COVERAGE_COMPILE_FLAGS "-fexceptions")
	add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

	# disable deprecated check for mingw-x-libgig
	if(LMMS_BUILD_WIN32)
		SET(GCC_GIG_COMPILE_FLAGS "-Wno-deprecated")
		add_definitions(${GCC_GIG_COMPILE_FLAGS})
	endif(LMMS_BUILD_WIN32)

	LINK_DIRECTORIES(${GIG_LIBRARY_DIRS} ${SAMPLERATE_LIBRARY_DIRS})
	LINK_LIBRARIES(${GIG_LIBRARIES} ${SAMPLERATE_LIBRARIES})
	BUILD_PLUGIN(gigplayer GigPlayer.cpp GigPlayer.h PatchesDialog.cpp PatchesDialog.h PatchesDialog.ui MOCFILES GigPlayer.h PatchesDialog.h UICFILES PatchesDialog.ui EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
endif(LMMS_HAVE_GIG)

