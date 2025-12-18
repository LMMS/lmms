include(BuildPlugin)

build_plugin(sfzsampler
	SfzSampler.cpp
	SfzSampler.h
	SfzSamplerView.cpp
	SfzSamplerView.h
	SfzOpcodes.h
	SfzOpcodeState.cpp
	SfzOpcodeState.h
	SfzRegion.cpp
	SfzRegion.h
	SfzParser.cpp
	SfzParser.h
	SfzRegionPlayState.cpp
	SfzRegionPlayState.h
	MOCFILES
	SfzSampler.h
	SfzSamplerView.h
	SfzOpcodes.h
	SfzOpcodeState.h
	SfzRegion.h
	SfzParser.h
	SfzRegionPlayState.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
