INCLUDE(BuildPlugin)

BUILD_PLUGIN(dynamicsprocessor dynamics_processor.cpp dynamics_processor_controls.cpp dynamics_processor_control_dialog.cpp MOCFILES dynamics_processor_controls.h dynamics_processor_control_dialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
