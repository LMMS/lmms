/*
 * song.h - class song - the root of the model-tree
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

#ifndef _SONG_H
#define _SONG_H

#include <QtCore/QSharedMemory>
#include <QtCore/QVector>

#include "TrackContainer.h"
#include "AutomatableModel.h"
#include "Controller.h"
#include "MeterModel.h"
#include "VstSyncController.h"

class AutomationTrack;
class Pattern;
class timeLine;


const bpm_t MinTempo = 10;
const bpm_t DefaultTempo = 140;
const bpm_t MaxTempo = 999;
const tick_t MaxSongLength = 9999 * DefaultTicksPerTact;


class EXPORT song : public TrackContainer
{
	Q_OBJECT
	mapPropertyFromModel(int,getTempo,setTempo,m_tempoModel);
	mapPropertyFromModel(int,masterPitch,setMasterPitch,m_masterPitchModel);
	mapPropertyFromModel(int,masterVolume,setMasterVolume,
							m_masterVolumeModel);
public:
	enum PlayModes
	{
		Mode_None,
		Mode_PlaySong,
		Mode_PlayTrack,
		Mode_PlayBB,
		Mode_PlayPattern,
		Mode_PlayAutomationPattern,
		Mode_Count
	} ;


	class playPos : public MidiTime
	{
	public:
		playPos( const int _abs = 0 ) :
			MidiTime( _abs ),
			m_timeLine( NULL ),
			m_timeLineUpdate( true ),
			m_currentFrame( 0.0f )
		{
		}
		inline void setCurrentFrame( const float _f )
		{
			m_currentFrame = _f;
		}
		inline float currentFrame() const
		{
			return m_currentFrame;
		}
		timeLine * m_timeLine;
		bool m_timeLineUpdate;

	private:
		float m_currentFrame;

	} ;



	void processNextBuffer();

	inline int getMilliseconds() const
	{
		return m_elapsedMilliSeconds;
	}
	inline void setMilliSeconds( float _ellapsedMilliSeconds )
	{
		m_elapsedMilliSeconds = (_ellapsedMilliSeconds);
	}
	inline int getTacts() const
	{
		return currentTact();
	}

	inline int ticksPerTact() const
	{
		return DefaultTicksPerTact *
				m_timeSigModel.getNumerator() /
					 m_timeSigModel.getDenominator();
	}

	// Returns the beat position inside the bar, 0-based
	inline int getBeat() const
	{
		return (currentTick() - currentTact()*ticksPerTact()) /
			(ticksPerTact() / m_timeSigModel.getNumerator() );
	}
	// the remainder after bar and beat are removed
	inline int getBeatTicks() const
	{
		return 	(currentTick() - currentTact()*ticksPerTact()) %
			(ticksPerTact() / m_timeSigModel.getNumerator() );
	}
	inline int getTicks() const
	{
		return currentTick();
	}
	inline bool isTempoAutomated()
	{
		return m_tempoModel.isAutomated();
	}
	inline bool isPaused() const
	{
		return m_paused;
	}

	inline bool isPlaying() const
	{
		return m_playing && m_exporting == false;
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

	inline bool isExportDone() const
	{
		if ( m_exportLoop )
		{
			return m_exporting == true &&
				m_playPos[Mode_PlaySong].getTicks() >= length() * ticksPerTact();
		}
		else
		{
			return m_exporting == true &&
				m_playPos[Mode_PlaySong].getTicks() >= ( length() + 1 ) * ticksPerTact();
		}
	}

	inline PlayModes playMode() const
	{
		return m_playMode;
	}

	inline playPos & getPlayPos( PlayModes _pm )
	{
		return m_playPos[_pm];
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
	void createNewProjectFromTemplate( const QString & _template );
	void loadProject( const QString & _filename );
	bool guiSaveProject();
	bool guiSaveProjectAs( const QString & _filename );
	bool saveProjectFile( const QString & _filename );

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

	void addController( Controller * _c );
	void removeController( Controller * _c );
	

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
	void playTrack( track * _trackToPlay );
	void playBB();
	void playPattern( Pattern* patternToPlay, bool _loop = true );
	void togglePause();
	void stop();

	void importProject();
	void exportProject(bool multiExport=false);
	void exportProjectTracks();

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
	song();
	song( const song & );
	virtual ~song();


	inline tact_t currentTact() const
	{
		return m_playPos[m_playMode].getTact();
	}

	inline tick_t currentTick() const
	{
		return m_playPos[m_playMode].getTicks();
	}
	void setPlayPos( tick_t _ticks, PlayModes _play_mode );

	void saveControllerStates( QDomDocument & _doc, QDomElement & _this );
	void restoreControllerStates( const QDomElement & _this );


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

	volatile bool m_recording;
	volatile bool m_exporting;
	volatile bool m_exportLoop;
	volatile bool m_playing;
	volatile bool m_paused;

	bool m_loadingProject;

	PlayModes m_playMode;
	playPos m_playPos[Mode_Count];
	tact_t m_length;

	track * m_trackToPlay;
	Pattern* m_patternToPlay;
	bool m_loopPattern;

	double m_elapsedMilliSeconds;
	tick_t m_elapsedTicks;
	tact_t m_elapsedTacts;

	VstSyncController m_vstSyncController;


	friend class engine;
	friend class SongEditor;
	friend class mainWindow;
	friend class ControllerRackView;

signals:
	void projectLoaded();
	void playbackStateChanged();
	void lengthChanged( int _tacts );
	void tempoChanged( bpm_t _new_bpm );
	void timeSignatureChanged( int _old_ticks_per_tact,
							int _ticks_per_tact );

} ;


#endif
