set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEBUG "")

IF(LMMS_BUILD_LINUX AND WANT_VST)
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory(qt5-x11embed)
ENDIF()

ADD_SUBDIRECTORY(rpmalloc)
ADD_SUBDIRECTORY(weakjack)

IF(WIN32 AND CMAKE_COMPILER_IS_GNUCXX)
	SET(MINGW TRUE)
ENDIF()

IF(MINGW)
	# Work around linking errors with MinGW
	SET(CDS_LIBRARY_TYPE SHARED)
ELSE()
	SET(CDS_LIBRARY_TYPE STATIC)
ENDIF()

ADD_LIBRARY(cds ${CDS_LIBRARY_TYPE}
	libcds/src/init.cpp
	libcds/src/hp.cpp
	libcds/src/dhp.cpp
	libcds/src/urcu_gp.cpp
	libcds/src/urcu_sh.cpp
	libcds/src/thread_data.cpp
	libcds/src/topology_hpux.cpp
	libcds/src/topology_linux.cpp
	libcds/src/topology_osx.cpp
	libcds/src/dllmain.cpp
)
SET_TARGET_PROPERTIES(cds PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED ON
)
TARGET_INCLUDE_DIRECTORIES(cds
	PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libcds"
)

IF(NEED_MINGW_THREADS_REPLACEMENT)
	# Provide win32 threads implementation
	TARGET_INCLUDE_DIRECTORIES(cds BEFORE
		PRIVATE "${CMAKE_SOURCE_DIR}/include/mingw-std-threads"
		PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/mingw-std-threads"
	)
ENDIF()

IF(CDS_LIBRARY_TYPE STREQUAL "STATIC")
	TARGET_COMPILE_DEFINITIONS(cds
		PUBLIC CDS_BUILD_STATIC_LIB
	)
ELSE()
	TARGET_COMPILE_DEFINITIONS(cds
		PRIVATE CDS_BUILD_LIB
	)
	# Install DLL
	install(TARGETS cds RUNTIME DESTINATION .)
ENDIF()

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
IF(HAVE_SYS_MMAN)
    SET(USE_MLOCK ON)
ELSE()
    SET(USE_MLOCK OFF)
ENDIF()
# Generate ringbuffer configuration headers
CONFIGURE_FILE(${RINGBUFFER_DIR}/src/ringbuffer-config.h.in  ${CMAKE_BINARY_DIR}/src/ringbuffer-config.h)
CONFIGURE_FILE(${RINGBUFFER_DIR}/src/ringbuffer-version.h.in ${CMAKE_BINARY_DIR}/src/ringbuffer-version.h)
