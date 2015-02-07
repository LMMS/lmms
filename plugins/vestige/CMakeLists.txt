IF(LMMS_SUPPORT_VST)
	INCLUDE(BuildPlugin)
	INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../vst_base")
	LINK_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}/../vst_base")
	LINK_LIBRARIES(vstbase)
	BUILD_PLUGIN(vestige
		vestige.cpp vestige.h MOCFILES vestige.h
		QRESOURCES artwork.png logo.png select_file.png select_file_active.png
	)
ENDIF(LMMS_SUPPORT_VST)

