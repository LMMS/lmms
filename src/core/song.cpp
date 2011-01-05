/*
 * song.cpp - root of the model tree
 *
 * Copyright (c) 2004-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <math.h>

#include "song.h"
#include "AutomationTrack.h"
#include "AutomationEditor.h"
#include "AutomationRecorder.h"
#include "bb_editor.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "config_mgr.h"
#include "ControllerRackView.h"
#include "ControllerConnection.h"
#include "embed.h"
#include "EnvelopeAndLfoParameters.h"
#include "ExportProjectDialog.h"
#include "FxMixer.h"
#include "FxMixerView.h"
#include "ImportFilter.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "MidiClient.h"
#include "mmp.h"
#include "note_play_handle.h"
#include "pattern.h"
#include "piano_roll.h"
#include "ProjectJournal.h"
#include "project_notes.h"
#include "ProjectRenderer.h"
#include "rename_dialog.h"
#include "song_editor.h"
#include "templates.h"
#include "text_float.h"
#include "timeline.h"


tick_t midiTime::s_ticksPerTact = DefaultTicksPerTact;



song::song() :
	trackContainer(),
	m_globalAutomationTrack( dynamic_cast<AutomationTrack *>(
				track::create( track::HiddenAutomationTrack,
								this ) ) ),
	m_tempoModel( DefaultTempo, MinTempo, MaxTempo, this, tr( "Tempo" ) ),
	m_timeSigModel( this ),
	m_oldTicksPerTact( DefaultTicksPerTact ),
	m_masterVolumeModel( 100, 0, 200, this, tr( "Master volume" ) ),
	m_masterPitchModel( 0, -12, 12, this, tr( "Master pitch" ) ),
	m_fileName(),
	m_oldFileName(),
	m_modified( false ),
	m_recording( false ),
	m_exporting( false ),
	m_playing( false ),
	m_paused( false ),
	m_loadingProject( false ),
	m_playMode( Mode_PlaySong ),
	m_length( 0 ),
	m_trackToPlay( NULL ),
	m_patternToPlay( NULL ),
	m_loopPattern( false )
{
	connect( &m_tempoModel, SIGNAL( dataChanged() ),
						this, SLOT( setTempo() ) );
	connect( &m_tempoModel, SIGNAL( dataUnchanged() ),
						this, SLOT( setTempo() ) );
	connect( &m_timeSigModel, SIGNAL( dataChanged() ),
					this, SLOT( setTimeSignature() ) );


	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ), this,
						SLOT( updateFramesPerTick() ) );


	connect( &m_masterVolumeModel, SIGNAL( dataChanged() ),
			this, SLOT( masterVolumeChanged() ) );
/*	connect( &m_masterPitchModel, SIGNAL( dataChanged() ),
			this, SLOT( masterPitchChanged() ) );*/

	qRegisterMetaType<note>( "note" );

}




song::~song()
{
	m_playing = false;
	delete m_globalAutomationTrack;
}




void song::masterVolumeChanged()
{
	engine::getMixer()->setMasterGain( m_masterVolumeModel.value() /
								100.0f );
}




void song::setTempo()
{
	const bpm_t tempo = (bpm_t) m_tempoModel.value();
	engine::getMixer()->lock();
	PlayHandleList & playHandles = engine::getMixer()->playHandles();
	for( PlayHandleList::Iterator it = playHandles.begin();
						it != playHandles.end(); ++it )
	{
		notePlayHandle * nph = dynamic_cast<notePlayHandle *>( *it );
		if( nph && !nph->released() )
		{
			nph->resize( tempo );
		}
	}
	engine::getMixer()->unlock();

	engine::updateFramesPerTick();

	emit tempoChanged( tempo );
}




void song::setTimeSignature()
{
	midiTime::setTicksPerTact( ticksPerTact() );
	emit timeSignatureChanged( m_oldTicksPerTact, ticksPerTact() );
	emit dataChanged();
	m_oldTicksPerTact = ticksPerTact();
}




