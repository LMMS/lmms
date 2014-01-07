INCLUDE(BuildPlugin)

BUILD_PLUGIN(OPL2 opl2instrument.cpp opl2instrument.h opl.h kemuopl.h adlibemu.c adlibemu.h fmopl.c fmopl.h temuopl.cpp temuopl.h MOCFILES opl2instrument.h EMBEDDED_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.png)