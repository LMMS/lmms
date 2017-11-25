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
	# rpmalloc uses GCC builtin "__builtin_umull_overflow" with ENABLE_VALIDATE_ARGS,
	# which is only available starting with GCC 5
	if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 5)
		set(ENABLE_VALIDATE_ARGS OFF)
	else ()
		set(ENABLE_VALIDATE_ARGS ON)
	endif()
	target_compile_definitions(rpmalloc
		PRIVATE -DENABLE_ASSERTS=1 -DENABLE_VALIDATE_ARGS=${ENABLE_VALIDATE_ARGS}
	)
endif()

option(LMMS_ENABLE_MALLOC_STATS "Enables statistics for rpmalloc" OFF)

if (LMMS_ENABLE_MALLOC_STATS)
	target_compile_definitions(rpmalloc
		PRIVATE -DENABLE_STATISTICS=1
	)
endif()
