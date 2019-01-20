BUILD_PLUGIN(vstbase
	../vst_base.cpp ../VstPlugin.cpp ../VstPlugin.h ../communication.h
	MOCFILES ../VstPlugin.h
	EXPORT_BASE_NAME vstbase
	LINK SHARED
)
TARGET_INCLUDE_DIRECTORIES(vstbase
	PUBLIC ../
)
IF(NOT HAS_STD_MUTEX)
        target_include_directories(vstbase PRIVATE
                "${PROJECT_SOURCE_DIR}/src/3rdparty/mingw-std-threads")
        target_compile_definitions(vstbase PRIVATE
                -DUSE_MINGW_THREADS_REPLACEMENT)
ENDIF()

IF(LMMS_BUILD_LINUX)
	TARGET_LINK_LIBRARIES(vstbase qx11embedcontainer)
ENDIF()