void song::doActions()
{
	while( !m_actions.empty() )
	{
		switch( m_actions.front() )
		{
			case ActionStop:
			{
				timeLine * tl =
					m_playPos[m_playMode].m_timeLine;
				m_playing = false;
				m_recording = true;
				if( tl != NULL )
				{

		switch( tl->behaviourAtStop() )
		{
			case timeLine::BackToZero:
				m_playPos[m_playMode].setTicks( 0 );
				break;

			case timeLine::BackToStart:
				if( tl->savedPos() >= 0 )
				{
					m_playPos[m_playMode].setTicks(
						tl->savedPos().getTicks() );
					tl->savePos( -1 );
				}
				break;

			case timeLine::KeepStopPosition:
			default:
				break;
		}

				}
				else
				{
					m_playPos[m_playMode].setTicks( 0 );
				}

				m_playPos[m_playMode].setCurrentFrame( 0 );

				// remove all note-play-handles that are active
				engine::getMixer()->clear();

				break;
			}

			case ActionPlaySong:
				m_playMode = Mode_PlaySong;
				m_playing = true;
				Controller::resetFrameCounter();
				break;

			case ActionPlayTrack:
				m_playMode = Mode_PlayTrack;
				m_playing = true;
				break;

			case ActionPlayBB:
				m_playMode = Mode_PlayBB;
				m_playing = true;
				break;

			case ActionPlayPattern:
				m_playMode = Mode_PlayPattern;
				m_playing = true;
				break;

			case ActionPause:
				m_playing = false;// just set the play-flag
				m_paused = true;
				break;

			case ActionResumeFromPause:
				m_playing = true;// just set the play-flag
				m_paused = false;
				break;
		}

		// a second switch for saving pos when starting to play
		// anything (need pos for restoring it later in certain
		// timeline-modes)
		switch( m_actions.front() )
		{
			case ActionPlaySong:
			case ActionPlayTrack:
			case ActionPlayBB:
			case ActionPlayPattern:
			{
				timeLine * tl =
					m_playPos[m_playMode].m_timeLine;
				if( tl != NULL )
				{
					tl->savePos( m_playPos[m_playMode] );
				}
				break;
			}

			// keep GCC happy...
			default:
				break;
		}

		m_actions.erase( m_actions.begin() );

	}

}




