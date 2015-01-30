#for tests looking for files stored in the source dir
add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

CXXTEST_ADD_TEST(ControllerTest ControllerTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/ControllerTest.h)
CXXTEST_ADD_TEST(EchoTest EchoTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/EchoTest.h)
#CXXTEST_ADD_TEST(SampleTest SampleTest.h)
CXXTEST_ADD_TEST(MicrotonalTest MicrotonalTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/MicrotonalTest.h)
CXXTEST_ADD_TEST(XMLwrapperTest XMLwrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/XMLwrapperTest.h)
CXXTEST_ADD_TEST(ADnoteTest AdNoteTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AdNoteTest.h)
CXXTEST_ADD_TEST(SUBnoteTest SubNoteTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/SubNoteTest.h)
CXXTEST_ADD_TEST(OscilGenTest OscilGenTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/OscilGenTest.h)
CXXTEST_ADD_TEST(RandTest RandTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/RandTest.h)
CXXTEST_ADD_TEST(PADnoteTest PadNoteTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/PadNoteTest.h)
CXXTEST_ADD_TEST(PluginTest PluginTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/PluginTest.h)
CXXTEST_ADD_TEST(UnisonTest UnisonTest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/UnisonTest.h)

#Extra libraries added to make test and full compilation use the same library
#links for quirky compilers
set(test_lib zynaddsubfx_core ${ZLIB_LIBRARY} ${FFTW_LIBRARIES} ${MXML_LIBRARIES} pthread)

message(STATUS "Linking tests with: ${test_lib}")
target_link_libraries(ADnoteTest     ${test_lib})
target_link_libraries(SUBnoteTest    ${test_lib})
target_link_libraries(ControllerTest ${test_lib})
target_link_libraries(EchoTest       ${test_lib})
target_link_libraries(MicrotonalTest ${test_lib})
target_link_libraries(OscilGenTest   ${test_lib})
target_link_libraries(XMLwrapperTest ${test_lib})
target_link_libraries(RandTest       ${test_lib})
target_link_libraries(PADnoteTest    ${test_lib})
target_link_libraries(PluginTest     zynaddsubfx_core zynaddsubfx_nio
    ${OS_LIBRARIES} ${AUDIO_LIBRARIES})
target_link_libraries(UnisonTest    ${test_lib})

