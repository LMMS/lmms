set(CMAKE_C_FLAGS "-std=c11")

add_library(rpmalloc STATIC
	rpmalloc/rpmalloc/rpmalloc.c
	rpmalloc/rpmalloc/rpmalloc.h
)

target_include_directories(rpmalloc PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/rpmalloc/rpmalloc
)

if (NOT LMMS_BUILD_WIN32)
	target_compile_definitions(rpmalloc
		PRIVATE -D_GNU_SOURCE
	)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(rpmalloc
		PRIVATE -DENABLE_ASSERTS -DENABLE_VALIDATE_ARGS
	)
endif()

option(LMMS_ENABLE_MALLOC_STATS "Enables statistics for rpmalloc" OFF)

if (LMMS_ENABLE_MALLOC_STATS)
	target_compile_definitions(rpmalloc
		PRIVATE -DENABLE_STATISTICS
	)
endif()
