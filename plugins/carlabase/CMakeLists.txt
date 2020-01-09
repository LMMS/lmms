# For MacOS, use "OLD" RPATH install_name behavior
# This can be changed to "NEW" safely if install_apple.sh.in
# is updated to relink libcarlabase.dylib.  MacOS 10.8 uses
# cmake 3.9.6, so this can be done at any time.
IF(NOT CMAKE_VERSION VERSION_LESS 3.9)
  CMAKE_POLICY(SET CMP0068 OLD)
ENDIF()

# If Carla was not provided by the system, make a dummy library instead
if(LMMS_HAVE_WEAKCARLA)
  # Mimic the Makefile header
  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h "")
  SET(CARLA_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/backend
    ${CMAKE_CURRENT_BINARY_DIR}
  )
  INCLUDE_DIRECTORIES(${CARLA_INCLUDE_DIRS})
  ADD_LIBRARY(carla_native-plugin MODULE DummyCarla.cpp)
  INSTALL(TARGETS carla_native-plugin LIBRARY DESTINATION "${PLUGIN_DIR}/optional")
  SET(CARLA_LIBRARIES $<TARGET_FILE:carla_native-plugin>)
  SET(CARLA_LIBRARY_DIRS $<TARGET_FILE_DIR:carla_native-plugin>)
  # Set parent scope variables so carlarack and carlapatchbay can see them
  SET(CARLA_LIBRARIES ${CARLA_LIBRARIES} PARENT_SCOPE)
  SET(CARLA_INCLUDE_DIRS ${CARLA_INCLUDE_DIRS} PARENT_SCOPE)
  SET(CARLA_LIBRARY_DIRS ${CARLA_LIBRARY_DIRS} PARENT_SCOPE)
endif()

if(LMMS_HAVE_CARLA OR LMMS_HAVE_WEAKCARLA)
        INCLUDE(BuildPlugin)
        INCLUDE_DIRECTORIES(${CARLA_INCLUDE_DIRS})
        LINK_DIRECTORIES(${CARLA_LIBRARY_DIRS})
        LINK_LIBRARIES(${CARLA_LIBRARIES})
        BUILD_PLUGIN(carlabase carla.cpp carla.h MOCFILES carla.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png" LINK SHARED)
        SET_TARGET_PROPERTIES(carlabase
                PROPERTIES SKIP_BUILD_RPATH TRUE
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH_USE_LINK_PATH TRUE
                INSTALL_RPATH "${CARLA_RPATH}")
endif()
