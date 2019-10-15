find_library(LIB_PTHREAD pthread)
find_library(LIB_GTEST gtest)

FIND_PROGRAM( GCOV_PATH gcov )
FIND_PROGRAM( LCOV_PATH lcov )
FIND_PROGRAM( GENHTML_PATH genhtml )
FIND_PROGRAM( GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/tests)

file(GLOB TEST_SRC_FILES "*.cpp")
file(GLOB SRC_FILES "../MultidimensionalNeighbourMap.hpp")

set(CXX_TESTS_FLAGS "-g -O0 --coverage -fno-elide-constructors -fprofile-arcs -ftest-coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
set(C_TEST_FLAGS "-g -O0 --coverage -fno-elide-constructors -fprofile-arcs -ftest-coverage -fno-inline -fno-inline-small-functions -fno-default-inline")

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_TESTS_FLAGS}")
SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${C_TEST_FLAGS}")

add_executable(diginstrument_test ${TEST_SRC_FILES} ${SRC_FILES})
set_target_properties(diginstrument_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_include_directories(diginstrument_test PRIVATE "../")
set_property(TARGET diginstrument_test PROPERTY CXX_STANDARD 14)

target_link_libraries(diginstrument_test "${LIB_GTEST}" "${LIB_PTHREAD}")

add_custom_target(diginstrument_test_coverage
DEPENDS diginstrument_test
COMMAND echo "Creating Coverage Info"
COMMAND ${CMAKE_BINARY_DIR}/plugins/diginstrument_test -s
COMMAND mkdir -p ${PROJECT_SOURCE_DIR}/doc/diginstrument_test
COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR}/plugins/Diginstrument/test/CMakeFiles/diginstrument_test.dir/ --base-directory ${PROJECT_SOURCE_DIR} --no-external --capture --output-file ${PROJECT_SOURCE_DIR}/doc/diginstrument_test/diginstrument_test_coverage.info -rc lcov_branch_coverage=1
COMMAND genhtml ${PROJECT_SOURCE_DIR}/doc/diginstrument_test/diginstrument_test_coverage.info --output-directory ${PROJECT_SOURCE_DIR}/doc/diginstrument_test_coverage
)