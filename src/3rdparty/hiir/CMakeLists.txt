add_library(hiir STATIC hiir/PolyphaseIir2Designer.cpp)
target_include_directories(hiir PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(hiir PROPERTIES
	CXX_STANDARD 17
)
