set_directory_properties(PROPERTIES SYSTEM TRUE)

if(LMMS_BUILD_LINUX AND LMMS_HAVE_VST AND NOT WANT_QT6)
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory(qt5-x11embed)
ENDIF()

add_library(jack_headers INTERFACE)
target_include_directories(jack_headers INTERFACE jack2/common)

ADD_SUBDIRECTORY(hiir)
ADD_SUBDIRECTORY(weakjack)

# The lockless ring buffer library is linked as part of the core
add_library(ringbuffer OBJECT
	ringbuffer/src/lib/ringbuffer.cpp
)
target_compile_features(ringbuffer PUBLIC cxx_std_20)
target_include_directories(ringbuffer PUBLIC
	ringbuffer/include
	"${CMAKE_CURRENT_BINARY_DIR}"
)
# Create a dummy ringbuffer_export.h, since ringbuffer is not compiled as a library
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ringbuffer_export.h.in" [[
	#include "${CMAKE_BINARY_DIR}/src/lmms_export.h"
	#define RINGBUFFER_EXPORT LMMS_EXPORT
]])
configure_file("${CMAKE_CURRENT_BINARY_DIR}/ringbuffer_export.h.in" ringbuffer_export.h)
target_compile_definitions(ringbuffer PRIVATE lmmsobjs_EXPORTS)
# Enable MLOCK support for ringbuffer if available
INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES(sys/mman.h HAVE_SYS_MMAN)
IF(HAVE_SYS_MMAN AND NOT CMAKE_SYSTEM_NAME MATCHES "Haiku")
    SET(USE_MLOCK ON)
ELSE()
    SET(USE_MLOCK OFF)
ENDIF()
# Generate ringbuffer configuration headers
configure_file(ringbuffer/src/ringbuffer-config.h.in  ringbuffer-config.h)
configure_file(ringbuffer/src/ringbuffer-version.h.in ringbuffer-version.h)

IF("MidiImport" IN_LIST PLUGIN_LIST OR "MidiExport" IN_LIST PLUGIN_LIST)
	# PortSMF defaults to a shared library and inherits BUILD_TESTING from LMMS.
	# LMMS links it into two plugin modules and relies on PortSMF's upstream tests.
	SET(BUILD_SHARED_LIBS OFF)
	SET(BUILD_TESTING OFF)
	SET(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
	ADD_SUBDIRECTORY(portsmf EXCLUDE_FROM_ALL)
	SET_TARGET_PROPERTIES(PortSMF PROPERTIES POSITION_INDEPENDENT_CODE ON)

	IF(NOT MSVC)
		# PortSMF raises these warnings, which will prevent compilation with -Werror.
		TARGET_COMPILE_OPTIONS(PortSMF PRIVATE
			-Wno-error=unknown-pragmas
			-Wno-error=maybe-uninitialized
			-Wno-error=restrict)
	ENDIF()
ENDIF()
