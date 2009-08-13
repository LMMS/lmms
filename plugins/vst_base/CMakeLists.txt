IF(LMMS_SUPPORT_VST)

INCLUDE(BuildPlugin)

BUILD_PLUGIN(vstbase vst_base.cpp vst_plugin.cpp vst_plugin.h communication.h MOCFILES vst_plugin.h)


IF(LMMS_BUILD_WIN32)
	ADD_DEFINITIONS(-DPTW32_STATIC_LIB)
	LINK_LIBRARIES(-lQtCore4 -lpthread -lgdi32 -lws2_32)
	ADD_EXECUTABLE(remote_vst_plugin ${CMAKE_CURRENT_SOURCE_DIR}/remote_vst_plugin.cpp)
	ADD_CUSTOM_COMMAND(TARGET remote_vst_plugin POST_BUILD COMMAND ${STRIP} ${CMAKE_CURRENT_BINARY_DIR}/remote_vst_plugin.exe)
	INSTALL(TARGETS remote_vst_plugin RUNTIME DESTINATION ${PLUGIN_DIR})
ENDIF(LMMS_BUILD_WIN32)


IF(LMMS_BUILD_LINUX AND NOT WANT_VST_NOWINE)

IF(LMMS_HOST_X86_64)
	SET(EXTRA_FLAGS -m32 -Wb,--as-cmd='as --32',--ld-cmd='ld -melf_i386' -L/usr/lib32)
ENDIF(LMMS_HOST_X86_64)
ADD_CUSTOM_COMMAND(
		SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/remote_vst_plugin.cpp
		COMMAND wineg++
		ARGS -I${CMAKE_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_INSTALL_PREFIX}/include/wine/windows -I${CMAKE_INSTALL_PREFIX}/include -I/usr/include/wine/windows ${CMAKE_CURRENT_SOURCE_DIR}/remote_vst_plugin.cpp -mwindows -lpthread ${EXTRA_FLAGS} -o remote_vst_plugin
		COMMAND find -name remote_vst_plugin.exe -exec mv "'{}'" remote_vst_plugin "';'"
		TARGET vstbase
		OUTPUTS remote_vst_plugin
		)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES remote_vst_plugin.exe.so)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/remote_vst_plugin ${CMAKE_CURRENT_BINARY_DIR}/remote_vst_plugin.exe.so DESTINATION ${PLUGIN_DIR})

ENDIF(LMMS_BUILD_LINUX AND NOT WANT_VST_NOWINE)


ENDIF(LMMS_SUPPORT_VST)
