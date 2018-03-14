FILE(GLOB LV2_INTERNALS_SOURCES ./*.h ./*.c ./*.cpp **/*.h **/*.c **/*.cpp)
add_library(lv2internals STATIC
    ${LV2_INTERNALS_SOURCES}
)

target_include_directories(lv2internals PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lv2internals
    lilv
    suil
)

set_target_properties(lv2internals PROPERTIES LINKER_LANGUAGE CXX)
