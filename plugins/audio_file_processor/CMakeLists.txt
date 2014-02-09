INCLUDE(BuildPlugin)

BUILD_PLUGIN(audiofileprocessor audio_file_processor.cpp audio_file_processor.h MOCFILES audio_file_processor.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
