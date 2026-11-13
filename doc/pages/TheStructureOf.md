\page TheStructureOfCore The structure of the LMMS core

There are some important C++ classes in the LMMS codebase. Here are some examples:

- \ref lmms::Engine "Engine" (also known as `LmmsCore`): Holds the instances of other important classes, e.g. Song, AudioEngine, PatternStore, Mixer.

- \ref lmms::Song "Song": Controls the playback. Also works as a top-level container of LMMS projects.

- \ref lmms::AudioEngine "AudioEngine": Renders audio independently of audio backends. Also manages audio backends and MIDI interfaces.

- \ref lmms::PlayHandle "PlayHandle": Manages objects that plays sound, ex. notes, instruments, sample clips.

- \ref lmms::AudioDevice "AudioDevice": Base class for audio backends.

- \ref lmms::Model "Model": Models in sense of Model-view-controller pattern

	- \ref lmms::AutomatableModel "AutomatableModel": Models which can be controlled by Controllers and/or AutomationPatterns, e.g. BoolModel, IntModel, FloatModel.

- \ref lmms::Track "Track": Base class for various types of tracks, including AutomationTrack, BBTrack, InstrumentTrack, and SampleTrack.

- \ref lmms::Plugin "Plugin": Base class for LMMS plugins(instruments and effects). For details on LMMS plugins, see the plugin system section.

	- \ref lmms::Instrument "Instrument": Generates audio signal from notes(from note clips or external MIDI inputs)

	- \ref lmms::Effect "Effect": Transforms audio signal
