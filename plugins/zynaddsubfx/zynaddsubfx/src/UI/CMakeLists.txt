set(UI_fl_files
    ADnoteUI.fl
    BankUI.fl
    ConfigUI.fl
    EffUI.fl
    EnvelopeUI.fl
    FilterUI.fl
    LFOUI.fl
    MasterUI.fl
    MicrotonalUI.fl
    OscilGenUI.fl
    PADnoteUI.fl
    PartUI.fl
    PresetsUI.fl
    ResonanceUI.fl
    SUBnoteUI.fl
    VirKeyboard.fl
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(UI/MasterUI.h PROPERTIES GENERATED 1)
fltk_wrap_ui(zynaddsubfx_gui ${UI_fl_files})

add_definitions(-DPIXMAP_PATH="${CMAKE_INSTALL_PREFIX}/share/zynaddsubfx/pixmaps/")
add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(LibloEnable)
    set(zynaddsubfx_gui_FLTK_UI_SRCS ${zynaddsubfx_gui_FLTK_UI_SRCS} NSM.C NSM/Client.C)
endif()

add_library(zynaddsubfx_gui STATIC
	${UI_objs}
	${zynaddsubfx_gui_FLTK_UI_SRCS}
        NioUI.cpp
        WidgetPDial.cpp
	)

if(NtkGui)
  target_link_libraries(zynaddsubfx_gui ${NTK_LIBRARIES})
endif(NtkGui)

if(FltkGui)
  target_link_libraries(zynaddsubfx_gui ${FLTK_LIBRARIES})
endif(FltkGui)
