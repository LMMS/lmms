INCLUDE(BuildPlugin)

BUILD_PLUGIN(ladspabrowser ladspa_browser.cpp ladspa_browser.h ladspa_description.cpp ladspa_description.h ladspa_port_dialog.cpp ladspa_port_dialog.h MOCFILES ladspa_browser.h ladspa_description.h ladspa_port_dialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
