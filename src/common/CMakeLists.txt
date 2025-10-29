set(COMMON_SRCS
	RemotePluginBase.cpp
	SharedMemory.cpp
	SystemSemaphore.cpp
)

foreach(SRC ${COMMON_SRCS})
	list(APPEND LMMS_COMMON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
endforeach()

set(LMMS_COMMON_SRCS ${LMMS_COMMON_SRCS} PARENT_SCOPE)
