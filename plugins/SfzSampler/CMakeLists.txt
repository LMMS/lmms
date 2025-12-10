include(BuildPlugin)

build_plugin(sfzsampler
	SfzSampler.cpp
	SfzSampler.h
	SfzSamplerView.cpp
	SfzSamplerView.h
	MOCFILES SfzSampler.h SfzSamplerView.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
