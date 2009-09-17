set(zynaddsubfx_params_SRCS
	ADnoteParameters.cpp
	Controller.cpp
	EnvelopeParams.cpp
	FilterParams.cpp
	LFOParams.cpp
	PADnoteParameters.cpp
	Presets.cpp
	PresetsStore.cpp
	SUBnoteParameters.cpp

)

add_library(zynaddsubfx_params STATIC
	${zynaddsubfx_params_SRCS} 
	)

target_link_libraries(zynaddsubfx_params)# ${ASOUND_LIBRARY})
