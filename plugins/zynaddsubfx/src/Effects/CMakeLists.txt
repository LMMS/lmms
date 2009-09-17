set(zynaddsubfx_effect_SRCS
	Alienwah.cpp
	Chorus.cpp
	Distorsion.cpp
	DynamicFilter.cpp
	Echo.cpp
	Effect.cpp
	EffectLFO.cpp
	EffectMgr.cpp
	EQ.cpp
	Phaser.cpp
	Reverb.cpp
)

add_library(zynaddsubfx_effect STATIC
	${zynaddsubfx_effect_SRCS} 
	)

target_link_libraries(zynaddsubfx_effect)
