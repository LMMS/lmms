include_directories(${MXML_INCLUDE_DIR})

set(zynaddsubfx_misc_SRCS
	Misc/Bank.cpp
	Misc/Config.cpp
	Misc/Dump.cpp
	Misc/Master.cpp
	Misc/Microtonal.cpp
	Misc/Part.cpp
	Misc/Util.cpp
	Misc/XMLwrapper.cpp
	Misc/Recorder.cpp
	Misc/WavFile.cpp
	Misc/WaveShapeSmps.cpp
)



if(LashEnable)
	set(zynaddsubfx_misc_SRCS
		${zynaddsubfx_misc_SRCS}
		Misc/LASHClient.cpp
		PARENT_SCOPE)
else()
	set(zynaddsubfx_misc_SRCS
		${zynaddsubfx_misc_SRCS}
		PARENT_SCOPE)
endif()
