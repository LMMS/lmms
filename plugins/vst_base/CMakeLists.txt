IF(LMMS_HAVE_VST)

INCLUDE(BuildPlugin)

BUILD_PLUGIN(vstbase vst_base.cpp lvsl_client.cpp lvsl_client.h communication.h MOCFILES lvsl_client.h)
SET_TARGET_PROPERTIES(vstbase PROPERTIES COMPILE_FLAGS "-D_FORTIFY_SOURCE=0")

IF(HOST_IS_64BIT)
	SET(EXTRA_FLAGS -m32 -Wb,--as-cmd='as --32',--ld-cmd='ld -melf_i386' -L/usr/lib32)
ENDIF(HOST_IS_64BIT)
ADD_CUSTOM_COMMAND(
		SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lvsl_server.cpp
		COMMAND wineg++
		ARGS -I${CMAKE_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_INSTALL_PREFIX}/include/wine/windows -I/usr/include/wine/windows ${CMAKE_CURRENT_SOURCE_DIR}/lvsl_server.cpp -mwindows -lpthread ${EXTRA_FLAGS} -o lvsl_server.exe.so
		TARGET vstbase
		OUTPUTS lvsl_server.exe.so
		)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES lvsl_server.exe.so)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/lvsl_server.exe.so DESTINATION ${PLUGIN_DIR})

ENDIF(LMMS_HAVE_VST)
