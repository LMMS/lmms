add_library(hiir INTERFACE)
target_include_directories(hiir INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(hiir PROPERTIES
	CXX_STANDARD 17
)
