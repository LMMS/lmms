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

	# Fix gig.h
	# See https://github.com/LMMS/lmms/pull/3208
	SET(LOCAL_GIG_H)
	IF(FIX_SYSTEM_HEADERS AND EXISTS /usr/include/gig.h)
		EXECUTE_PROCESS(
			COMMAND grep -E "^ +DLS::[:A-Za-z]+;" /usr/include/gig.h
			OUTPUT_VARIABLE BAD_HEADER)
		IF(BAD_HEADER)
			MESSAGE("-- Bad gig.h, rewriting...")
			SET(LOCAL_GIG_H gig.h)
			ADD_CUSTOM_COMMAND(
				OUTPUT gig.h
				COMMAND sed -r
				"s/^( +)(DLS::[:A-Za-z]+;)/\\1using \\2/"
					/usr/include/gig.h > gig.h
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
				VERBATIM)
			SET_DIRECTORY_PROPERTIES(PROPERTIES
				ADDITIONAL_MAKE_CLEAN_FILES gig.h)
		ENDIF()
	ENDIF()
	
	LINK_DIRECTORIES(${GIG_LIBRARY_DIRS} ${SAMPLERATE_LIBRARY_DIRS})
	LINK_LIBRARIES(${GIG_LIBRARIES} ${SAMPLERATE_LIBRARIES})
	BUILD_PLUGIN(gigplayer GigPlayer.cpp GigPlayer.h PatchesDialog.cpp
			PatchesDialog.h PatchesDialog.ui ${LOCAL_GIG_H}
		MOCFILES GigPlayer.h PatchesDialog.h
		UICFILES PatchesDialog.ui
		EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
endif(LMMS_HAVE_GIG)

