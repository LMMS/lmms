#
# "$Id: CMakeLists.txt 7461 2010-04-06 20:36:34Z AlbrechtS $"
#
# Main CMakeLists.txt to build the FLTK project using CMake (www.cmake.org)
# Written by Andy Cedilnik and Julien Jomier
#
# Copyright 1998-2010 by Bill Spitzak and others.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA.
#
# Please report all bugs and problems on the following page:
#
#     http:#www.fltk.org/str.php
#

#######################################################################
# basic setup
#######################################################################
project(FLTK)
cmake_minimum_required(VERSION 2.6)

# The FLTK version
set(FLTK_VERSION_MAJOR "1")
set(FLTK_VERSION_MINOR "3")
set(FLTK_VERSION_PATCH "0")
set(FLTK_VERSION "${FLTK_VERSION_MAJOR}.${FLTK_VERSION_MINOR}")
set(FLTK_VERSION_FULL "${FLTK_VERSION}.${FLTK_VERSION_PATCH}")

set(EXECUTABLE_OUTPUT_PATH ${FLTK_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${FLTK_BINARY_DIR}/lib)
set(ARCHIVE_OUTPUT_PATH ${FLTK_BINARY_DIR}/lib)

# Search for modules in the FLTK source dir first
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${FLTK_SOURCE_DIR}/CMake)

include_directories(${FLTK_SOURCE_DIR} ${FLTK_BINARY_DIR})

