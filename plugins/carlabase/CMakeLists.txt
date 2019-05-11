# For MacOS, use "OLD" RPATH install_name behavior
# This can be changed to "NEW" safely if install_apple.sh.in
# is updated to relink libcarlabase.dylib.  MacOS 10.8 uses
# cmake 3.9.6, so this can be done at any time.
IF(NOT CMAKE_VERSION VERSION_LESS 3.9)
  CMAKE_POLICY(SET CMP0068 OLD)
ENDIF()

if(LMMS_HAVE_CARLA)
        INCLUDE(BuildPlugin)
        INCLUDE_DIRECTORIES(${CARLA_INCLUDE_DIRS})
        LINK_DIRECTORIES(${CARLA_LIBRARY_DIRS})
        LINK_LIBRARIES(${CARLA_LIBRARIES})
        BUILD_PLUGIN(carlabase carla.cpp carla.h MOCFILES carla.h EMBEDDED_RESOURCES artwork-patchbay.png artwork-rack.png LINK SHARED)
        SET_TARGET_PROPERTIES(carlabase
                PROPERTIES SKIP_BUILD_RPATH TRUE
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH_USE_LINK_PATH TRUE
                INSTALL_RPATH "${CARLA_RPATH}")
endif(LMMS_HAVE_CARLA)