void song::processNextBuffer()
{
	doActions();

	if( m_playing == false )
	{
		return;
	}

	trackList track_list;
	int tco_num = -1;

	switch( m_playMode )
	{
		case Mode_PlaySong:
			track_list = tracks();
			// at song-start we have to reset the LFOs
			if( m_playPos[Mode_PlaySong] == 0 )
			{
				EnvelopeAndLfoParameters::instances()->reset();
			}
			break;

		case Mode_PlayTrack:
			track_list.push_back( m_trackToPlay );
			break;

		case Mode_PlayBB:
			if( engine::getBBTrackContainer()->numOfBBs() > 0 )
			{
				tco_num = engine::getBBTrackContainer()->
								currentBB();
				track_list.push_back( bbTrack::findBBTrack(
								tco_num ) );
			}
			break;

		case Mode_PlayPattern:
			if( m_patternToPlay != NULL )
			{
				tco_num = m_patternToPlay->getTrack()->
						getTCONum( m_patternToPlay );
				track_list.push_back(
						m_patternToPlay->getTrack() );
			}
			break;

		default:
			return;

	}

	if( track_list.empty() == true )
	{
		return;
	}

	// check for looping-mode and act if necessary
	timeLine * tl = m_playPos[m_playMode].m_timeLine;
	bool check_loop = tl != NULL && m_exporting == false &&
				tl->loopPointsEnabled() &&
				!( m_playMode == Mode_PlayPattern &&
					m_patternToPlay->freezing() == true );
	if( check_loop )
	{
		if( m_playPos[m_playMode] < tl->loopBegin() ||
					m_playPos[m_playMode] >= tl->loopEnd() )
		{
			m_playPos[m_playMode].setTicks(
						tl->loopBegin().getTicks() );
		}
	}

	f_cnt_t total_frames_played = 0;
	const float frames_per_tick = engine::framesPerTick();

	while( total_frames_played
				< engine::getMixer()->framesPerPeriod() )
	{
		f_cnt_t played_frames = engine::getMixer()
				->framesPerPeriod() - total_frames_played;

		float current_frame = m_playPos[m_playMode].currentFrame();
		// did we play a tick?
		if( current_frame >= frames_per_tick )
		{
			int ticks = m_playPos[m_playMode].getTicks()
				+ (int)( current_frame / frames_per_tick );
			// did we play a whole tact?
			if( ticks >= midiTime::ticksPerTact() )
			{
				// per default we just continue playing even if
				// there's no more stuff to play
				// (song-play-mode)
				int max_tact = m_playPos[m_playMode].getTact()
									+ 2;

				// then decide whether to go over to next tact
				// or to loop back to first tact
				if( m_playMode == Mode_PlayBB )
				{
					max_tact = engine::getBBTrackContainer()
							->lengthOfCurrentBB();
				}
				else if( m_playMode == Mode_PlayPattern &&
					m_loopPattern == true &&
					tl != NULL &&
					tl->loopPointsEnabled() == false )
				{
					max_tact = m_patternToPlay->length()
								.getTact();
				}

				// end of played object reached?
				if( m_playPos[m_playMode].getTact() + 1
								>= max_tact )
				{
					// then start from beginning and keep
					// offset
					ticks = ticks % ( max_tact *
						midiTime::ticksPerTact() );
				}
			}
			m_playPos[m_playMode].setTicks( ticks );

			if( check_loop )
			{
				if( m_playPos[m_playMode] >= tl->loopEnd() )
				{
					m_playPos[m_playMode].setTicks(
						tl->loopBegin().getTicks() );
				}
			}

			current_frame = fmodf( current_frame, frames_per_tick );
			m_playPos[m_playMode].setCurrentFrame( current_frame );
		}

		f_cnt_t last_frames = (f_cnt_t)frames_per_tick -
						(f_cnt_t) current_frame;
		// skip last frame fraction
		if( last_frames == 0 )
		{
			++total_frames_played;
			m_playPos[m_playMode].setCurrentFrame( current_frame
								+ 1.0f );
			continue;
		}
		// do we have some samples left in this tick but these are
		// less then samples we have to play?
		if( last_frames < played_frames )
		{
			// then set played_samples to remaining samples, the
			// rest will be played in next loop
			played_frames = last_frames;
		}

		if( (f_cnt_t) current_frame == 0 )
		{
			if( m_playMode == Mode_PlaySong )
			{
				m_globalAutomationTrack->play(
						m_playPos[m_playMode],
						played_frames,
						total_frames_played, tco_num );
			}

			// loop through all tracks and play them
			for( int i = 0; i < track_list.size(); ++i )
			{
				track_list[i]->play( m_playPos[m_playMode],
						played_frames,
						total_frames_played, tco_num );
			}
		}

		// update frame-counters
		total_frames_played += played_frames;
		m_playPos[m_playMode].setCurrentFrame( played_frames +
								current_frame );
	}
}




bool song::realTimeTask() const
{
	return !( m_exporting == true || ( m_playMode == Mode_PlayPattern &&
		  	m_patternToPlay != NULL &&
			m_patternToPlay->freezing() == true ) );
}




void song::play()
{
	m_recording = false;
	if( m_playing == true )
	{
		if( m_playMode != Mode_PlaySong )
		{
			// make sure, bb-editor updates/resets it play-button
			engine::getBBTrackContainer()->stop();
			//pianoRoll::inst()->stop();
		}
		else
		{
			pause();
			return;
		}
	}
	m_actions.push_back( ActionPlaySong );
}




void song::record()
{
	m_recording = true;
	// TODO: Implement
}




void song::playAndRecord()
{
	play();
	m_recording = true;
}




void song::playTrack( track * _trackToPlay )
{
	if( m_playing == true )
	{
		stop();
	}
	m_trackToPlay = _trackToPlay;

	m_actions.push_back( ActionPlayTrack );
}




void song::playBB()
{
	if( m_playing == true )
	{
		stop();
	}
	m_actions.push_back( ActionPlayBB );
}




