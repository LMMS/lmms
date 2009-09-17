set(zynaddsubfx_input_SRCS
	MidiIn.cpp 
	NULLMidiIn.cpp 
	#OSSMidiIn.cpp #[TODO] get OSS midi detection and 
	#WINMidiIn.cpp #       Win midi detection working
)

if(AlsaMidiInput)
	set(zynaddsubfx_input_SRCS
		${zynaddsubfx_input_SRCS}
		ALSAMidiIn.cpp
		)
	message(STATUS "Alsa midi input enabled")
	set(MIDIINPUT_LIBRARIES ${ASOUND_LIBRARY} PARENT_SCOPE)
endif(AlsaMidiInput)

add_library(zynaddsubfx_input STATIC
	${zynaddsubfx_input_SRCS} 
	)
