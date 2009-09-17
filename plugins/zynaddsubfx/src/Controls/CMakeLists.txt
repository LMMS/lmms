set(zynaddsubfx_controls_SRCS
	Control.cpp
	DelayCtl.cpp
)

add_library(zynaddsubfx_controls STATIC
	${zynaddsubfx_controls_SRCS} 
	)
