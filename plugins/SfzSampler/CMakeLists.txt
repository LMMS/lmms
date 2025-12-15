include(BuildPlugin)

build_plugin(sfzsampler
	SfzSampler.cpp
	SfzSampler.h
	SfzSamplerView.cpp
	SfzSamplerView.h
	SfzFormat.cpp
	SfzFormat.h
	MOCFILES SfzSampler.h SfzSamplerView.h SfzFormat.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
