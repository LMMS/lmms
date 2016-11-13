/*
 * Song.h - class song - the root of the model-tree
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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
#include "AutomatableModel.h"
#include "Controller.h"
#include "MeterModel.h"
#include "VstSyncController.h"


class AutomationTrack;
class Pattern;
class TimeLineWidget;


const bpm_t MinTempo = 10;
const bpm_t DefaultTempo = 140;
const bpm_t MaxTempo = 999;
const tick_t MaxSongLength = 9999 * DefaultTicksPerTact;


class EXPORT Song : public TrackContainer
{
	Q_OBJECT
	mapPropertyFromModel( int,getTempo,setTempo,m_tempoModel );
	mapPropertyFromModel( int,masterPitch,setMasterPitch,m_masterPitchModel );
	mapPropertyFromModel( int,masterVolume,setMasterVolume, m_masterVolumeModel );
public:
	enum PlayModes {
		Mode_None,
		Mode_PlaySong,
		Mode_PlayBB,
		Mode_PlayPattern,
		Mode_PlayAutomationPattern,
		Mode_Count
	} ;

	void clearErrors();
	void collectError( const QString error );
	bool hasErrors();
	QString* errorSummary();

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
		TimeLineWidget * m_timeLine;

	private:
		float m_currentFrame;

	} ;



	void processNextBuffer();

	inline int getMilliseconds() const
	{
		return m_elapsedMilliSeconds;
	}
	inline void setMilliSeconds( float ellapsedMilliSeconds )
	{
		m_elapsedMilliSeconds = ellapsedMilliSeconds;
	}
	inline int getTacts() const
	{
		return currentTact();
	}

	inline int ticksPerTact() const
	{
		return MidiTime::ticksPerTact( m_timeSigModel );
	}

	// Returns the beat position inside the bar, 0-based
	inline int getBeat() const
	{
		return getPlayPos().getBeatWithinBar( m_timeSigModel );
	}
	// the remainder after bar and beat are removed
	inline int getBeatTicks() const
	{
		return getPlayPos().getTickWithinBeat( m_timeSigModel );
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

	bool isExportDone() const;
	std::pair<MidiTime, MidiTime> getExportEndpoints() const;

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
		return getPlayPos( m_playMode );
	}

	void updateLength();
	tact_t length() const
	{
		return m_length;
	}


	bpm_t getTempo();
	virtual AutomationPattern * tempoAutomationPattern();

	AutomationTrack * globalAutomationTrack()
	{
		return m_globalAutomationTrack;
	}

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

	bool isModified() const
	{
		return m_modified;
	}

	virtual QString nodeName() const
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


public slots:
	void playSong();
	void record();
	void playAndRecord();
	void playBB();
	void playPattern( const Pattern * patternToPlay, bool loop = true );
	void togglePause();
	void stop();

	void importProject();
	void exportProject( bool multiExport = false );
	void exportProjectTracks();
	void exportProjectMidi();

	void startExport();
	void stopExport();


	void setModified();

	void clearProject();

	void addBBTrack();

	void setPlayPos( qint64 ticks, PlayModes playMode );


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


	inline tact_t currentTact() const
	{
		return m_playPos[m_playMode].getTact();
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

	void saveControllerStates( QDomDocument & doc, QDomElement & element );
	void restoreControllerStates( const QDomElement & element );

	void removeAllControllers();


	AutomationTrack * m_globalAutomationTrack;

	IntModel m_tempoModel;
	MeterModel m_timeSigModel;
	int m_oldTicksPerTact;
	IntModel m_masterVolumeModel;
	IntModel m_masterPitchModel;

	ControllerVector m_controllers;


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

	bool m_loadingProject;

	QList<QString> * m_errors;

	PlayModes m_playMode;
	PlayPos m_playPos[Mode_Count];
	tact_t m_length;

	const Pattern* m_patternToPlay;
	bool m_loopPattern;

	double m_elapsedMilliSeconds;
	tick_t m_elapsedTicks;
	tact_t m_elapsedTacts;

	VstSyncController m_vstSyncController;


	friend class LmmsCore;
	friend class SongEditor;
	friend class mainWindow;
	friend class ControllerRackView;

signals:
	void projectLoaded();
	void playbackStateChanged();
	void playbackPositionChanged();
	void lengthChanged( int tacts );
	void tempoChanged( bpm_t newBPM );
	void timeSignatureChanged( int oldTicksPerTact, int ticksPerTact );
	void controllerAdded( Controller * );
	void controllerRemoved( Controller * );

} ;


#endif