void song::playPattern( pattern * _patternToPlay, bool _loop )
{
	if( m_playing == true )
	{
		stop();
	}
	m_patternToPlay = _patternToPlay;
	m_loopPattern = _loop;
	if( m_patternToPlay != NULL )
	{
		m_actions.push_back( ActionPlayPattern );
	}
}




void song::updateLength()
{
	m_length = 0;
	m_tracksMutex.lockForRead();
	for( trackList::const_iterator it = tracks().begin();
						it != tracks().end(); ++it )
	{
		const tact_t cur = ( *it )->length();
		if( cur > m_length )
		{
			m_length = cur;
		}
	}
	m_tracksMutex.unlock();

	emit lengthChanged( m_length );
}




void song::setPlayPos( tick_t _ticks, PlayModes _play_mode )
{
	m_playPos[_play_mode].setTicks( _ticks );
	m_playPos[_play_mode].setCurrentFrame( 0.0f );
}




void song::stop()
{
	m_actions.push_back( ActionStop );

	engine::automationRecorder()->initRecord();
}






void song::pause()
{
	m_actions.push_back( ActionPause );
}




void song::resumeFromPause()
{
	m_actions.push_back( ActionResumeFromPause );
}




void song::startExport()
{
	stop();
	doActions();

	play();
	doActions();

	m_exporting = true;
}




void song::stopExport()
{
	stop();
	m_exporting = false;
}




void song::insertBar()
{
	m_tracksMutex.lockForRead();
	for( trackList::const_iterator it = tracks().begin();
					it != tracks().end(); ++it )
	{
		( *it )->insertTact( m_playPos[Mode_PlaySong] );
	}
	m_tracksMutex.unlock();
}




void song::removeBar()
{
	m_tracksMutex.lockForRead();
	for( trackList::const_iterator it = tracks().begin();
					it != tracks().end(); ++it )
	{
		( *it )->removeTact( m_playPos[Mode_PlaySong] );
	}
	m_tracksMutex.unlock();
}




void song::addBBTrack()
{
	engine::getMixer()->lock();
	track * t = track::create( track::BBTrack, this );
	engine::getBBTrackContainer()->setCurrentBB(
						bbTrack::numOfBBTrack( t ) );
	engine::getMixer()->unlock();
}




void song::addSampleTrack()
{
	engine::getMixer()->lock();
	(void) track::create( track::SampleTrack, this );
	engine::getMixer()->unlock();
}




void song::addAutomationTrack()
{
	engine::getMixer()->lock();
	(void) track::create( track::AutomationTrack, this );
	engine::getMixer()->unlock();
}




bpm_t song::getTempo()
{
	return (bpm_t) m_tempoModel.value();
}




AutomationPattern * song::tempoAutomationPattern()
{
	return AutomationPattern::globalAutomationPattern( &m_tempoModel );
}




void song::clearProject()
{
	engine::projectJournal()->setJournalling( false );

	if( m_playing )
	{
		stop();
	}

	engine::getMixer()->lock();
	engine::fxMixer()->clear();

	if( engine::getBBEditor() )
	{
		engine::getBBEditor()->clearAllTracks();
	}
	if( engine::getSongEditor() )
	{
		engine::getSongEditor()->clearAllTracks();
	}

	// depends on the fxMixer being cleared
	if( engine::fxMixerView() )
	{
		engine::fxMixerView()->clear();
	}

	QCoreApplication::sendPostedEvents();
	engine::getBBTrackContainer()->clearAllTracks();
	clearAllTracks();



	if( engine::automationEditor() )
	{
		engine::automationEditor()->setCurrentPattern( NULL );
	}

	m_tempoModel.reset();
	m_masterVolumeModel.reset();
	m_masterPitchModel.reset();
	m_timeSigModel.reset();

	AutomationPattern::globalAutomationPattern( &m_tempoModel )->clear();
	AutomationPattern::globalAutomationPattern( &m_masterVolumeModel )->
									clear();
	AutomationPattern::globalAutomationPattern( &m_masterPitchModel )->
									clear();

	engine::getMixer()->unlock();

	if( engine::getProjectNotes() )
	{
		engine::getProjectNotes()->clear();
	}

	// Move to function
	while( !m_controllers.empty() )
	{
		delete m_controllers.last();
	}

	emit dataChanged();

	engine::projectJournal()->clearJournal();

	engine::projectJournal()->setJournalling( true );
}





