cmake_minimum_required(VERSION 3.0)
project(resid)

set(SRC_VER 0.16)
set(MAJOR_VER 5)
set(MINOR_VER 0)
set(PATCH_VER 0)

option(SIDPLAY_BUILD_SHARED "Build a dynamic library instead of static library" OFF)

if (APPLE)
  option(SIDPLAY_BUILD_FRAMEWORK "Build the library as a macOS framework" OFF)
  option(SIDPLAY_BUILD_UNIVERSAL "Build the shared/framework library as a universal binary" ON)
  if (SIDPLAY_BUILD_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")
  endif()
endif ()

file(GLOB SOURCES *.cc)
set(API_HEADERS envelope.h extfilt.h filter.h pot.h sid.h siddefs.h spline.h voice.h wave.h)

if (SIDPLAY_BUILD_SHARED OR SIDPLAY_BUILD_FRAMEWORK)
  add_library(${PROJECT_NAME} SHARED ${SOURCES} ${API_HEADERS})
else()
  add_library(${PROJECT_NAME} ${SOURCES} ${API_HEADERS})
endif()

if (WIN32 AND SIDPLAY_BUILD_SHARED)
  target_compile_definitions(${PROJECT_NAME} PRIVATE RESID_DLL=1 RESID_EXPORTS=1)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION ${SRC_VER}
  SOVERSION ${MAJOR_VER}
  CXX_STANDARD 11)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_BINARY_DIR})

if (APPLE AND SIDPLAY_BUILD_FRAMEWORK)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    FRAMEWORK TRUE
    FRAMEWORK_VERSION C
    MACOSX_FRAMEWORK_IDENTIFIER net.zimmers)
  set_source_files_properties(${API_HEADERS} PROPERTIES
    MACOSX_PACKAGE_LOCATION Headers)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION="${MAJOR_VER}.${MINOR_VER}.${PATCH_VER}")

install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  FRAMEWORK DESTINATION "/Library/Frameworks"
  PUBLIC_HEADER DESTINATION include/${PROJECT_NAME})

if (NOT APPLE OR NOT SIDPLAY_BUILD_FRAMEWORK)
  install(FILES ${API_HEADERS} DESTINATION include/${PROJECT_NAME})
endif ()
