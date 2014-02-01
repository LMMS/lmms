INCLUDE(BuildPlugin)

BUILD_PLUGIN(papu papu_instrument.cpp papu_instrument.h Basic_Gb_Apu.cpp Basic_Gb_Apu.h gb_apu/Gb_Oscs.cpp gb_apu/Gb_Apu.h gb_apu/Blip_Buffer.cpp gb_apu/Gb_Apu.cpp gb_apu/Gb_Oscs.h gb_apu/blargg_common.h gb_apu/Blip_Buffer.h gb_apu/Multi_Buffer.cpp gb_apu/blargg_source.h gb_apu/Blip_Synth.h gb_apu/Multi_Buffer.h MOCFILES papu_instrument.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
