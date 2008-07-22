IF(LMMS_HAVE_VST)

INCLUDE(BuildPlugin)

BUILD_PLUGIN(vstbase vst_base.cpp lvsl_client.cpp lvsl_client.h communication.h MOCFILES lvsl_client.h)
SET_TARGET_PROPERTIES(vstbase PROPERTIES COMPILE_FLAGS "-D_FORTIFY_SOURCE=0")

IF(HOST_IS_64BIT)
	SET(EXTRA_FLAGS -m32 -Wb,--as-cmd='as --32',--ld-cmd='ld -melf_i386' -L/usr/lib32)
ENDIF(HOST_IS_64BIT)
ADD_CUSTOM_COMMAND(
		SOURCE lvsl_server.cpp
		COMMAND wineg++
		ARGS -I${CMAKE_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_INSTALL_PREFIX}/include/wine/windows -I/usr/include/wine/windows lvsl_server.cpp -mwindows -lpthread ${EXTRA_FLAGS} -o lvsl_server.exe.so
		TARGET vstbase
		OUTPUTS lvsl_server.exe.so
		)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES lvsl_server.exe.so)

INSTALL(FILES lvsl_server.exe.so DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGIN_DIR})

ENDIF(LMMS_HAVE_VST)
