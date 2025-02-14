SET(CMAKE_AUTOMOC ON)

ADD_EXECUTABLE(benchmarks
	EXCLUDE_FROM_ALL
	benchmark.cpp
)

# TODO replace usages of include_directories in src/CMakeLists.txt with target_include_directories
# and remove this line (also in tests/CMakeLists.txt)
target_include_directories(benchmarks PRIVATE $<TARGET_PROPERTY:lmmsobjs,INCLUDE_DIRECTORIES>)

target_static_libraries(benchmarks PRIVATE lmmsobjs)
target_compile_features(benchmarks PRIVATE cxx_std_17)
