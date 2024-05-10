include(BuildPlugin)
build_plugin(malletsstk
	Mallets.cpp Mallets.h
	MOCFILES Mallets.h
	EMBEDDED_RESOURCES artwork.png logo.png
)
target_link_libraries(malletsstk unofficial::libstk::libstk)