# If this is out-of-source build, then copy FL directory
if(NOT ${FLTK_SOURCE_DIR} STREQUAL ${FLTK_BINARY_DIR})
   file(GLOB FLTK_HEADER_FILES ${FLTK_SOURCE_DIR}/FL/*.[hHr])
   foreach(file ${FLTK_HEADER_FILES})
      GET_FILENAME_COMPONENT(name ${file} NAME)
      CONFIGURE_file(${file} ${FLTK_BINARY_DIR}/FL/${name} COPYONLY)
   endforeach(file)
endif(NOT ${FLTK_SOURCE_DIR} STREQUAL ${FLTK_BINARY_DIR})

#######################################################################
# platform dependent information
#######################################################################
# On unix create backward compatibility symlinks
if(CMAKE_HOST_UNIX)
   if(NOT EXISTS ${FLTK_BINARY_DIR}/Fl)
      EXECUTE_PROCESS(COMMAND ln -s FL Fl
         WORKING_DIRECTORY ${FLTK_BINARY_DIR}
         )
   endif(NOT EXISTS ${FLTK_BINARY_DIR}/Fl)

   file(GLOB FLTK_HEADER_FILES ${FLTK_BINARY_DIR}/FL/*.H)
   foreach(file ${FLTK_HEADER_FILES})
      GET_FILENAME_COMPONENT(nameWE ${file} NAME_WE)
      if(NOT EXISTS ${FLTK_BINARY_DIR}/FL/${nameWE}.h)
         EXECUTE_PROCESS(COMMAND ln -s ${nameWE}.H ${nameWE}.h
            WORKING_DIRECTORY ${FLTK_BINARY_DIR}/FL
            )
      endif()
   endforeach(file)
endif(CMAKE_HOST_UNIX)

include(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

if(APPLE)
   set(USE_QUARTZ ON)
endif(APPLE)

if(WIN32)
   if(MSVC)
      add_definitions(-DWIN32_LEAN_AND_MEAN)
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
   endif(MSVC)
   if(CMAKE_C_COMPILER_ID STREQUAL GNU)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem,windows")
   endif(CMAKE_C_COMPILER_ID STREQUAL GNU)
   if(MINGW AND EXISTS /mingw)
      list(APPEND CMAKE_PREFIX_PATH /mingw)
	endif(MINGW AND EXISTS /mingw)
endif(WIN32)

#######################################################################
# size of ints
include(CheckTypeSize)

CHECK_TYPE_SIZE(short SIZEOF_SHORT)
CHECK_TYPE_SIZE(int   SIZEOF_INT)
CHECK_TYPE_SIZE(long  SIZEOF_LONG)
CHECK_TYPE_SIZE("long long"  HAVE_LONG_LONG)

if(${SIZEOF_SHORT} MATCHES "^2$")
   set(U16 "unsigned short")
endif(${SIZEOF_SHORT} MATCHES "^2$")

if(${SIZEOF_INT} MATCHES "^4$")
   set(U32 "unsigned")
else()
   if(${SIZEOF_LONG} MATCHES "^4$")
      set(U32 "unsigned long")
   endif(${SIZEOF_LONG} MATCHES "^4$")
endif(${SIZEOF_INT} MATCHES "^4$")

if(${SIZEOF_INT} MATCHES "^8$")
   set(U64 "unsigned")
else()
   if(${SIZEOF_LONG} MATCHES "^8$")
      set(U64 "unsigned long")
   endif(${SIZEOF_LONG} MATCHES "^8$")
endif(${SIZEOF_INT} MATCHES "^8$")

#######################################################################
# check for headers, libraries and functions
#######################################################################
# set default search paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# headers
find_file(HAVE_ALSA_ASOUNDLIB_H alsa/asoundlib.h)
find_file(HAVE_DIRENT_H dirent.h)
find_file(HAVE_DLFCN_H dlfcn.h)
find_file(HAVE_FREETYPE_H freetype.h PATH_SUFFIXES freetype2)
find_file(HAVE_GL_GL_H GL/gl.h)
find_file(HAVE_GL_GLU_H GL/glu.h)
find_file(HAVE_LIBPNG_PNG_H libpng/png.h)
find_file(HAVE_LOCALE_H locale.h)
find_file(HAVE_NDIR_H ndir.h)
find_file(HAVE_OPENGL_GLU_H OpenGL/glu.h)
find_file(HAVE_PNG_H png.h)
find_file(HAVE_PTHREAD_H pthread.h)
find_file(HAVE_STDIO_H stdio.h)
find_file(HAVE_STRINGS_H strings.h)
find_file(HAVE_SYS_DIR_H sys/dir.h)
find_file(HAVE_SYS_NDIR_H sys/ndir.h)
find_file(HAVE_SYS_SELECT_H sys/select.h)
find_file(HAVE_SYS_STDTYPES_H sys/stdtypes.h)
find_path(HAVE_XDBE_H Xdbe.h PATH_SUFFIXES X11/extensions extensions)

mark_as_advanced(HAVE_ALSA_ASOUNDLIB_H HAVE_DIRENT_H HAVE_DLFCN_H)
mark_as_advanced(HAVE_FREETYPE_H HAVE_GL_GL_H HAVE_GL_GLU_H)
mark_as_advanced(HAVE_LIBPNG_PNG_H HAVE_LOCALE_H HAVE_NDIR_H)
mark_as_advanced(HAVE_OPENGL_GLU_H HAVE_PNG_H HAVE_PTHREAD_H)
mark_as_advanced(HAVE_STDIO_H HAVE_STRINGS_H HAVE_SYS_DIR_H)
mark_as_advanced(HAVE_SYS_NDIR_H HAVE_SYS_SELECT_H)
mark_as_advanced(HAVE_SYS_STDTYPES_H HAVE_XDBE_H)

# where to find freetype headers
find_path(FREETYPE_PATH freetype/config/ftheader.h PATH_SUFFIXES freetype2)
if(FREETYPE_PATH)
   include_directories(${FREETYPE_PATH})
endif(FREETYPE_PATH)
mark_as_advanced(FREETYPE_PATH)

#######################################################################
# libraries
find_library(LIB_CAIRO cairo)
find_library(LIB_fontconfig fontconfig)
find_library(LIB_freetype freetype)
find_library(LIB_GL GL)
find_library(LIB_MesaGL MesaGL)
find_library(LIB_jpeg jpeg)
find_library(LIB_png png)
find_library(LIB_zlib z)

mark_as_advanced(LIB_CAIRO LIB_fontconfig LIB_freetype)
mark_as_advanced(LIB_GL LIB_MesaGL)
mark_as_advanced(LIB_jpeg LIB_png LIB_zlib)

#######################################################################
# functions
include(CheckFunctionExists)

if(HAVE_DLFCN_H)
   set(CMAKE_REQUIRED_LIBRARIES dl)
endif(HAVE_DLFCN_H)
CHECK_FUNCTION_EXISTS(dlsym                  HAVE_DLSYM)

CHECK_FUNCTION_EXISTS(localeconv             HAVE_LOCALECONV)

if(LIB_png)
   set(CMAKE_REQUIRED_LIBRARIES png)
endif(LIB_png)
CHECK_FUNCTION_EXISTS(png_get_valid          HAVE_PNG_GET_VALID)
CHECK_FUNCTION_EXISTS(png_set_tRNS_to_alpha  HAVE_PNG_SET_TRNS_TO_ALPHA)

CHECK_FUNCTION_EXISTS(scandir                HAVE_SCANDIR)
CHECK_FUNCTION_EXISTS(snprintf               HAVE_SNPRINTF)

# not really true but we convert strcasecmp calls to _stricmp calls in flstring.h
if(MSVC)
   set(HAVE_STRCASECMP 1)
endif(MSVC)
CHECK_FUNCTION_EXISTS(strcasecmp             HAVE_STRCASECMP)

CHECK_FUNCTION_EXISTS(strlcat                HAVE_STRLCAT)
CHECK_FUNCTION_EXISTS(strlcpy                HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS(strtoll                HAVE_STRTOLL)
CHECK_FUNCTION_EXISTS(vsnprintf              HAVE_VSNPRINTF)

set(CMAKE_REQUIRED_LIBRARIES)

if(HAVE_SCANDIR AND NOT HAVE_SCANDIR_POSIX)
   set(MSG "POSIX compatible scandir")
   message(STATUS "Looking for ${MSG}")
   try_compile(V
      ${FLTK_BINARY_DIR}
      ${FLTK_SOURCE_DIR}/CMake/posixScandir.cxx
      )
   if(V)
      message(STATUS "${MSG} - found")
      set(HAVE_SCANDIR_POSIX 1 CACHE INTERNAL "")
   else()
      message(STATUS "${MSG} - not found")
      set(HAVE_SCANDIR_POSIX HAVE_SCANDIR_POSIX-NOTFOUND)
   endif(V)
endif(HAVE_SCANDIR AND NOT HAVE_SCANDIR_POSIX)
mark_as_advanced(HAVE_SCANDIR_POSIX)

#######################################################################
# options
#######################################################################
set(OPTION_OPTIM ""
   CACHE STRING
   "custom optimization flags"
   )
add_definitions(${OPTION_OPTIM})

#######################################################################
set(OPTION_ARCHFLAGS ""
   CACHE STRING
   "custom architecture flags"
   )
add_definitions(${OPTION_ARCHFLAGS})

#######################################################################
set(OPTION_PREFIX_BIN ""
   CACHE STRING
   "where to install executables - leave empty to use {CMAKE_INSTALL_PREFIX}/bin"
   )

if(OPTION_PREFIX_BIN)
   set(PREFIX_BIN ${OPTION_PREFIX_BIN} CACHE INTERNAL "" FORCE)
else()
   set(PREFIX_BIN ${CMAKE_INSTALL_PREFIX}/bin CACHE INTERNAL "" FORCE)
endif(OPTION_PREFIX_BIN)

#######################################################################
set(OPTION_PREFIX_LIB ""
   CACHE STRING
   "where to install libraries - leave empty to use {CMAKE_INSTALL_PREFIX}/lib"
   )

if(OPTION_PREFIX_LIB)
   set(PREFIX_LIB ${OPTION_PREFIX_LIB} CACHE INTERNAL "" FORCE)
else()
   set(PREFIX_LIB ${CMAKE_INSTALL_PREFIX}/lib CACHE INTERNAL "" FORCE)
endif(OPTION_PREFIX_LIB)

#######################################################################
set(OPTION_PREFIX_INCLUDE ""
   CACHE STRING
   "where to install include files - leave empty to use {CMAKE_INSTALL_PREFIX}/include"
   )

if(OPTION_PREFIX_INCLUDE)
   set(PREFIX_INCLUDE ${OPTION_PREFIX_INCLUDE} CACHE INTERNAL "" FORCE)
else()
   set(PREFIX_INCLUDE ${CMAKE_INSTALL_PREFIX}/include CACHE INTERNAL "" FORCE)
endif(OPTION_PREFIX_INCLUDE)

#######################################################################
set(OPTION_PREFIX_DATA ""
   CACHE STRING
   "where to install data files - leave empty to use {CMAKE_INSTALL_PREFIX}/share/FLTK"
   )

if(OPTION_PREFIX_DATA)
   set(PREFIX_DATA ${OPTION_PREFIX_DATA} CACHE INTERNAL "" FORCE)
else()
   set(PREFIX_DATA ${CMAKE_INSTALL_PREFIX}/share/FLTK CACHE INTERNAL "" FORCE)
endif(OPTION_PREFIX_DATA)

#######################################################################
set(OPTION_PREFIX_DOC ""
   CACHE STRING
   "where to install doc files - leave empty to use {CMAKE_INSTALL_PREFIX}/share/doc/FLTK"
   )

if(OPTION_PREFIX_DOC)
   set(PREFIX_DOC ${OPTION_PREFIX_DOC} CACHE INTERNAL "" FORCE)
else()
   set(PREFIX_DOC ${CMAKE_INSTALL_PREFIX}/share/doc/FLTK CACHE INTERNAL "" FORCE)
endif(OPTION_PREFIX_DOC)

#######################################################################
if(WIN32)
   set(DEFAULT_CONFIG "${CMAKE_INSTALL_PREFIX}/CMake")
elseif(APPLE)
   set(DEFAULT_CONFIG "${CMAKE_INSTALL_PREFIX}/FLTK/.framework/Resources/CMake")
else()
   set(DEFAULT_CONFIG "${CMAKE_INSTALL_PREFIX}/lib/FLTK-${FLTK_VERSION}")
endif(WIN32)

set(OPTION_PREFIX_CONFIG ""
   CACHE STRING
   "where to install CMake config files - leave empty to use ${DEFAULT_CONFIG}"
   )

if(OPTION_PREFIX_CONFIG)
   set(PREFIX_CONFIG "${OPTION_PREFIX_CONFIG}" CACHE INTERNAL "" FORCE)
else()
   set(PREFIX_CONFIG "${DEFAULT_CONFIG}" CACHE INTERNAL "" FORCE)
endif(OPTION_PREFIX_CONFIG)

#######################################################################
if(CMAKE_HOST_UNIX)
   set(OPTION_PREFIX_MAN ""
      CACHE STRING
      "where to install man files - leave empty to use {CMAKE_INSTALL_PREFIX}/man"
      )

   if(OPTION_PREFIX_MAN)
      set(PREFIX_MAN ${OPTION_PREFIX_MAN} CACHE INTERNAL "" FORCE)
   else()
      set(PREFIX_MAN ${CMAKE_INSTALL_PREFIX}/man CACHE INTERNAL "" FORCE)
   endif(OPTION_PREFIX_MAN)

endif(CMAKE_HOST_UNIX)

#######################################################################
if(APPLE)
   option(OPTION_APPLE_X11 "use X11" OFF)
endif(APPLE)

if(NOT APPLE OR OPTION_APPLE_X11)
   include(FindX11)
   if(X11_FOUND)
      set(USE_X11 1)
      list(APPEND FLTK_CFLAGS -DUSE_X11)
      list(APPEND FLTK_LDLIBS -lX11)
   endif(X11_FOUND)
endif(NOT APPLE OR OPTION_APPLE_X11)

#######################################################################
option(OPTION_USE_POLL "use poll if available" OFF)

if(OPTION_USE_POLL)
   CHECK_FUNCTION_EXISTS(poll USE_POLL)
endif(OPTION_USE_POLL)

#######################################################################
option(OPTION_BUILD_SHARED_LIBS "Build shared libraries" OFF)

#######################################################################
option(OPTION_BUILD_EXAMPLES "build example programs" ON)

#######################################################################
include(FindPkgConfig)

option(OPTION_CAIRO "use lib Cairo" OFF)
option(OPTION_CAIROEXT
   "use fltk code instrumentation for cairo extended use" OFF
   )

if(OPTION_CAIRO OR OPTION_CAIROEXT AND LIB_CAIRO)
   pkg_search_module(PKG_CAIRO cairo)
endif(OPTION_CAIRO OR OPTION_CAIROEXT AND LIB_CAIRO)

if(PKG_CAIRO_FOUND)
   set(HAVE_CAIRO 1)
   add_subdirectory(cairo)
   list(APPEND FLTK_LDLIBS -lcairo -lpixman-1)
   include_directories(${PKG_CAIRO_INCLUDE_DIRS})
   string(REPLACE ";" " " CAIROFLAGS "${PKG_CAIRO_CFLAGS}")
endif(PKG_CAIRO_FOUND)

if(LIB_CAIRO AND OPTION_CAIROEXT AND PKG_CAIRO_FOUND)
   set(USE_CAIRO 1)
   set(FLTK_CAIRO_FOUND TRUE)
else()
   set(FLTK_CAIRO_FOUND FALSE)
endif(LIB_CAIRO AND OPTION_CAIROEXT AND PKG_CAIRO_FOUND)

#######################################################################
set(HAVE_GL LIB_GL OR LIB_MesaGL)

if(HAVE_GL)
   option(OPTION_USE_GL "use OpenGL" ON)
endif(HAVE_GL)

if(OPTION_USE_GL)
   include(FindOpenGL)
endif(OPTION_USE_GL)

if(OPENGL_FOUND)
   set(CMAKE_REQUIRED_INCLUDES ${OPENGL_INCLUDE_DIR}/GL)
   set(CMAKE_REQUIRED_LIBRARIES -lGLU -lGL)
   CHECK_FUNCTION_EXISTS(glXGetProcAddressARB HAVE_GLXGETPROCADDRESSARB)
   set(FLTK_GL_FOUND TRUE)
else()
   set(FLTK_GL_FOUND FALSE)
endif(OPENGL_FOUND)

#######################################################################
option(OPTION_USE_THREADS "use multi-threading" ON)

if(OPTION_USE_THREADS)
   include(FindThreads)
endif(OPTION_USE_THREADS)

if(OPTION_USE_THREADS AND CMAKE_HAVE_THREADS_LIBRARY)
   add_definitions("-D_THREAD_SAFE -D_REENTRANT")
   set(USE_THREADS 1)
   set(FLTK_THREADS_FOUND TRUE)
else()
   set(FLTK_THREADS_FOUND FALSE)
endif(OPTION_USE_THREADS AND CMAKE_HAVE_THREADS_LIBRARY)

if(OPTION_USE_THREADS AND CMAKE_USE_PTHREADS_INIT)
   set(HAVE_PTHREAD 1)
   list(APPEND FLTK_LDLIBS -lpthread)
   list(APPEND FLTK_CFLAGS -D_THREAD_SAFE -D_REENTRANT)
   set(FLTK_PTHREADS_FOUND TRUE)
else()
   set(HAVE_PTHREAD 0)
   set(HAVE_PTHREAD_H 0)
   set(FLTK_PTHREADS_FOUND FALSE)
endif(OPTION_USE_THREADS AND CMAKE_USE_PTHREADS_INIT)

#######################################################################
option(OPTION_LARGE_FILE "enable large file support" ON)

if(OPTION_LARGE_FILE)
   if(NOT MSVC)
      add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE)
      list(APPEND FLTK_CFLAGS -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE)
   endif(NOT MSVC)
endif(OPTION_LARGE_FILE)

#######################################################################
option(OPTION_USE_SYSTEM_LIBJPEG "Use's system libjpeg" ON)

if(OPTION_USE_SYSTEM_LIBJPEG AND LIB_jpeg)
   include(FindJPEG)
endif(OPTION_USE_SYSTEM_LIBJPEG AND LIB_jpeg)

if(OPTION_USE_SYSTEM_LIBJPEG AND JPEG_FOUND)
   set(FLTK_JPEG_LIBRARIES ${JPEG_LIBRARIES})
   include_directories(${JPEG_INCLUDE_DIR})
   set(FLTK_BUILTIN_JPEG_FOUND FALSE)
else()
   set(FLTK_JPEG_LIBRARIES fltk_jpeg)
   include_directories(${FLTK_SOURCE_DIR}/jpeg)
   set(FLTK_BUILTIN_JPEG_FOUND TRUE)
endif(OPTION_USE_SYSTEM_LIBJPEG AND JPEG_FOUND)

if(OPTION_USE_SYSTEM_LIBJPEG AND NOT JPEG_FOUND)
   message(STATUS "\ncannot find system jpeg library - using built-in\n")
endif(OPTION_USE_SYSTEM_LIBJPEG AND NOT JPEG_FOUND)

set(HAVE_LIBJPEG 1)

#######################################################################
option(OPTION_USE_SYSTEM_ZLIB "Use's system zlib" ON)

if(OPTION_USE_SYSTEM_ZLIB AND LIB_zlib)
   include(FindZLIB)
endif(OPTION_USE_SYSTEM_ZLIB AND LIB_zlib)

if(OPTION_USE_SYSTEM_ZLIB AND ZLIB_FOUND)
   set(FLTK_ZLIB_LIBRARIES ${ZLIB_LIBRARIES})
   include_directories(${ZLIB_INCLUDE_DIRS})
   set(FLTK_BUILTIN_ZLIB_FOUND FALSE)
else()
   set(FLTK_ZLIB_LIBRARIES fltk_z)
   set(ZLIB_INCLUDE_DIR ${FLTK_SOURCE_DIR}/zlib)
   include_directories(${FLTK_SOURCE_DIR}/zlib)
   set(FLTK_BUILTIN_ZLIB_FOUND TRUE)
endif(OPTION_USE_SYSTEM_ZLIB AND ZLIB_FOUND)

if(OPTION_USE_SYSTEM_ZLIB AND NOT ZLIB_FOUND)
   message(STATUS "\ncannot find system zlib library - using built-in\n")
endif(OPTION_USE_SYSTEM_ZLIB AND NOT ZLIB_FOUND)

set(HAVE_LIBZ 1)

#######################################################################
option(OPTION_USE_SYSTEM_LIBPNG "Use's system libpng" ON)

if(OPTION_USE_SYSTEM_LIBPNG AND LIB_png)
   include(FindPNG)
endif(OPTION_USE_SYSTEM_LIBPNG AND LIB_png)

if(OPTION_USE_SYSTEM_LIBPNG AND PNG_FOUND)
   set(FLTK_PNG_LIBRARIES ${PNG_LIBRARIES})
   include_directories(${PNG_INCLUDE_DIR})
   add_definitions(${PNG_DEFINITIONS})
   set(FLTK_BUILTIN_PNG_FOUND FALSE)
else()
   set(FLTK_PNG_LIBRARIES fltk_png)
   set(HAVE_PNG_H 1)
   set(HAVE_PNG_GET_VALID 1)
   set(HAVE_PNG_SET_TRNS_TO_ALPHA 1)
   include_directories(${FLTK_SOURCE_DIR}/png)
   set(FLTK_BUILTIN_PNG_FOUND TRUE)
endif(OPTION_USE_SYSTEM_LIBPNG AND PNG_FOUND)

if(OPTION_USE_SYSTEM_LIBPNG AND NOT PNG_FOUND)
   message(STATUS "\ncannot find system png library - using built-in\n")
endif(OPTION_USE_SYSTEM_LIBPNG AND NOT PNG_FOUND)

set(HAVE_LIBPNG 1)

#######################################################################
if(X11_Xinerama_FOUND)
   option(OPTION_USE_XINERAMA "use lib Xinerama" ON)
endif(X11_Xinerama_FOUND)

if(OPTION_USE_XINERAMA)
   set(HAVE_XINERAMA X11_Xinerama_FOUND)
   include_directories(${X11_Xinerama_INCLUDE_PATH})
   list(APPEND FLTK_LDLIBS -lXinerama)
   set(FLTK_XINERAMA_FOUND TRUE)
else()
   set(FLTK_XINERAMA_FOUND FALSE)
endif(OPTION_USE_XINERAMA)

#######################################################################
if(X11_Xft_FOUND)
   option(OPTION_USE_XFT "use lib Xft" ON)
endif(X11_Xft_FOUND)

if(OPTION_USE_XFT)
   set(USE_XFT X11_Xft_FOUND)
   list(APPEND FLTK_LDLIBS -lXft)
   set(FLTK_XFT_FOUND TRUE)
else()   
   set(FLTK_XFT_FOUND FALSE)
endif(OPTION_USE_XFT)

#######################################################################
if(X11_FOUND)
   option(OPTION_USE_XDBE "use lib Xdbe" ON)
endif(X11_FOUND)

if(OPTION_USE_XDBE AND HAVE_XDBE_H)
   set(HAVE_XDBE 1)
   set(FLTK_XDBE_FOUND TRUE)
else()
   set(FLTK_XDBE_FOUND FALSE)
endif(OPTION_USE_XDBE AND HAVE_XDBE_H)

#######################################################################
# final setup and config
#######################################################################
# Set the fluid executable path
if(CMAKE_CROSSCOMPILING)
   find_file(FLUID_PATH
      NAMES fluid fluid.exe
      PATHS ENV PATH
      NO_CMAKE_FIND_ROOT_PATH
   )
   add_executable(fluid IMPORTED)
   set(FLTK_FLUID_EXECUTABLE fluid)
   set(FLTK_FLUID_PATH ${FLUID_PATH})
   set_target_properties(fluid
      PROPERTIES IMPORTED_LOCATION ${FLUID_PATH}
      )
else()
   set(FLTK_FLUID_EXECUTABLE fluid)
   set(FLTK_FLUID_PATH ${EXECUTABLE_OUTPUT_PATH}/${FLTK_FLUID_EXECUTABLE})
endif(CMAKE_CROSSCOMPILING)

add_subdirectory(src)

# generate FLTKConfig.cmake
string(REPLACE ";" " " EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
configure_file(${FLTK_SOURCE_DIR}/CMake/FLTKConfig.cmake.in
   ${EXECUTABLE_OUTPUT_PATH}/FLTKConfig.cmake
   @ONLY
   )

# generate UseFLTK.cmake
configure_file(${FLTK_SOURCE_DIR}/CMake/UseFLTK.cmake.in
   ${EXECUTABLE_OUTPUT_PATH}/UseFLTK.cmake
   @ONLY
   )

# generate config.h
configure_file("${FLTK_SOURCE_DIR}/configh.cmake.in"
  "${FLTK_BINARY_DIR}/config.h" @ONLY
   )

# generate fltk-config
get_filename_component(CC ${CMAKE_C_COMPILER} NAME)
get_filename_component(CXX ${CMAKE_CXX_COMPILER} NAME)

string(REPLACE ";" " " C_FLAGS "${FLTK_CFLAGS}")

if(X11_Xext_FOUND)
   list(APPEND FLTK_LDLIBS -lXext)
endif(X11_Xext_FOUND)
string(REPLACE ";" " " LD_LIBS "${FLTK_LDLIBS}")

configure_file(${FLTK_SOURCE_DIR}/fltk-config.cmake.in
   ${FLTK_BINARY_DIR}/fltk-config
   )
if(UNIX)
   execute_process(COMMAND chmod 755 fltk-config
      WORKING_DIRECTORY ${FLTK_BINARY_DIR}
   )
endif(UNIX)

# generate uninstall target
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  @ONLY
   )
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  )

#######################################################################
# build examples - these have to be after fluid is built/imported
#######################################################################
if(OPTION_BUILD_EXAMPLES)
   add_subdirectory(test)
endif(OPTION_BUILD_EXAMPLES)

#######################################################################
# installation
#######################################################################

install(DIRECTORY ${FLTK_BINARY_DIR}/FL
   DESTINATION ${PREFIX_INCLUDE} USE_SOURCE_PERMISSIONS
   )

install(DIRECTORY ${FLTK_BINARY_DIR}/Fl
   DESTINATION ${PREFIX_INCLUDE} USE_SOURCE_PERMISSIONS
   )

install(PROGRAMS ${FLTK_BINARY_DIR}/fltk-config
   DESTINATION ${PREFIX_BIN}
   OPTIONAL
   )

install(EXPORT fltk-install
   DESTINATION ${PREFIX_CONFIG}
   FILE FLTKLibraries.cmake
   )

install(FILES ${EXECUTABLE_OUTPUT_PATH}/FLTKConfig.cmake
   DESTINATION ${PREFIX_CONFIG}
   )

install(FILES ${EXECUTABLE_OUTPUT_PATH}/UseFLTK.cmake
   DESTINATION ${PREFIX_CONFIG}
   )

if(CMAKE_HOST_UNIX)
   macro(INSTALL_MAN FILE LEVEL)
   install(FILES
      ${FLTK_SOURCE_DIR}/documentation/src/${FILE}.man
      DESTINATION ${PREFIX_MAN}/man${LEVEL}
      RENAME ${FILE}.${LEVEL}
      )
   endmacro(INSTALL_MAN FILE LEVEL)

   INSTALL_MAN(fluid 1)
   INSTALL_MAN(fltk-config 1)
   INSTALL_MAN(fltk 3)
   INSTALL_MAN(blocks 6)
   INSTALL_MAN(checkers 6)
   INSTALL_MAN(sudoku 6)

endif(CMAKE_HOST_UNIX)

