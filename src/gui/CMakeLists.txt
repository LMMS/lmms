SET(LMMS_GUI_SRCS
	ActionGroup.cpp
	AudioAlsaSetupWidget.cpp
	AudioDeviceSetupWidget.cpp
	AutomatableModelView.cpp
	ControlLayout.cpp
	ControllerDialog.cpp
	ControllerRackView.cpp
	ControllerView.cpp
	Controls.cpp
	EffectControlDialog.cpp
	EffectRackView.cpp
	EffectView.cpp
	embed.cpp
	FileBrowser.cpp
	GuiApplication.cpp
	LadspaControlView.cpp
	LfoControllerDialog.cpp
	LinkedModelGroupViews.cpp
	LmmsPalette.cpp
	LmmsStyle.cpp
	Lv2ViewBase.cpp
	MainApplication.cpp
	MainWindow.cpp
	MicrotunerConfig.cpp
	MidiCCRackView.cpp
	MidiSetupWidget.cpp
	MixerLine.cpp
	MixerView.cpp
	ModelView.cpp
	PeakControllerDialog.cpp
	PluginBrowser.cpp
	ProjectNotes.cpp
	RowTableView.cpp
	SampleTrackWindow.cpp
	SendButtonIndicator.cpp
	SideBar.cpp
	SideBarWidget.cpp
	StringPairDrag.cpp
	SubWindow.cpp
	ToolPluginView.cpp

	clips/AutomationClipView.cpp
	clips/ClipView.cpp
	clips/MidiClipView.cpp
	clips/PatternClipView.cpp
	clips/SampleClipView.cpp

	editors/AutomationEditor.cpp
	editors/Editor.cpp
	editors/PatternEditor.cpp
	editors/PianoRoll.cpp
	editors/PositionLine.cpp
	editors/Rubberband.cpp
	editors/SongEditor.cpp
	editors/StepRecorderWidget.cpp
	editors/TimeLineWidget.cpp
	editors/TrackContainerView.cpp

	instrument/EnvelopeAndLfoView.cpp
	instrument/InstrumentFunctionViews.cpp
	instrument/InstrumentMidiIOView.cpp
	instrument/InstrumentMiscView.cpp
	instrument/InstrumentSoundShapingView.cpp
	instrument/InstrumentTrackWindow.cpp
	instrument/InstrumentView.cpp
	instrument/PianoView.cpp

	menus/MidiPortMenu.cpp
	menus/RecentProjectsMenu.cpp
	menus/TemplatesMenu.cpp

	modals/AboutDialog.cpp
	modals/ColorChooser.cpp
	modals/ControllerConnectionDialog.cpp
	modals/EffectSelectDialog.cpp
	modals/ExportProjectDialog.cpp
	modals/FileDialog.cpp
	modals/RenameDialog.cpp
	modals/SetupDialog.cpp
	modals/VersionedSaveDialog.cpp

	tracks/AutomationTrackView.cpp
	tracks/FadeButton.cpp
	tracks/InstrumentTrackView.cpp
	tracks/PatternTrackView.cpp
	tracks/SampleTrackView.cpp
	tracks/TrackContentWidget.cpp
	tracks/TrackLabelButton.cpp
	tracks/TrackOperationsWidget.cpp
	tracks/TrackRenameLineEdit.cpp
	tracks/TrackView.cpp

	widgets/AutomatableButton.cpp
	widgets/AutomatableSlider.cpp
	widgets/CPULoadWidget.cpp
	widgets/CaptionMenu.cpp
	widgets/ComboBox.cpp
	widgets/CustomTextKnob.cpp
	widgets/Fader.cpp
	widgets/Graph.cpp
	widgets/GroupBox.cpp
	widgets/Knob.cpp
	widgets/LcdFloatSpinBox.cpp
	widgets/LcdSpinBox.cpp
	widgets/LcdWidget.cpp
	widgets/LedCheckBox.cpp
	widgets/LeftRightNav.cpp
	widgets/MeterDialog.cpp
	widgets/MixerLineLcdSpinBox.cpp
	widgets/NStateButton.cpp
	widgets/Oscilloscope.cpp
	widgets/PixmapButton.cpp
	widgets/SimpleTextFloat.cpp
	widgets/TabBar.cpp
	widgets/TabWidget.cpp
	widgets/TempoSyncKnob.cpp
	widgets/TextFloat.cpp
	widgets/TimeDisplayWidget.cpp
	widgets/ToolButton.cpp
)

SET(LMMS_UIS
	modals/about_dialog.ui
	modals/EffectSelectDialog.ui
	modals/export_project.ui
)

QT5_WRAP_UI(LMMS_UI_OUT ${LMMS_UIS})
ADD_LIBRARY(lmms_gui STATIC ${LMMS_GUI_SRCS} ${LMMS_UI_OUT})
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${LMMS_UI_OUT}")

target_include_directories(lmms_gui PUBLIC "${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/src" "${WEAKJACK_INCLUDE_DIRS}")

GENERATE_EXPORT_HEADER(lmms_gui
	BASE_NAME lmms
)