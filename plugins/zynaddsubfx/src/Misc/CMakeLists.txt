include_directories(${MXML_INCLUDE_DIR})

set(zynaddsubfx_misc_SRCS
	Bank.cpp
	Config.cpp
	Dump.cpp
	Master.cpp
	Microtonal.cpp
	Part.cpp
	Util.cpp
	QtXmlWrapper.cpp
)

if (LASH_FOUND)
    set(zynaddsubfx_misc_SRCS ${zynaddsubfx_misc_SRCS} LASHClient.cpp)
endif()

add_library(zynaddsubfx_misc STATIC
	${zynaddsubfx_misc_SRCS} 
	)

target_link_libraries(zynaddsubfx_misc zynaddsubfx_output)
