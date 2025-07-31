/*
 * Song.h - class song - the root of the model-tree
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_SONG_H
#define LMMS_SONG_H

#include <array>
#include <memory>

#include <QString>
#include <QHash>  // IWYU pragma: keep

#include "AudioEngine.h"
#include "Controller.h"
#include "Metronome.h"
#include "lmms_constants.h"
#include "MeterModel.h"
#include "Timeline.h"
#include "TrackContainer.h"
#include "VstSyncController.h"

namespace lmms
{

class AutomationTrack;
class Keymap;
class MidiClip;
class Scale;

namespace gui
{
class SongEditor;
class ControllerRackView;
}


const bpm_t MinTempo = 10;
const bpm_t DefaultTempo = 140;
const bpm_t MaxTempo = 999;
const tick_t MaxSongLength = 9999 * DefaultTicksPerBar;


class LMMS_EXPORT Song : public TrackContainer
{
	Q_OBJECT
	mapPropertyFromModel( int,getTempo,setTempo,m_tempoModel );
	mapPropertyFromModel( int,masterPitch,setMasterPitch,m_masterPitchModel );
	mapPropertyFromModel( int,masterVolume,setMasterVolume, m_masterVolumeModel );
public:
	enum class PlayMode
	{
		None,
		Song,
		Pattern,
		MidiClip,
		AutomationClip,
		Count
	} ;
	constexpr static auto PlayModeCount = static_cast<std::size_t>(PlayMode::Count);

	struct SaveOptions {
		/**
		 * Should we discard MIDI ControllerConnections from project files?
		 */
		BoolModel discardMIDIConnections{false};
		/**
		 * Should we save the project as a project bundle? (with resources)
		 */
		BoolModel saveAsProjectBundle{false};

		void setDefaultOptions() {
			discardMIDIConnections.setValue(false);
			saveAsProjectBundle.setValue(false);
		}
	};

	void clearErrors();
	void collectError( const QString error );
	bool hasErrors();
	QString errorSummary();

	class PlayPos : public TimePos
	{
	public:
		PlayPos( const int abs = 0 ) :
			TimePos( abs ),
			m_currentFrame( 0.0f )
		{
		}
		inline void setCurrentFrame( const float f )
		{
			m_currentFrame = f;
		}
		inline float currentFrame() const
		{
			return m_currentFrame;
		}
		inline void setJumped( const bool jumped )
		{
			m_jumped = jumped;
		}
		inline bool jumped() const
		{
			return m_jumped;
		}

	private:
		float m_currentFrame;
		bool m_jumped;
	};

	void processNextBuffer();

	inline int getLoadingTrackCount() const
	{
		return m_nLoadingTrack;
	}

	inline int getMilliseconds() const
	{
		return getMilliseconds(m_playMode);
	}

	inline int getMilliseconds(PlayMode playMode) const
	{
		return m_elapsedMilliSeconds[static_cast<std::size_t>(playMode)];
	}

	inline void setToTime(TimePos const & pos)
	{
		setToTime(pos, m_playMode);
	}

	inline void setToTime(TimePos const & pos, PlayMode playMode)
	{
		m_elapsedMilliSeconds[static_cast<std::size_t>(playMode)] = pos.getTimeInMilliseconds(getTempo());
		getPlayPos(playMode).setTicks(pos.getTicks());
	}

	inline void setToTimeByTicks(tick_t ticks)
	{
		setToTimeByTicks(ticks, m_playMode);
	}

	inline void setToTimeByTicks(tick_t ticks, PlayMode playMode)
	{
		m_elapsedMilliSeconds[static_cast<std::size_t>(playMode)] = TimePos::ticksToMilliseconds(ticks, getTempo());
		getPlayPos(playMode).setTicks(ticks);
	}

	inline int getBars() const
	{
		return currentBar();
	}

	inline int ticksPerBar() const
	{
		return TimePos::ticksPerBar(m_timeSigModel);
	}

	// Returns the beat position inside the bar, 0-based
	inline int getBeat() const
	{
		return getPlayPos().getBeatWithinBar(m_timeSigModel);
	}
	// the remainder after bar and beat are removed
	inline int getBeatTicks() const
	{
		return getPlayPos().getTickWithinBeat(m_timeSigModel);
	}
	inline int getTicks() const
	{
		return currentTick();
	}
	inline f_cnt_t getFrames() const
	{
		return currentFrame();
	}
	inline bool isPaused() const
	{
		return m_paused;
	}

	inline bool isPlaying() const
	{
		return m_playing == true && m_exporting == false;
	}

	inline bool isStopped() const
	{
		return m_playing == false && m_paused == false;
	}

	inline bool isExporting() const
	{
		return m_exporting;
	}

	inline void setExportLoop( bool exportLoop )
	{
		m_exportLoop = exportLoop;
	}

	inline bool isRecording() const
	{
		return m_recording;
	}
	
	inline void setLoopRenderCount(int count)
	{
		if (count < 1)
			m_loopRenderCount = 1;
		else
			m_loopRenderCount = count;
		m_loopRenderRemaining = m_loopRenderCount;
	}
    
	inline int getLoopRenderCount() const
	{
		return m_loopRenderCount;
	}

	bool isExportDone() const;
	int getExportProgress() const;

	inline void setRenderBetweenMarkers( bool renderBetweenMarkers )
	{
		m_renderBetweenMarkers = renderBetweenMarkers;
	}

	inline PlayMode playMode() const
	{
		return m_playMode;
	}

	PlayMode lastPlayMode() const { return m_lastPlayMode; }
	inline PlayPos & getPlayPos( PlayMode pm )
	{
		return m_playPos[static_cast<std::size_t>(pm)];
	}
	inline const PlayPos & getPlayPos( PlayMode pm ) const
	{
		return m_playPos[static_cast<std::size_t>(pm)];
	}
	inline PlayPos & getPlayPos()
	{
		return getPlayPos(m_playMode);
	}
	inline const PlayPos & getPlayPos() const
	{
		return getPlayPos(m_playMode);
	}

	auto getTimeline(PlayMode mode) -> Timeline& { return m_timelines[static_cast<std::size_t>(mode)]; }
	auto getTimeline(PlayMode mode) const -> const Timeline& { return m_timelines[static_cast<std::size_t>(mode)]; }
	auto getTimeline() -> Timeline& { return getTimeline(m_playMode); }
	auto getTimeline() const -> const Timeline& { return getTimeline(m_playMode); }

	void updateLength();
	bar_t length() const
	{
		return m_length;
	}


	bpm_t getTempo();

	AutomationTrack * globalAutomationTrack()
	{
		return m_globalAutomationTrack;
	}

	//TODO: Add Q_DECL_OVERRIDE when Qt4 is dropped
	AutomatedValueMap automatedValuesAt(TimePos time, int clipNum = -1) const override;

	// file management
	void createNewProject();
	void createNewProjectFromTemplate( const QString & templ );
	void loadProject( const QString & filename );
	bool guiSaveProject();
	bool guiSaveProjectAs(const QString & filename);
	bool saveProjectFile(const QString & filename, bool withResources = false);

	const QString & projectFileName() const
	{
		return m_fileName;
	}

	bool isLoadingProject() const
	{
		return m_loadingProject;
	}

	void loadingCancelled()
	{
		m_isCancelled = true;
		Engine::audioEngine()->clearNewPlayHandles();
	}

	bool isCancelled()
	{
		return m_isCancelled;
	}

	bool isModified() const
	{
		return m_modified;
	}

	QString nodeName() const override
	{
		return "song";
	}

	virtual bool fixedClips() const
	{
		return false;
	}

	void addController( Controller * c );
	void removeController( Controller * c );


	const ControllerVector & controllers() const
	{
		return m_controllers;
	}


	MeterModel & getTimeSigModel()
	{
		return m_timeSigModel;
	}

	IntModel& tempoModel()
	{
		return m_tempoModel;
	}

	void exportProjectMidi(QString const & exportFileName) const;

	inline void setLoadOnLaunch(bool value) { m_loadOnLaunch = value; }
	SaveOptions &getSaveOptions() {
		return m_saveOptions;
	}

	bool isSavingProject() const;

	std::shared_ptr<const Scale> getScale(unsigned int index) const;
	std::shared_ptr<const Keymap> getKeymap(unsigned int index) const;
	void setScale(unsigned int index, std::shared_ptr<Scale> newScale);
	void setKeymap(unsigned int index, std::shared_ptr<Keymap> newMap);

	const std::string& syncKey() const noexcept { return m_vstSyncController.sharedMemoryKey(); }

	Metronome& metronome() { return m_metronome; }

