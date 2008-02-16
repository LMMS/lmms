/*
 * song.h - class song - the root of the model-tree
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "track_container.h"
#include "automatable_model.h"
#include "automatable_slider.h"
#include "lcd_spinbox.h"


class pattern;
class timeLine;


const bpm_t MIN_BPM = 10;
const bpm_t DEFAULT_BPM = 140;
const bpm_t MAX_BPM = 999;
const Uint16 MAX_SONG_LENGTH = 9999;


class song : public trackContainer
{
	Q_OBJECT
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
		playPos( Sint32 _abs = 0 ) :
			midiTime( _abs ),
			m_timeLine( NULL ),
			m_timeLineUpdate( TRUE ),
			m_currentFrame( 0.0f )
		{
		}
		inline void setCurrentFrame( const float _f )
		{
			m_currentFrame = _f;
		}
		inline float currentFrame( void ) const
		{
			return( m_currentFrame );
		}
		timeLine * m_timeLine;
		bool m_timeLineUpdate;

	private:
		float m_currentFrame;

	} ;



	void processNextBuffer( void );


	inline bool paused( void ) const
	{
		return( m_paused );
	}

	inline bool playing( void ) const
	{
		return( m_playing && m_exporting == FALSE );
	}

	inline bool exporting( void ) const
	{
		return( m_exporting );
	}

	bool realTimeTask( void ) const;

	inline bool exportDone( void ) const
	{
		return( m_exporting == TRUE &&
			m_playPos[Mode_PlaySong].getTact() >=
							lengthInTacts() + 1 );
	}

	inline PlayModes playMode( void ) const
	{
		return( m_playMode );
	}

	inline playPos & getPlayPos( PlayModes _pm )
	{
		return( m_playPos[_pm] );
	}

	tact lengthInTacts( void ) const;


	bpm_t getTempo( void );
	virtual automationPattern * tempoAutomationPattern( void );


	// file management
	void createNewProject( void );
	void createNewProjectFromTemplate( const QString & _template );
	void loadProject( const QString & _file_name );
	bool saveProject( void );
	bool saveProjectAs( const QString & _file_name );
	inline const QString & projectFileName( void ) const
	{
		return( m_fileName );
	}
	inline bool isLoadingProject( void ) const
	{
		return( m_loadingProject );
	}
	inline bool isModified( void ) const
	{
		return( m_modified );
	}

	inline virtual QString nodeName( void ) const
	{
		return( "song" );
	}

	virtual inline bool fixedTCOs( void ) const
	{
		return( FALSE );
	}


public slots:
	void play( void );
	void stop( void );
	void playTrack( track * _trackToPlay );
	void playBB( void );
	void playPattern( pattern * _patternToPlay, bool _loop = TRUE );
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

	void setTempo( void );

	void masterVolumeChanged( void );

	void doActions( void );

	void updateFramesPerTact64th( void );



private:
	song( void );
	song( const song & );
	virtual ~song();


	inline tact currentTact( void ) const
	{
		return( m_playPos[m_playMode].getTact() );
	}

	midiTime length( void ) const;
	inline tact64th currentTact64th( void ) const
	{
		return( m_playPos[m_playMode].getTact64th() );
	}
	void setPlayPos( tact _tact_num, tact64th _t_64th, PlayModes
								_play_mode );



	track * m_automationTrack;

	lcdSpinBoxModel m_tempoModel;
	sliderModel m_masterVolumeModel;
	sliderModel m_masterPitchModel;


	QString m_fileName;
	QString m_oldFileName;
	bool m_modified;

	volatile bool m_exporting;
	volatile bool m_playing;
	volatile bool m_paused;

	bool m_loadingProject;

	PlayModes m_playMode;
	playPos m_playPos[Mode_Count];

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

signals:
	void tempoChanged( bpm_t _new_bpm );

} ;


#endif
