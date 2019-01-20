IF(LMMS_SUPPORT_VST)
   INCLUDE(BuildPlugin)
   LINK_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}/..")
   IF(LMMS_BUILD_LINUX)
      LINK_LIBRARIES(-Wl,--enable-new-dtags)
      SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
      SET(CMAKE_INSTALL_RPATH "$ORIGIN")
   ELSE()
      SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${PLUGIN_DIR}")
   ENDIF()
   BUILD_PLUGIN(vestige vestige.cpp vestige.h MOCFILES vestige.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
   TARGET_LINK_LIBRARIES(vestige vstbase)
   
   IF(NOT HAS_STD_MUTEX)
      target_include_directories(vestige PRIVATE
                "${PROJECT_SOURCE_DIR}/src/3rdparty/mingw-std-threads")
      target_compile_definitions(vestige PRIVATE
                -DUSE_MINGW_THREADS_REPLACEMENT)
   ENDIF()
ENDIF(LMMS_SUPPORT_VST)

