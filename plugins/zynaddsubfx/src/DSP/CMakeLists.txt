set(zynaddsubfx_dsp_SRCS
	AnalogFilter.cpp
	FFTwrapper.cpp
	Filter.cpp
	FormantFilter.cpp
	SVFilter.cpp
)

add_library(zynaddsubfx_dsp STATIC
	${zynaddsubfx_dsp_SRCS} 
	)

target_link_libraries(zynaddsubfx_dsp)
