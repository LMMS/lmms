include(BuildPlugin)

build_plugin(watsyn
	Watsyn.cpp Watsyn.h
	MOCFILES Watsyn.h
	EMBEDDED_RESOURCES *.png
)
target_link_libraries(watsyn SampleRate::samplerate)
