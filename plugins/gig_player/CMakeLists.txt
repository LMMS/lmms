if(LMMS_HAVE_GIG)
	INCLUDE(BuildPlugin)
    INCLUDE_DIRECTORIES(${GIG_INCLUDE_DIRS}{)
    LINK_DIRECTORIES(${GIG_LIBRARY_DIRS})
    LINK_LIBRARIES(${GIG_LIBRARIES})
	BUILD_PLUGIN(gigplayer gig_player.cpp gig_player.h patches_dialog.cpp patches_dialog.h patches_dialog.ui MOCFILES gig_player.h patches_dialog.h UICFILES patches_dialog.ui EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
endif(LMMS_HAVE_GIG)

