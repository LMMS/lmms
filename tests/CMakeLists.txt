INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include")
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}")
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/src")

SET(CMAKE_CXX_STANDARD 14)

FIND_PACKAGE(Qt5Test)

SET(CMAKE_AUTOMOC ON)

# FIXME: remove this once we export include directories for LMMS
IF(LMMS_BUILD_APPLE)
INCLUDE_DIRECTORIES("/usr/local/include")
ENDIF()

ADD_EXECUTABLE(tests
	EXCLUDE_FROM_ALL
	main.cpp
	QTestSuite

	src/core/AutomatableModelTest.cpp
	src/core/ProjectVersionTest.cpp
	src/core/RelativePathsTest.cpp
	src/core/MemoryPoolTest.cpp

	src/tracks/AutomationTrackTest.cpp
)
TARGET_LINK_LIBRARIES(tests lmmslib Qt5::Test)
