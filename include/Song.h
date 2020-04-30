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

#ifndef SONG_H
#define SONG_H

#include <utility>

#include <QtCore/QSharedMemory>
#include <QtCore/QVector>

#include "TrackContainer.h"
#include "Controller.h"
#include "MeterModel.h"
#include "Mixer.h"
#include "VstSyncController.h"


class AutomationTrack;
class Pattern;
class TimeLineWidget;


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
	enum PlayModes
	{
		Mode_None,
		Mode_PlaySong,
		Mode_PlayBB,
		Mode_PlayPattern,
		Mode_PlayAutomationPattern,
		Mode_Count
	} ;

	struct SaveOptions {
		/**
		 * Should we discard MIDI ControllerConnections from project files?
		 */
		BoolModel discardMIDIConnections{false};

		void setDefaultOptions() {
			discardMIDIConnections.setValue(false);
		}
	};

	void clearErrors();
	void collectError( const QString error );
	bool hasErrors();
	QString errorSummary();

	class PlayPos : public MidiTime
	{
	public:
		PlayPos( const int abs = 0 ) :
			MidiTime( abs ),
			m_timeLine( NULL ),
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
		TimeLineWidget * m_timeLine;

	private:
		float m_currentFrame;
		bool m_jumped;

	} ;

	void processNextBuffer();

	inline int getLoadingTrackCount() const
	{
		return m_nLoadingTrack;
	}

	inline int getMilliseconds() const
	{
		return m_elapsedMilliSeconds[m_playMode];
	}

	inline int getMilliseconds(PlayModes playMode) const
	{
		return m_elapsedMilliSeconds[playMode];
	}

	inline void setToTime(MidiTime const & midiTime)
	{
		m_elapsedMilliSeconds[m_playMode] = midiTime.getTimeInMilliseconds(getTempo());
		m_playPos[m_playMode].setTicks(midiTime.getTicks());
	}

	inline void setToTime(MidiTime const & midiTime, PlayModes playMode)
	{
		m_elapsedMilliSeconds[playMode] = midiTime.getTimeInMilliseconds(getTempo());
		m_playPos[playMode].setTicks(midiTime.getTicks());
	}

	inline void setToTimeByTicks(tick_t ticks)
	{
		m_elapsedMilliSeconds[m_playMode] = MidiTime::ticksToMilliseconds(ticks, getTempo());
		m_playPos[m_playMode].setTicks(ticks);
	}

	inline void setToTimeByTicks(tick_t ticks, PlayModes playMode)
	{
		m_elapsedMilliSeconds[playMode] = MidiTime::ticksToMilliseconds(ticks, getTempo());
		m_playPos[playMode].setTicks(ticks);
	}

	inline int getBars() const
	{
		return currentBar();
	}

	inline int ticksPerBar() const
	{
		return MidiTime::ticksPerBar(m_timeSigModel);
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

	inline PlayModes playMode() const
	{
		return m_playMode;
	}

	inline PlayPos & getPlayPos( PlayModes pm )
	{
		return m_playPos[pm];
	}
	inline const PlayPos & getPlayPos( PlayModes pm ) const
	{
		return m_playPos[pm];
	}
	inline const PlayPos & getPlayPos() const
	{
		return getPlayPos(m_playMode);
	}

	void updateLength();
	bar_t length() const
	{
		return m_length;
	}


	bpm_t getTempo();
	AutomationPattern * tempoAutomationPattern() override;

	AutomationTrack * globalAutomationTrack()
	{
		return m_globalAutomationTrack;
	}

	//TODO: Add Q_DECL_OVERRIDE when Qt4 is dropped
	AutomatedValueMap automatedValuesAt(MidiTime time, int tcoNum = -1) const override;

	// file management
	void createNewProject();
	void createNewProjectFromTemplate( const QString & templ );
	void loadProject( const QString & filename );
	bool guiSaveProject();
	bool guiSaveProjectAs( const QString & filename );
	bool saveProjectFile( const QString & filename );

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
		Engine::mixer()->clearNewPlayHandles();
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

	virtual bool fixedTCOs() const
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

	void exportProjectMidi(QString const & exportFileName) const;

	inline void setLoadOnLauch(bool value) { m_loadOnLaunch = value; }
	SaveOptions &getSaveOptions() {
		return m_saveOptions;
	}

	bool isSavingProject() const;

public slots:
	void playSong();
	void record();
	void playAndRecord();
	void playBB();
	void playPattern( const Pattern * patternToPlay, bool loop = true );
	void togglePause();
	void stop();

	void startExport();
	void stopExport();


	void setModified();

	void clearProject();

	void addBBTrack();


private slots:
	void insertBar();
	void removeBar();
	void addSampleTrack();
	void addAutomationTrack();

	void setTempo();
	void setTimeSignature();

	void masterVolumeChanged();

	void savePos();

	void updateFramesPerTick();



private:
	Song();
	Song( const Song & );
	virtual ~Song();


	inline bar_t currentBar() const
	{
		return m_playPos[m_playMode].getBar();
	}

	inline tick_t currentTick() const
	{
		return m_playPos[m_playMode].getTicks();
	}

	inline f_cnt_t currentFrame() const
	{
		return m_playPos[m_playMode].getTicks() * Engine::framesPerTick() +
			m_playPos[m_playMode].currentFrame();
	}

	void setPlayPos( tick_t ticks, PlayModes playMode );

	void saveControllerStates( QDomDocument & doc, QDomElement & element );
	void restoreControllerStates( const QDomElement & element );

	void removeAllControllers();

	void processAutomations(const TrackList& tracks, MidiTime timeStart, fpp_t frames);

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

	QStringList m_errors;

	PlayModes m_playMode;
	PlayPos m_playPos[Mode_Count];
	bar_t m_length;

	const Pattern* m_patternToPlay;
	bool m_loopPattern;

	double m_elapsedMilliSeconds[Mode_Count];
	tick_t m_elapsedTicks;
	bar_t m_elapsedBars;

	VstSyncController m_vstSyncController;
    
	int m_loopRenderCount;
	int m_loopRenderRemaining;
	MidiTime m_exportSongBegin;
	MidiTime m_exportLoopBegin;
	MidiTime m_exportLoopEnd;
	MidiTime m_exportSongEnd;
	MidiTime m_exportEffectiveLength;

	friend class LmmsCore;
	friend class SongEditor;
	friend class mainWindow;
	friend class ControllerRackView;

signals:
	void projectLoaded();
	void playbackStateChanged();
	void playbackPositionChanged();
	void lengthChanged( int bars );
	void tempoChanged( bpm_t newBPM );
	void timeSignatureChanged( int oldTicksPerBar, int ticksPerBar );
	void controllerAdded( Controller * );
	void controllerRemoved( Controller * );
	void updateSampleTracks();
	void stopped();
	void modified();
	void projectFileNameChanged();
} ;


#endif
