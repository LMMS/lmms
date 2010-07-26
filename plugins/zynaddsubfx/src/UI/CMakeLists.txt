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
    SeqUI.fl
    SUBnoteUI.fl
    VirKeyboard.fl
    WidgetPDial.fl
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(UI/MasterUI.h PROPERTIES GENERATED 1)
fltk_wrap_ui(zynaddsubfx_gui ${UI_fl_files})

add_library(zynaddsubfx_gui STATIC
	${UI_objs}
	${zynaddsubfx_gui_FLTK_UI_SRCS}
	)

target_link_libraries(zynaddsubfx_gui ${FLTK_LIBRARIES} ${MYFLTK_LIBRARIES})
