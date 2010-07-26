set(zynaddsubfx_synth_SRCS
	ADnote.cpp
	Envelope.cpp
	LFO.cpp
	OscilGen.cpp
	PADnote.cpp
	Resonance.cpp
	SUBnote.cpp
)

add_library(zynaddsubfx_synth STATIC
	${zynaddsubfx_synth_SRCS} 
	)

target_link_libraries(zynaddsubfx_synth)

