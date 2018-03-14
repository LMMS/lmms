add_library(sratom STATIC
	sratom/sratom/sratom.h
	sratom/src/sratom.c
)

target_include_directories(sratom PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/sratom
	${CMAKE_CURRENT_SOURCE_DIR}/sratom/build
    ${SORD_INCLUDE_DIRS}
)

target_link_libraries(sratom
    serd
    ${SORD_LIBRARIES}
)
