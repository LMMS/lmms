FILE(GLOB LILV_SOURCES lilv/src/*.c)

add_library(lilv STATIC
    lilv/src/lilv_internal.h
    lilv/src/zix/common.h
    lilv/src/zix/tree.h
    lilv/src/zix/tree.c
    ${LILV_SOURCES}
)

target_include_directories(lilv PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/lilv
	${CMAKE_CURRENT_SOURCE_DIR}/lilv/src
	${CMAKE_CURRENT_SOURCE_DIR}/lilv/build
    ${GTK2_INCLUDE_DIRS}
    ${SORD_INCLUDE_DIRS}
)

target_link_libraries(lilv
    ${GTK2_LIBRARIES}
    serd
    sratom
    ${SORD_LIBRARIES}
)