public slots:
	void playSong();
	void record();
	void playAndRecord();
	void playPattern();
	void playMidiClip( const lmms::MidiClip * midiClipToPlay, bool loop = true );
	void togglePause();
	void stop();

	void startExport();
	void stopExport();


	void setModified();

	void clearProject();

	void addPatternTrack();


private slots:
	void insertBar();
	void removeBar();
	void addSampleTrack();
	void addAutomationTrack();

	void setTempo();
	void setTimeSignature();

	void masterVolumeChanged();

	void savePlayStartPosition();

	void updateFramesPerTick();



private:
	Song();
	Song( const Song & );
	~Song() override;


	inline bar_t currentBar() const
	{
		return getPlayPos(m_playMode).getBar();
	}

	inline tick_t currentTick() const
	{
		return getPlayPos(m_playMode).getTicks();
	}

	inline f_cnt_t currentFrame() const
	{
		return getPlayPos(m_playMode).getTicks() * Engine::framesPerTick() +
			getPlayPos(m_playMode).currentFrame();
	}

	void setPlayPos( tick_t ticks, PlayMode playMode );

	void saveControllerStates( QDomDocument & doc, QDomElement & element );
	void restoreControllerStates( const QDomElement & element );

	void removeAllControllers();

	void saveScaleStates(QDomDocument &doc, QDomElement &element);
	void restoreScaleStates(const QDomElement &element);

	void saveKeymapStates(QDomDocument &doc, QDomElement &element);
	void restoreKeymapStates(const QDomElement &element);

	void processAutomations(const TrackList& tracks, TimePos timeStart, fpp_t frames);
	void processMetronome(size_t bufferOffset);

	void setModified(bool value);

	void setProjectFileName(QString const & projectFileName);

	AutomationTrack * m_globalAutomationTrack;

	IntModel m_tempoModel;
	MeterModel m_timeSigModel;
	int m_oldTicksPerBar;
	IntModel m_masterVolumeModel;
	IntModel m_masterPitchModel;

	ControllerVector m_controllers;

	int m_nLoadingTrack;

	QString m_fileName;
	QString m_oldFileName;
	bool m_modified;
	bool m_loadOnLaunch;

	volatile bool m_recording;
	volatile bool m_exporting;
	volatile bool m_exportLoop;
	volatile bool m_renderBetweenMarkers;
	volatile bool m_playing;
	volatile bool m_paused;

	bool m_savingProject;
	bool m_loadingProject;
	bool m_isCancelled;

	SaveOptions m_saveOptions;

	QHash<QString, int> m_errors;

	std::array<Timeline, PlayModeCount> m_timelines;

	PlayMode m_playMode;
	PlayMode m_lastPlayMode;
	PlayPos m_playPos[PlayModeCount];
	bar_t m_length;

	const MidiClip* m_midiClipToPlay;
	bool m_loopMidiClip;

	double m_elapsedMilliSeconds[PlayModeCount];
	tick_t m_elapsedTicks;
	bar_t m_elapsedBars;

	VstSyncController m_vstSyncController;
    
	int m_loopRenderCount;
	int m_loopRenderRemaining;
	TimePos m_exportSongBegin;
	TimePos m_exportLoopBegin;
	TimePos m_exportLoopEnd;
	TimePos m_exportSongEnd;
	TimePos m_exportEffectiveLength;

	std::shared_ptr<Scale> m_scales[MaxScaleCount];
	std::shared_ptr<Keymap> m_keymaps[MaxKeymapCount];

	AutomatedValueMap m_oldAutomatedValues;

	Metronome m_metronome;

	friend class Engine;
	friend class gui::SongEditor;
	friend class gui::ControllerRackView;

signals:
	void projectLoaded();
	void playbackStateChanged();
	void playbackPositionChanged();
	void lengthChanged( int bars );
	void tempoChanged( lmms::bpm_t newBPM );
	void timeSignatureChanged( int oldTicksPerBar, int ticksPerBar );
	void controllerAdded( lmms::Controller * );
	void controllerRemoved( lmms::Controller * );
	void updateSampleTracks();
	void stopped();
	void modified();
	void projectFileNameChanged();
	void scaleListChanged(int index);
	void keymapListChanged(int index);
} ;


} // namespace lmms

#endif // LMMS_SONG_H
