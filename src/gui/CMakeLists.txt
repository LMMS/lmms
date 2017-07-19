SET(LMMS_SRCS
	${LMMS_SRCS}
	gui/AboutDialog.cpp
	gui/ActionGroup.cpp
	gui/AudioAlsaSetupWidget.cpp
	gui/AudioDeviceSetupWidget.cpp
	gui/AutomatableModelView.cpp
	gui/AutomationPatternView.cpp
	gui/ControllerConnectionDialog.cpp
	gui/ControllerDialog.cpp
	gui/EffectControlDialog.cpp
	gui/EffectSelectDialog.cpp
	gui/embed.cpp
	gui/ExportProjectDialog.cpp
	gui/FileBrowser.cpp
	gui/FxMixerView.cpp
	gui/GuiApplication.cpp
	gui/InstrumentView.cpp
	gui/LfoControllerDialog.cpp
	gui/LmmsPalette.cpp
	gui/LmmsStyle.cpp
	gui/MainApplication.cpp
	gui/MainWindow.cpp
	gui/MidiSetupWidget.cpp
	gui/ModelView.cpp
	gui/PeakControllerDialog.cpp
	gui/PianoView.cpp
	gui/PluginBrowser.cpp
	gui/RowTableView.cpp
	gui/SetupDialog.cpp
	gui/StringPairDrag.cpp
	gui/SubWindow.cpp
	gui/TimeLineWidget.cpp
	gui/ToolPluginView.cpp
	gui/TrackContainerView.cpp

	gui/dialogs/FileDialog.cpp
	gui/dialogs/VersionedSaveDialog.cpp

	gui/editors/AutomationEditor.cpp
	gui/editors/BBEditor.cpp
	gui/editors/Editor.cpp
	gui/editors/PianoRoll.cpp
	gui/editors/SongEditor.cpp

	gui/widgets/AutomatableButton.cpp
	gui/widgets/AutomatableSlider.cpp
	gui/widgets/CaptionMenu.cpp
	gui/widgets/ComboBox.cpp
	gui/widgets/ControllerRackView.cpp
	gui/widgets/ControllerView.cpp
	gui/widgets/CPULoadWidget.cpp
	gui/widgets/EffectRackView.cpp
	gui/widgets/EffectView.cpp
	gui/widgets/EnvelopeAndLfoView.cpp
	gui/widgets/FadeButton.cpp
	gui/widgets/Fader.cpp
	gui/widgets/FxLine.cpp
	gui/widgets/Graph.cpp
	gui/widgets/GroupBox.cpp
	gui/widgets/InstrumentFunctionViews.cpp
	gui/widgets/InstrumentMidiIOView.cpp
	gui/widgets/InstrumentSoundShapingView.cpp
	gui/widgets/LeftRightNav.cpp
	gui/widgets/Knob.cpp
	gui/widgets/LadspaControlView.cpp
	gui/widgets/LcdSpinBox.cpp
	gui/widgets/LcdWidget.cpp
	gui/widgets/LedCheckbox.cpp
	gui/widgets/MeterDialog.cpp
	gui/widgets/MidiPortMenu.cpp
	gui/widgets/NStateButton.cpp
	gui/widgets/PixmapButton.cpp
	gui/widgets/ProjectNotes.cpp
	gui/widgets/RenameDialog.cpp
	gui/widgets/Rubberband.cpp
	gui/widgets/SendButtonIndicator.cpp
	gui/widgets/SideBar.cpp
	gui/widgets/SideBarWidget.cpp
	gui/widgets/TabBar.cpp
	gui/widgets/TabWidget.cpp
	gui/widgets/TempoSyncKnob.cpp
	gui/widgets/TextFloat.cpp
	gui/widgets/TimeDisplayWidget.cpp
	gui/widgets/ToolButton.cpp
	gui/widgets/ToolTip.cpp
	gui/widgets/TrackLabelButton.cpp
	gui/widgets/VisualizationWidget.cpp

	PARENT_SCOPE
)

IF(QT5 AND LMMS_BUILD_LINUX)
    INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include/")
    add_library(x11embedcontainer STATIC
        "${CMAKE_SOURCE_DIR}/src/gui/X11EmbedContainer.cpp"
        "${CMAKE_SOURCE_DIR}/include/X11EmbedContainer.h"
    )
    FIND_PACKAGE(Qt5Core COMPONENTS Private)
    FIND_PACKAGE(Qt5Widgets COMPONENTS Private)
    FIND_PACKAGE(Qt5X11Extras REQUIRED)

	include_directories(${Qt5Core_PRIVATE_INCLUDE_DIRS})
	include_directories(${Qt5Widgets_PRIVATE_INCLUDE_DIRS})

	target_link_Libraries(x11embedcontainer
		 xcb xcb-util X11-xcb Qt5::X11Extras X11
	)

    SET(LMMS_REQUIRED_LIBS ${LMMS_REQUIRED_LIBS} x11embedcontainer PARENT_SCOPE)
ELSE()
    add_library(x11embedcontainer STATIC /dev/null)
ENDIF()

set(LMMS_UIS
	${LMMS_UIS}
	gui/dialogs/about_dialog.ui
	gui/dialogs/export_project.ui

	gui/Forms/EffectSelectDialog.ui

	PARENT_SCOPE
)
