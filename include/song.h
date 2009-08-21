/*
 * song.h - class song - the root of the model-tree
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#include <QtCore/QVector>

#include "track_container.h"
#include "automatable_model.h"
#include "Controller.h"
#include "meter_model.h"


class automationTrack;
class pattern;
class timeLine;


const bpm_t MinTempo = 10;
const bpm_t DefaultTempo = 140;
const bpm_t MaxTempo = 999;
const tick_t MaxSongLength = 9999 * DefaultTicksPerTact;


class EXPORT song : public trackContainer
{
	Q_OBJECT
	mapPropertyFromModel(int,getTempo,setTempo,m_tempoModel);
	mapPropertyFromModel(int,masterPitch,setMasterPitch,m_masterPitchModel);
	mapPropertyFromModel(int,masterVolume,setMasterVolume,
							m_masterVolumeModel);
public:
	enum PlayModes
	{
		Mode_PlaySong,
		Mode_PlayTrack,
		Mode_PlayBB,
		Mode_PlayPattern,
		Mode_PlayAutomationPattern,
		Mode_Count
	} ;


	class playPos : public midiTime
	{
	public:
		playPos( const int _abs = 0 ) :
			midiTime( _abs ),
			m_timeLine( NULL ),
			m_timeLineUpdate( true ),
			m_currentFrame( 0.0f )
		{
		}
		inline void setCurrentFrame( const float _f )
		{
			m_currentFrame = _f;
		}
		inline float currentFrame( void ) const
		{
			return m_currentFrame;
		}
		timeLine * m_timeLine;
		bool m_timeLineUpdate;

	private:
		float m_currentFrame;

	} ;



	void processNextBuffer( void );


	inline bool isPaused( void ) const
	{
		return m_paused;
	}

	inline bool isPlaying( void ) const
	{
		return m_playing && m_exporting == false;
	}

	inline bool isExporting( void ) const
	{
		return m_exporting;
	}

	inline bool isRecording( void ) const
	{
		return m_recording;
	}

	bool realTimeTask( void ) const;

	inline bool isExportDone( void ) const
	{
		return m_exporting == true &&
			m_playPos[Mode_PlaySong].getTact() >= length() + 1;
	}

	inline PlayModes playMode( void ) const
	{
		return m_playMode;
	}

	inline playPos & getPlayPos( PlayModes _pm )
	{
		return m_playPos[_pm];
	}

	void updateLength( void );
	tact_t length( void ) const
	{
		return m_length;
	}


	bpm_t getTempo( void );
	virtual automationPattern * tempoAutomationPattern( void );

	automationTrack * globalAutomationTrack( void )
	{
		return m_globalAutomationTrack;
	}

	// file management
	void createNewProject( void );
	void createNewProjectFromTemplate( const QString & _template );
	void loadProject( const QString & _file_name );
	bool saveProject( void );
	bool saveProjectAs( const QString & _file_name );
	inline const QString & projectFileName( void ) const
	{
		return m_fileName;
	}
	inline bool isLoadingProject( void ) const
	{
		return m_loadingProject;
	}
	inline bool isModified( void ) const
	{
		return m_modified;
	}

	inline virtual QString nodeName( void ) const
	{
		return "song";
	}

	virtual inline bool fixedTCOs( void ) const
	{
		return false;
	}

	void addController( Controller * _c );
	void removeController( Controller * _c );
	

	const ControllerVector & controllers( void ) const
	{
		return m_controllers;
	}


	meterModel & getTimeSigModel( void )
	{
		return m_timeSigModel;
	}

	inline void toggleLoopPoints( int _mode )
	{
		emit loopPointsChanged( _mode );
	}
	
public slots:
	void play( void );
	void record( void );
	void playAndRecord( void );
	void stop( void );
	void playTrack( track * _trackToPlay );
	void playBB( void );
	void playPattern( pattern * _patternToPlay, bool _loop = true );
	void pause( void );
	void resumeFromPause( void );

	void importProject( void );
	void exportProject( void );

	void startExport( void );
	void stopExport( void );


	void setModified( void );

	void clearProject( void );


private slots:
	void insertBar( void );
	void removeBar( void );
	void addBBTrack( void );
	void addSampleTrack( void );
	void addAutomationTrack( void );

	void setTempo( void );
	void setTimeSignature( void );

	void masterVolumeChanged( void );

	void doActions( void );

	void updateFramesPerTick( void );



private:
	song( void );
	song( const song & );
	virtual ~song();


	inline int ticksPerTact( void ) const
	{
		return DefaultTicksPerTact *
				m_timeSigModel.getNumerator() /
					 m_timeSigModel.getDenominator();
	}

	inline tact_t currentTact( void ) const
	{
		return m_playPos[m_playMode].getTact();
	}

	inline tick_t currentTick( void ) const
	{
		return m_playPos[m_playMode].getTicks();
	}
	void setPlayPos( tick_t _ticks, PlayModes _play_mode );

	void saveControllerStates( QDomDocument & _doc, QDomElement & _this );
	void restoreControllerStates( const QDomElement & _this );


	automationTrack * m_globalAutomationTrack;

	intModel m_tempoModel;
	meterModel m_timeSigModel;
	int m_oldTicksPerTact;
	intModel m_masterVolumeModel;
	intModel m_masterPitchModel;

	ControllerVector m_controllers;


	QString m_fileName;
	QString m_oldFileName;
	bool m_modified;

	volatile bool m_recording;
	volatile bool m_exporting;
	volatile bool m_playing;
	volatile bool m_paused;

	bool m_loadingProject;

	PlayModes m_playMode;
	playPos m_playPos[Mode_Count];
	tact_t m_length;

	track * m_trackToPlay;
	pattern * m_patternToPlay;
	bool m_loopPattern;


	enum Actions
	{
		ActionStop,
		ActionPlaySong,
		ActionPlayTrack,
		ActionPlayBB,
		ActionPlayPattern,
		ActionPause,
		ActionResumeFromPause
	} ;
	QVector<Actions> m_actions;


	friend class engine;
	friend class songEditor;
	friend class MainWindow;
	friend class ControllerRackView;

signals:
	void lengthChanged( int _tacts );
	void tempoChanged( bpm_t _new_bpm );
	void timeSignatureChanged( int _old_ticks_per_tact,
							int _ticks_per_tact );
	void loopPointsChanged( int _mode );
} ;


#endif