// create new file
void song::createNewProject()
{
	QString default_template = configManager::inst()->userProjectsDir()
						+ "templates/default.mpt";
	if( QFile::exists( default_template ) )
	{
		createNewProjectFromTemplate( default_template );
		return;
	}

	default_template = configManager::inst()->factoryProjectsDir()
						+ "templates/default.mpt";
	if( QFile::exists( default_template ) )
	{
		createNewProjectFromTemplate( default_template );
		return;
	}

	m_loadingProject = true;

	clearProject();

	engine::projectJournal()->setJournalling( false );

	m_fileName = m_oldFileName = "";

	track * t;
	t = track::create( track::InstrumentTrack, this );
	dynamic_cast<InstrumentTrack * >( t )->loadInstrument(
					"tripleoscillator" );
	t = track::create( track::InstrumentTrack,
						engine::getBBTrackContainer() );
	dynamic_cast<InstrumentTrack * >( t )->loadInstrument(
						"tripleoscillator" );
	track::create( track::SampleTrack, this );
	track::create( track::BBTrack, this );
	track::create( track::AutomationTrack, this );

	m_tempoModel.setInitValue( DefaultTempo );
	m_timeSigModel.reset();
	m_masterVolumeModel.setInitValue( 100 );
	m_masterPitchModel.setInitValue( 0 );

	QCoreApplication::instance()->processEvents();

	m_loadingProject = false;

	engine::getBBTrackContainer()->updateAfterTrackAdd();

	engine::projectJournal()->setJournalling( true );

	QCoreApplication::sendPostedEvents();

	m_modified = false;

	if( engine::mainWindow() )
	{
		engine::mainWindow()->resetWindowTitle();
	}
}




void song::createNewProjectFromTemplate( const QString & _template )
{
	loadProject( _template );
	// clear file-name so that user doesn't overwrite template when
	// saving...
	m_fileName = m_oldFileName = "";
	// update window title
	if( engine::mainWindow() )
	{
		engine::mainWindow()->resetWindowTitle();
	}

}




