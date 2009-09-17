set(zynaddsubfx_samples_SRCS
	AuSample.cpp
	FqSample.cpp
	Sample.cpp
)

add_library(zynaddsubfx_samples STATIC
	${zynaddsubfx_samples_SRCS} 
	)

target_link_libraries(zynaddsubfx_samples 
	zynaddsubfx_samples
	)
