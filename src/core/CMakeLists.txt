set(LMMS_CORE_SRCS
	AudioEngine.cpp
	AudioEngineProfiler.cpp
	AudioEngineWorkerThread.cpp
	AudioResampler.cpp
	AutomatableModel.cpp
	AutomationClip.cpp
	AutomationNode.cpp
	BandLimitedWave.cpp
	base64.cpp
	BufferManager.cpp
	Clipboard.cpp
	ComboBoxModel.cpp
	ConfigManager.cpp
	Controller.cpp
	ControllerConnection.cpp
	DataFile.cpp
	DrumSynth.cpp
	Effect.cpp
	EffectChain.cpp
	Engine.cpp
	EnvelopeAndLfoParameters.cpp
	fft_helpers.cpp
	FileSearch.cpp
	Mixer.cpp
	ImportFilter.cpp
	InlineAutomation.cpp
	Instrument.cpp
	InstrumentFunctions.cpp
	InstrumentPlayHandle.cpp
	InstrumentSoundShaping.cpp
	JournallingObject.cpp
	Keymap.cpp
	Ladspa2LMMS.cpp
	LadspaControl.cpp
	LadspaManager.cpp
	LfoController.cpp
	LinkedModelGroups.cpp
	LocklessAllocator.cpp
	MemoryHelper.cpp
	MeterModel.cpp
	MicroTimer.cpp
	Microtuner.cpp
	MixHelpers.cpp
	Model.cpp
	ModelVisitor.cpp
	Note.cpp
	NotePlayHandle.cpp
	Oscillator.cpp
	PathUtil.cpp
	PatternClip.cpp
	PatternStore.cpp
	PeakController.cpp
	PerfLog.cpp
	Piano.cpp
	PlayHandle.cpp
	Plugin.cpp
	PluginIssue.cpp
	PluginFactory.cpp
	PresetPreviewPlayHandle.cpp
	ProjectJournal.cpp
	ProjectRenderer.cpp
	ProjectVersion.cpp
	RemotePlugin.cpp
	RenderManager.cpp
	RingBuffer.cpp
	Sample.cpp
	SampleBuffer.cpp
	SampleClip.cpp
	SampleDecoder.cpp
	SamplePlayHandle.cpp
	SampleRecordHandle.cpp
	Scale.cpp
	LmmsSemaphore.cpp
	SerializingObject.cpp
	Song.cpp
	TempoSyncKnobModel.cpp
	ThreadPool.cpp
	Timeline.cpp
	TimePos.cpp
	ToolPlugin.cpp
	Track.cpp
	TrackContainer.cpp
	UpgradeExtendedNoteRange.h
	UpgradeExtendedNoteRange.cpp
	Clip.cpp
	ValueBuffer.cpp
	VstSyncController.cpp
	StepRecorder.cpp

	audio/AudioAlsa.cpp
	audio/AudioDevice.cpp
	audio/AudioFileDevice.cpp
	audio/AudioFileMP3.cpp
	audio/AudioFileOgg.cpp
	audio/AudioFileFlac.cpp
	audio/AudioFileWave.cpp
	audio/AudioJack.cpp
	audio/AudioOss.cpp
	audio/AudioSndio.cpp
	audio/AudioPort.cpp
	audio/AudioPortAudio.cpp
	audio/AudioSoundIo.cpp
	audio/AudioPulseAudio.cpp
	audio/AudioSampleRecorder.cpp
	audio/AudioSdl.cpp

	lv2/Lv2Basics.cpp
	lv2/Lv2ControlBase.cpp
	lv2/Lv2Evbuf.cpp
	lv2/Lv2Features.cpp
	lv2/Lv2Ports.cpp
	lv2/Lv2Proc.cpp
	lv2/Lv2Manager.cpp
	lv2/Lv2Options.cpp
	lv2/Lv2SubPluginFeatures.cpp
	lv2/Lv2UridCache.cpp
	lv2/Lv2UridMap.cpp
	lv2/Lv2Worker.cpp

	midi/MidiAlsaRaw.cpp
	midi/MidiAlsaSeq.cpp
	midi/MidiClient.cpp
	midi/MidiController.cpp
	midi/MidiEventToByteSeq.cpp
	midi/MidiJack.cpp
	midi/MidiOss.cpp
	midi/MidiSndio.cpp
	midi/MidiApple.cpp
	midi/MidiPort.cpp
	midi/MidiWinMM.cpp

	tracks/AutomationTrack.cpp
	tracks/InstrumentTrack.cpp
	tracks/MidiClip.cpp
	tracks/PatternTrack.cpp
	tracks/SampleTrack.cpp
)

SET(LMMS_HAVE_ALSA TRUE)

FILE(GLOB CORE_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*")

QT_WRAP_CPP(LMMS_CORE_SRCS "${CORE_INCLUDE_FILES}" TARGET lmms_core_objs)

ADD_LIBRARY(lmms_core_objs OBJECT ${LMMS_CORE_SRCS})
TARGET_INCLUDE_DIRECTORIES(lmms_core_objs PUBLIC "${CMAKE_SOURCE_DIR}/src/gui/include" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/src" "${PROJECT_SOURCE_DIR}" "${WEAKJACK_INCLUDE_DIRS}")

target_link_libraries(lmms_core_objs PUBLIC lmms_common_objs)

GENERATE_EXPORT_HEADER(lmms_core_objs
	BASE_NAME lmms
)