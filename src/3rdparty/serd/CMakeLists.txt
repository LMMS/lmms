FILE(GLOB SERD_SOURCES serd/src/*.c)

add_library(serd STATIC
	serd/serd/serd.h
    serd/src/reader.h
    serd/src/serd_internal.h
    ${SERD_SOURCES}
)

target_include_directories(serd PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/serd
	${CMAKE_CURRENT_SOURCE_DIR}/serd/build
)
