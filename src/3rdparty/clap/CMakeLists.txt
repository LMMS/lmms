if(LMMS_HAVE_CLAP)
    add_library(clap INTERFACE)
    target_include_directories(clap INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/clap/include)
endif()