// load given song
void song::loadProject( const QString & _file_name )
{
	m_loadingProject = true;

	clearProject();

	engine::projectJournal()->setJournalling( false );

	m_fileName = _file_name;
	m_oldFileName = _file_name;

	multimediaProject mmp( m_fileName );
	// if file could not be opened, head-node is null and we create
	// new project
	if( mmp.head().isNull() )
	{
		createNewProject();
		return;
	}

	engine::getMixer()->lock();

	// get the header information from the DOM
	m_tempoModel.loadSettings( mmp.head(), "bpm" );
	m_timeSigModel.loadSettings( mmp.head(), "timesig" );
	m_masterVolumeModel.loadSettings( mmp.head(), "mastervol" );
	m_masterPitchModel.loadSettings( mmp.head(), "masterpitch" );

	if( m_playPos[Mode_PlaySong].m_timeLine )
	{
		// reset loop-point-state
		m_playPos[Mode_PlaySong].m_timeLine->toggleLoopPoints( 0 );
	}

	if( !mmp.content().firstChildElement( "track" ).isNull() )
	{
		m_globalAutomationTrack->restoreState( mmp.content().
						firstChildElement( "track" ) );
	}
	QDomNode node = mmp.content().firstChild();

	// walk through and fix up the mixer
	while( !node.isNull() )
	{
		if( node.nodeName() == engine::fxMixer()->nodeName() )
		{
			engine::fxMixer()->restoreState( node.toElement() );

			if( engine::hasGUI() )
			{
				// refresh FxMixerView
				engine::fxMixerView()->refreshDisplay();
			}
		}

		node = node.nextSibling();
	}

	node = mmp.content().firstChild();

	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == "trackcontainer" )
			{
				( (JournallingObject *)( this ) )->
					restoreState( node.toElement() );
			}
			else if( node.nodeName() == "controllers" )
			{
				restoreControllerStates( node.toElement() );
			}
			else if( engine::hasGUI() )
			{
				if( node.nodeName() ==
					engine::getControllerRackView()->nodeName() )
				{
					engine::getControllerRackView()->
						restoreState( node.toElement() );
				}
				else if( node.nodeName() ==
					engine::getPianoRoll()->nodeName() )
				{
					engine::getPianoRoll()->restoreState(
							node.toElement() );
				}
				else if( node.nodeName() ==
					engine::automationEditor()->
								nodeName() )
				{
					engine::automationEditor()->
						restoreState( node.toElement() );
				}
				else if( node.nodeName() ==
						engine::getProjectNotes()->
								nodeName() )
				{
					 engine::getProjectNotes()->
			SerializingObject::restoreState( node.toElement() );
				}
				else if( node.nodeName() ==
						m_playPos[Mode_PlaySong].
							m_timeLine->nodeName() )
				{
					m_playPos[Mode_PlaySong].
						m_timeLine->restoreState(
							node.toElement() );
				}
			}
		}
		node = node.nextSibling();
	}

	// quirk for fixing projects with broken positions of TCOs inside
	// BB-tracks
	engine::getBBTrackContainer()->fixIncorrectPositions();

	// Connect controller links to their controllers 
	// now that everything is loaded
	ControllerConnection::finalizeConnections();

	// resolve all IDs so that autoModels are automated
	AutomationPattern::resolveAllIDs();

	engine::getMixer()->unlock();

	configManager::inst()->addRecentlyOpenedProject( _file_name );

	engine::projectJournal()->setJournalling( true );

	emit projectLoaded();

	m_loadingProject = false;
	m_modified = false;

	if( engine::mainWindow() )
	{
		engine::mainWindow()->resetWindowTitle();
	}
}


// only save current song as _filename and do nothing else
bool song::saveProjectFile( const QString & _filename )
{
	multimediaProject mmp( multimediaProject::SongProject );

	m_tempoModel.saveSettings( mmp, mmp.head(), "bpm" );
	m_timeSigModel.saveSettings( mmp, mmp.head(), "timesig" );
	m_masterVolumeModel.saveSettings( mmp, mmp.head(), "mastervol" );
	m_masterPitchModel.saveSettings( mmp, mmp.head(), "masterpitch" );

	saveState( mmp, mmp.content() );

	m_globalAutomationTrack->saveState( mmp, mmp.content() );
	engine::fxMixer()->saveState( mmp, mmp.content() );
	if( engine::hasGUI() )
	{
		engine::getControllerRackView()->saveState( mmp, mmp.content() );
		engine::getPianoRoll()->saveState( mmp, mmp.content() );
		engine::automationEditor()->saveState( mmp, mmp.content() );
		engine::getProjectNotes()->
			SerializingObject::saveState( mmp, mmp.content() );
		m_playPos[Mode_PlaySong].m_timeLine->saveState(
							mmp, mmp.content() );
	}

	saveControllerStates( mmp, mmp.content() );

    return mmp.writeFile( _filename );
}



// save current song and update the gui
bool song::guiSaveProject()
{
	multimediaProject mmp( multimediaProject::SongProject );
	m_fileName = mmp.nameWithExtension( m_fileName );
	if( saveProjectFile( m_fileName ) && engine::hasGUI() )
	{
		textFloat::displayMessage( tr( "Project saved" ),
					tr( "The project %1 is now saved."
							).arg( m_fileName ),
				embed::getIconPixmap( "project_save", 24, 24 ),
									2000 );
		configManager::inst()->addRecentlyOpenedProject( m_fileName );
		m_modified = false;
		engine::mainWindow()->resetWindowTitle();
	}
	else if( engine::hasGUI() )
	{
		textFloat::displayMessage( tr( "Project NOT saved." ),
				tr( "The project %1 was not saved!" ).arg(
							m_fileName ),
				embed::getIconPixmap( "error" ), 4000 );
		return false;
	}

	return true;
}




