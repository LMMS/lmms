# Use weak jack library linking
if(LMMS_HAVE_WEAKJACK)
	add_library(weakjack STATIC
		weakjack/weak_libjack.c
	)
	target_include_directories(weakjack PUBLIC weakjack)
	target_link_libraries(weakjack PUBLIC jack_headers ${CMAKE_DL_LIBS})
	target_compile_features(weakjack PRIVATE c_std_11)
	# Enable weakjack, disable metadata support
	target_compile_definitions(weakjack PUBLIC
		USE_WEAK_JACK=1
		NO_JACK_METADATA=1
	)
endif()
