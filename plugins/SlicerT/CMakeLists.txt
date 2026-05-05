include(BuildPlugin)

include_directories(SYSTEM ${FFTW3F_INCLUDE_DIRS})
link_libraries(${FFTW3F_LIBRARIES})

build_plugin(slicert
	SlicerT.cpp
	SlicerT.h
	SlicerTView.cpp
	SlicerTView.h
	SlicerTWaveform.cpp
	SlicerTWaveform.h
	MOCFILES SlicerT.h SlicerTView.h SlicerTWaveform.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
target_link_libraries(slicert SampleRate::samplerate)
