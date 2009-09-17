set(zynaddsubfx_seq_SRCS
	MIDIEvents.cpp
	MIDIFile.cpp
	Sequencer.cpp
)

add_library(zynaddsubfx_seq STATIC
	${zynaddsubfx_seq_SRCS} 
	)

target_link_libraries(zynaddsubfx_seq)
