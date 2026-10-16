include(CTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(LMMS_TESTS
	src/core/ArrayVectorTest.cpp
	src/core/AudioBufferTest.cpp
	src/core/AutomatableModelTest.cpp
	src/core/MathTest.cpp
	src/core/ProjectVersionTest.cpp
	src/core/RelativePathsTest.cpp
	src/core/TimelineTest.cpp
	src/tracks/AutomationTrackTest.cpp
	src/gui/DpiRasterTest.cpp
	src/gui/EmbedDpiTest.cpp
	src/gui/DpiCacheTest.cpp
	src/gui/WindowGeometryTest.cpp
	src/gui/DpiWidgetTest.cpp
)

foreach(LMMS_TEST_SRC IN LISTS LMMS_TESTS)
	# TODO CMake 3.20: Use cmake_path
	get_filename_component(LMMS_TEST_NAME ${LMMS_TEST_SRC} NAME_WE)

	add_executable(${LMMS_TEST_NAME} ${LMMS_TEST_SRC})
	add_test(NAME ${LMMS_TEST_NAME} COMMAND ${LMMS_TEST_NAME} -o -,txt)
	if(LMMS_TEST_SRC MATCHES "src/gui/")
		set_tests_properties(${LMMS_TEST_NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
		foreach(DPI_SCALE 1 1.25 1.5 1.5625 1.75 2)
			add_test(NAME ${LMMS_TEST_NAME}_${DPI_SCALE} COMMAND ${LMMS_TEST_NAME} -o -,txt)
			set_tests_properties(${LMMS_TEST_NAME}_${DPI_SCALE} PROPERTIES
				ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=${DPI_SCALE};QT_AUTO_SCREEN_SCALE_FACTOR=0;QT_ENABLE_HIGHDPI_SCALING=1;QT_SCALE_FACTOR_ROUNDING_POLICY=PassThrough")
		endforeach()
	endif()

	# TODO CMake 3.12: Propagate usage requirements by linking to lmmsobjs
	target_include_directories(${LMMS_TEST_NAME} PRIVATE $<TARGET_PROPERTY:lmmsobjs,INCLUDE_DIRECTORIES>)

	target_static_libraries("${LMMS_TEST_NAME}" PRIVATE lmmsobjs)
	target_link_libraries(${LMMS_TEST_NAME} PRIVATE
		${QT_LIBRARIES}
		${QT_QTTEST_LIBRARY}
	)

	target_compile_features(${LMMS_TEST_NAME} PRIVATE cxx_std_20)
	target_compile_definitions(${LMMS_TEST_NAME} PRIVATE LMMS_TESTING)
endforeach()
