IF(LMMS_SUPPORT_VST)

INCLUDE(BuildPlugin)

IF(LMMS_BUILD_WIN32)
	ADD_DEFINITIONS(-DPTW32_STATIC_LIB)
	ADD_EXECUTABLE(RemoteVstPlugin "${CMAKE_CURRENT_SOURCE_DIR}/RemoteVstPlugin.cpp")
	TARGET_LINK_LIBRARIES(RemoteVstPlugin -lQtCore4 -lpthread -lgdi32 -lws2_32)
	SET_TARGET_PROPERTIES(RemoteVstPlugin PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -O3")
	ADD_CUSTOM_COMMAND(TARGET RemoteVstPlugin POST_BUILD COMMAND "${STRIP}" "${CMAKE_CURRENT_BINARY_DIR}/RemoteVstPlugin.exe")
	INSTALL(TARGETS RemoteVstPlugin RUNTIME DESTINATION "${PLUGIN_DIR}")

	IF(LMMS_BUILD_WIN64)
		# build 32 bit version of RemoteVstPlugin for Win64 so we can load
		# 32 bit VST plugins
		ADD_SUBDIRECTORY(Win64)
	ENDIF(LMMS_BUILD_WIN64)
ENDIF(LMMS_BUILD_WIN32)

BUILD_PLUGIN(vstbase vst_base.cpp VstPlugin.cpp VstPlugin.h communication.h MOCFILES VstPlugin.h)


IF(LMMS_BUILD_LINUX AND NOT WANT_VST_NOWINE)

IF(LMMS_HOST_X86_64)
	SET(EXTRA_FLAGS -m32)

	# workaround for broken wineg++ in WINE 1.4 (shipped e.g. with Ubuntu Precise)
	EXEC_PROGRAM( ${WINE_CXX} ARGS "-v -m32 /dev/zero" OUTPUT_VARIABLE WINEBUILD_OUTPUT)
	if("${WINEBUILD_OUTPUT}" MATCHES ".*x86_64-linux-gnu/wine/libwinecrt0.a.*")
		SET(EXTRA_FLAGS ${EXTRA_FLAGS} -nodefaultlibs /usr/lib/i386-linux-gnu/wine/libwinecrt0.a -luser32 -lkernel32 -lgdi32)
	ENDIF()
ENDIF(LMMS_HOST_X86_64)

ADD_CUSTOM_COMMAND(
		SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/RemoteVstPlugin.cpp"
		COMMAND ${WINE_CXX}
		ARGS "-I\"${CMAKE_BINARY_DIR}\"" "-I\"${CMAKE_SOURCE_DIR}/include\"" "-I\"${CMAKE_INSTALL_PREFIX}/include/wine/windows\"" "-I\"${CMAKE_INSTALL_PREFIX}/include\"" -I/usr/include/wine/windows "\"${CMAKE_CURRENT_SOURCE_DIR}/RemoteVstPlugin.cpp\"" -mwindows -lpthread ${EXTRA_FLAGS} -o RemoteVstPlugin
		COMMAND find -name RemoteVstPlugin.exe -exec mv "'{}'" RemoteVstPlugin "';'"
		TARGET vstbase
		OUTPUTS RemoteVstPlugin
		)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES RemoteVstPlugin.exe.so)
INSTALL(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/RemoteVstPlugin" "${CMAKE_CURRENT_BINARY_DIR}/RemoteVstPlugin.exe.so" DESTINATION "${PLUGIN_DIR}")
ENDIF(LMMS_BUILD_LINUX AND NOT WANT_VST_NOWINE)


ENDIF(LMMS_SUPPORT_VST)
