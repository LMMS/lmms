IF(LMMS_BUILD_LINUX AND WANT_VST)
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory(qt5-x11embed)
ENDIF()

ADD_SUBDIRECTORY(hiir)
ADD_SUBDIRECTORY(rpmalloc)
ADD_SUBDIRECTORY(weakjack)
ADD_SUBDIRECTORY(clap)

if(MINGW)
	option(MINGW_STDTHREADS_GENERATE_STDHEADERS "" ON)
	add_subdirectory(mingw-std-threads)
	set(LMMS_USE_MINGW_STD_THREADS ON PARENT_SCOPE)
endif()

# The lockless ring buffer library is compiled as part of the core
SET(RINGBUFFER_DIR "${CMAKE_SOURCE_DIR}/src/3rdparty/ringbuffer/")
SET(RINGBUFFER_DIR ${RINGBUFFER_DIR} PARENT_SCOPE)
# Create a dummy ringbuffer_export.h, since ringbuffer is not compiled as a library
FILE(WRITE ${CMAKE_BINARY_DIR}/src/ringbuffer_export.h
	"#include \"${CMAKE_BINARY_DIR}/src/lmms_export.h\"\n
	#define RINGBUFFER_EXPORT LMMS_EXPORT")
# Enable MLOCK support for ringbuffer if available
INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES(sys/mman.h HAVE_SYS_MMAN)
IF(HAVE_SYS_MMAN AND NOT CMAKE_SYSTEM_NAME MATCHES "Haiku")
    SET(USE_MLOCK ON)
ELSE()
    SET(USE_MLOCK OFF)
ENDIF()
# Generate ringbuffer configuration headers
CONFIGURE_FILE(${RINGBUFFER_DIR}/src/ringbuffer-config.h.in  ${CMAKE_BINARY_DIR}/src/ringbuffer-config.h)
CONFIGURE_FILE(${RINGBUFFER_DIR}/src/ringbuffer-version.h.in ${CMAKE_BINARY_DIR}/src/ringbuffer-version.h)
