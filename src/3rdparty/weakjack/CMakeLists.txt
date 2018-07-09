# Use weak jack library linking
IF(LMMS_HAVE_WEAKJACK)
	SET(CMAKE_C_FLAGS "-std=c11")
	
	# Enable weakjack, disable metadata support
	ADD_DEFINITIONS(-DUSE_WEAK_JACK=1 -DNO_JACK_METADATA=1)
	
	# Library stub for AppImages running on systems without jack
	ADD_LIBRARY(weakjack MODULE weakjack/weak_libjack.c weakjack/weak_libjack.h)
	TARGET_INCLUDE_DIRECTORIES(weakjack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/weakjack ${JACK_INCLUDE_DIRS})
	INSTALL(TARGETS weakjack LIBRARY DESTINATION "${PLUGIN_DIR}/optional")
ENDIF()