// save current song in given filename
bool song::guiSaveProjectAs( const QString & _file_name )
{
	QString o = m_oldFileName;
	m_oldFileName = m_fileName;
	m_fileName = _file_name;
	if( guiSaveProject() == false )
	{
		m_fileName = m_oldFileName;
		m_oldFileName = o;
		return false;
	}
	m_oldFileName = m_fileName;
	return true;
}




void song::importProject()
{
	QFileDialog ofd( NULL, tr( "Import file" ),
			configManager::inst()->userProjectsDir(),
			tr("MIDI sequences") +
			" (*.mid *.midi *.rmi);;" +
			tr("FL Studio projects") +
			" (*.flp"
#ifdef LMMS_HAVE_ZIP	
				" *.zip);;" +
#else 
			 	");;" +
#endif
			tr("All file types") +
			" (*.*)");

	ofd.setFileMode( QFileDialog::ExistingFiles );
	if( ofd.exec () == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		ImportFilter::import( ofd.selectedFiles()[0], this );
	}
}




void song::saveControllerStates( QDomDocument & _doc, QDomElement & _this )
{
	// save settings of controllers
	QDomElement controllersNode =_doc.createElement( "controllers" );
	_this.appendChild( controllersNode );
	for( int i = 0; i < m_controllers.size(); ++i )
	{
		m_controllers[i]->saveState( _doc, controllersNode );
	}
}




void song::restoreControllerStates( const QDomElement & _this )
{
	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		addController( Controller::create( node.toElement(), this ) );

		node = node.nextSibling();
	}
}




void song::exportProject()
{
	if( isEmpty() )
	{
		QMessageBox::information( engine::mainWindow(),
				tr( "Empty project" ),
				tr( "This project is empty so exporting makes "
					"no sense. Please put some items into "
					"Song Editor first!" ) );
		return;
	}

	QFileDialog efd( engine::mainWindow() );
	efd.setFileMode( QFileDialog::AnyFile );
	efd.setAcceptMode( QFileDialog::AcceptSave );
	int idx = 0;
	QStringList types;
	while( __fileEncodeDevices[idx].m_fileFormat !=
					ProjectRenderer::NumFileFormats )
	{
		types << tr( __fileEncodeDevices[idx].m_description );
		++idx;
	}
	efd.setFilters( types );

	QString base_filename;
	if( !m_fileName.isEmpty() )
	{
		efd.setDirectory( QFileInfo( m_fileName ).absolutePath() );
		base_filename = QFileInfo( m_fileName ).completeBaseName();
	}
	else
	{
		efd.setDirectory( configManager::inst()->userProjectsDir() );
		base_filename = tr( "untitled" );
	}
	efd.selectFile( base_filename + __fileEncodeDevices[0].m_extension );
	efd.setWindowTitle( tr( "Select file for project-export..." ) );

	if( efd.exec() == QDialog::Accepted &&
		!efd.selectedFiles().isEmpty() && !efd.selectedFiles()[0].isEmpty() )
	{
		const QString export_file_name = efd.selectedFiles()[0];
		ExportProjectDialog epd( export_file_name,
						engine::mainWindow() );
		epd.exec();
	}
}




void song::updateFramesPerTick()
{
	engine::updateFramesPerTick();
}




void song::setModified()
{
	if( !m_loadingProject )
	{
		m_modified = true;
		if( engine::mainWindow() &&
			QThread::currentThread() ==
					engine::mainWindow()->thread() )
		{
			engine::mainWindow()->resetWindowTitle();
		}
	}
}




void song::addController( Controller * _c )
{
	if( _c != NULL && !m_controllers.contains( _c ) ) 
	{
		m_controllers.append( _c );
		emit dataChanged();
	}
}




void song::removeController( Controller * _controller )
{
	int index = m_controllers.indexOf( _controller );
	if( index != -1 )
	{
		m_controllers.remove( index );

		if( engine::getSong() )
		{
			engine::getSong()->setModified();
		}
		emit dataChanged();
	}
}



#include "moc_song.cxx"


