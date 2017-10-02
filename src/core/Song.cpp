/*
 * Song.cpp - root of the model tree
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

#include "Song.h"
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include <functional>

#include "AutomationTrack.h"
#include "AutomationEditor.h"
#include "BBEditor.h"
#include "BBTrack.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "ControllerRackView.h"
#include "ControllerConnection.h"
#include "embed.h"
#include "EnvelopeAndLfoParameters.h"
#include "ExportProjectDialog.h"
#include "FxMixer.h"
#include "FxMixerView.h"
#include "GuiApplication.h"
#include "ImportFilter.h"
#include "ExportFilter.h"
#include "MainWindow.h"
#include "FileDialog.h"
#include "Pattern.h"
#include "PianoRoll.h"
#include "ProjectJournal.h"
#include "ProjectNotes.h"
#include "SongEditor.h"
#include "TextFloat.h"
#include "TimeLineWidget.h"
#include "PeakController.h"
#include "VersionedSaveDialog.h"


tick_t MidiTime::s_ticksPerTact = DefaultTicksPerTact;



Song::Song() :
	TrackContainer(),
	m_globalAutomationTrack( dynamic_cast<AutomationTrack *>(
				Track::create( Track::HiddenAutomationTrack,
								this ) ) ),
	m_tempoModel( DefaultTempo, MinTempo, MaxTempo, this, tr( "Tempo" ) ),
	m_timeSigModel( this ),
	m_oldTicksPerTact( DefaultTicksPerTact ),
	m_masterVolumeModel( 100, 0, 200, this, tr( "Master volume" ) ),
	m_masterPitchModel( 0, -12, 12, this, tr( "Master pitch" ) ),
	m_metaData(),
	m_fileName(),
	m_oldFileName(),
	m_modified( false ),
	m_loadOnLaunch( true ),
	m_recording( false ),
	m_exporting( false ),
	m_exportLoop( false ),
	m_renderBetweenMarkers( false ),
	m_playing( false ),
	m_paused( false ),
	m_loadingProject( false ),
	m_playMode( Mode_None ),
	m_length( 0 ),
	m_patternToPlay( NULL ),
	m_loopPattern( false ),
	m_elapsedMilliSeconds( 0 ),
	m_elapsedTicks( 0 ),
	m_elapsedTacts( 0 )
{
	connect( &m_tempoModel, SIGNAL( dataChanged() ),
						this, SLOT( setTempo() ) );
	connect( &m_tempoModel, SIGNAL( dataUnchanged() ),
						this, SLOT( setTempo() ) );
	connect( &m_timeSigModel, SIGNAL( dataChanged() ),
					this, SLOT( setTimeSignature() ) );


	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ), this,
						SLOT( updateFramesPerTick() ) );

	connect( &m_masterVolumeModel, SIGNAL( dataChanged() ),
			this, SLOT( masterVolumeChanged() ) );
/*	connect( &m_masterPitchModel, SIGNAL( dataChanged() ),
			this, SLOT( masterPitchChanged() ) );*/

	qRegisterMetaType<Note>( "Note" );
	setType( SongContainer );
}




Song::~Song()
{
	m_playing = false;
	delete m_globalAutomationTrack;
}




void Song::masterVolumeChanged()
{
	Engine::mixer()->setMasterGain( m_masterVolumeModel.value() /
								100.0f );
}




void Song::setTempo()
{
	Engine::mixer()->requestChangeInModel();
	const bpm_t tempo = ( bpm_t ) m_tempoModel.value();
	PlayHandleList & playHandles = Engine::mixer()->playHandles();
	for( PlayHandleList::Iterator it = playHandles.begin();
						it != playHandles.end(); ++it )
	{
		NotePlayHandle * nph = dynamic_cast<NotePlayHandle *>( *it );
		if( nph && !nph->isReleased() )
		{
			nph->lock();
			nph->resize( tempo );
			nph->unlock();
		}
	}
	Engine::mixer()->doneChangeInModel();

	Engine::updateFramesPerTick();

	m_vstSyncController.setTempo( tempo );

	emit tempoChanged( tempo );
}




void Song::setTimeSignature()
{
	MidiTime::setTicksPerTact( ticksPerTact() );
	emit timeSignatureChanged( m_oldTicksPerTact, ticksPerTact() );
	emit dataChanged();
	m_oldTicksPerTact = ticksPerTact();

	m_vstSyncController.setTimeSignature(
		getTimeSigModel().getNumerator(), getTimeSigModel().getDenominator() );
}




void Song::savePos()
{
	TimeLineWidget * tl = m_playPos[m_playMode].m_timeLine;

	if( tl != NULL )
	{
		tl->savePos( m_playPos[m_playMode] );
	}
}




void Song::processNextBuffer()
{
	// if not playing, nothing to do
	if( m_playing == false )
	{
		return;
	}

	TrackList trackList;
	int tcoNum = -1; // track content object number

	// determine the list of tracks to play and the track content object
	// (TCO) number
	switch( m_playMode )
	{
		case Mode_PlaySong:
			trackList = tracks();
			// at song-start we have to reset the LFOs
			if( m_playPos[Mode_PlaySong] == 0 )
			{
				EnvelopeAndLfoParameters::instances()->reset();
			}
			break;

		case Mode_PlayBB:
			if( Engine::getBBTrackContainer()->numOfBBs() > 0 )
			{
				tcoNum = Engine::getBBTrackContainer()->
								currentBB();
				trackList.push_back( BBTrack::findBBTrack(
								tcoNum ) );
			}
			break;

		case Mode_PlayPattern:
			if( m_patternToPlay != NULL )
			{
				tcoNum = m_patternToPlay->getTrack()->
						getTCONum( m_patternToPlay );
				trackList.push_back(
						m_patternToPlay->getTrack() );
			}
			break;

		default:
			return;

	}

	// if we have no tracks to play, nothing to do
	if( trackList.empty() == true )
	{
		return;
	}

	// check for looping-mode and act if necessary
	TimeLineWidget * tl = m_playPos[m_playMode].m_timeLine;
	bool checkLoop =
		tl != NULL && m_exporting == false && tl->loopPointsEnabled();

	if( checkLoop )
	{
		// if looping-mode is enabled and we are outside of the looping
		// range, go to the beginning of the range
		if( m_playPos[m_playMode] < tl->loopBegin() ||
					m_playPos[m_playMode] >= tl->loopEnd() )
		{
			setToTime(tl->loopBegin());
			m_playPos[m_playMode].setTicks(
						tl->loopBegin().getTicks() );
			emit updateSampleTracks();
		}
	}

	f_cnt_t framesPlayed = 0;
	const float framesPerTick = Engine::framesPerTick();

	while( framesPlayed < Engine::mixer()->framesPerPeriod() )
	{
		m_vstSyncController.update();

		float currentFrame = m_playPos[m_playMode].currentFrame();
		// did we play a tick?
		if( currentFrame >= framesPerTick )
		{
			int ticks = m_playPos[m_playMode].getTicks() +
				( int )( currentFrame / framesPerTick );

			m_vstSyncController.setAbsolutePosition( ticks );

			// did we play a whole tact?
			if( ticks >= MidiTime::ticksPerTact() )
			{
				// per default we just continue playing even if
				// there's no more stuff to play
				// (song-play-mode)
				int maxTact = m_playPos[m_playMode].getTact()
									+ 2;

				// then decide whether to go over to next tact
				// or to loop back to first tact
				if( m_playMode == Mode_PlayBB )
				{
					maxTact = Engine::getBBTrackContainer()
							->lengthOfCurrentBB();
				}
				else if( m_playMode == Mode_PlayPattern &&
					m_loopPattern == true &&
					tl != NULL &&
					tl->loopPointsEnabled() == false )
				{
					maxTact = m_patternToPlay->length()
								.getTact();
				}

				// end of played object reached?
				if( m_playPos[m_playMode].getTact() + 1
								>= maxTact )
				{
					// then start from beginning and keep
					// offset
					ticks %= ( maxTact * MidiTime::ticksPerTact() );

					// wrap milli second counter
					setToTimeByTicks(ticks);

					m_vstSyncController.setAbsolutePosition( ticks );
				}
			}
			m_playPos[m_playMode].setTicks( ticks );

			if( checkLoop )
			{
				m_vstSyncController.startCycle(
					tl->loopBegin().getTicks(), tl->loopEnd().getTicks() );

				// if looping-mode is enabled and we have got
				// past the looping range, return to the
				// beginning of the range
				if( m_playPos[m_playMode] >= tl->loopEnd() )
				{
					m_playPos[m_playMode].setTicks( tl->loopBegin().getTicks() );
					setToTime(tl->loopBegin());
				}
				else if( m_playPos[m_playMode] == tl->loopEnd() - 1 )
				{
					emit updateSampleTracks();
				}
			}
			else
			{
				m_vstSyncController.stopCycle();
			}

			currentFrame = fmodf( currentFrame, framesPerTick );
			m_playPos[m_playMode].setCurrentFrame( currentFrame );
		}

		f_cnt_t framesToPlay =
			Engine::mixer()->framesPerPeriod() - framesPlayed;

		f_cnt_t framesLeft = ( f_cnt_t )framesPerTick -
						( f_cnt_t )currentFrame;
		// skip last frame fraction
		if( framesLeft == 0 )
		{
			++framesPlayed;
			m_playPos[m_playMode].setCurrentFrame( currentFrame
								+ 1.0f );
			continue;
		}
		// do we have samples left in this tick but these are less
		// than samples we have to play?
		if( framesLeft < framesToPlay )
		{
			// then set framesToPlay to remaining samples, the
			// rest will be played in next loop
			framesToPlay = framesLeft;
		}

		if( ( f_cnt_t ) currentFrame == 0 )
		{
			processAutomations(trackList, m_playPos[m_playMode], framesToPlay);

			// loop through all tracks and play them
			for( int i = 0; i < trackList.size(); ++i )
			{
				trackList[i]->play( m_playPos[m_playMode],
						framesToPlay,
						framesPlayed, tcoNum );
			}
		}

		// update frame-counters
		framesPlayed += framesToPlay;
		m_playPos[m_playMode].setCurrentFrame( framesToPlay +
								currentFrame );
		m_elapsedMilliSeconds += MidiTime::ticksToMilliseconds( framesToPlay / framesPerTick, getTempo());
		m_elapsedTacts = m_playPos[Mode_PlaySong].getTact();
		m_elapsedTicks = ( m_playPos[Mode_PlaySong].getTicks() % ticksPerTact() ) / 48;
	}
}


void Song::processAutomations(const TrackList &tracklist, MidiTime timeStart, fpp_t)
{
	AutomatedValueMap values;

	QSet<const AutomatableModel*> recordedModels;

	TrackContainer* container = this;
	int tcoNum = -1;

	switch (m_playMode)
	{
	case Mode_PlaySong:
		break;
	case Mode_PlayBB:
	{
		Q_ASSERT(tracklist.size() == 1);
		Q_ASSERT(tracklist.at(0)->type() == Track::BBTrack);
		auto bbTrack = dynamic_cast<BBTrack*>(tracklist.at(0));
		auto bbContainer = Engine::getBBTrackContainer();
		container = bbContainer;
		tcoNum = bbTrack->index();
	}
		break;
	default:
		return;
	}

	values = container->automatedValuesAt(timeStart, tcoNum);
	TrackList tracks = container->tracks();

	Track::tcoVector tcos;
	for (Track* track : tracks)
	{
		if (track->type() == Track::AutomationTrack) {
			track->getTCOsInRange(tcos, 0, timeStart);
		}
	}

	// Process recording
	for (TrackContentObject* tco : tcos)
	{
		auto p = dynamic_cast<AutomationPattern *>(tco);
		MidiTime relTime = timeStart - p->startPosition();
		if (p->isRecording() && relTime >= 0 && relTime < p->length())
		{
			const AutomatableModel* recordedModel = p->firstObject();
			p->recordValue(relTime, recordedModel->value<float>());

			recordedModels << recordedModel;
		}
	}

	// Apply values
	for (auto it = values.begin(); it != values.end(); it++)
	{
		if (! recordedModels.contains(it.key()))
		{
			it.key()->setAutomatedValue(it.value());
		}
	}
}

std::pair<MidiTime, MidiTime> Song::getExportEndpoints() const
{
	if ( m_renderBetweenMarkers )
	{
		return std::pair<MidiTime, MidiTime>(
			m_playPos[Mode_PlaySong].m_timeLine->loopBegin(),
			m_playPos[Mode_PlaySong].m_timeLine->loopEnd()
		);
	}
	else if ( m_exportLoop )
	{
		return std::pair<MidiTime, MidiTime>( MidiTime(0, 0), MidiTime(m_length, 0) );
	}
	else
	{
		// if not exporting as a loop, we leave one bar of padding at the end of the song to accomodate reverb, etc.
		return std::pair<MidiTime, MidiTime>( MidiTime(0, 0), MidiTime(m_length+1, 0) );
	}
}




void Song::playSong()
{
	m_recording = false;

	if( isStopped() == false )
	{
		stop();
	}

	m_playMode = Mode_PlaySong;
	m_playing = true;
	m_paused = false;

	m_vstSyncController.setPlaybackState( true );

	savePos();

	emit playbackStateChanged();
}




void Song::record()
{
	m_recording = true;
	// TODO: Implement
}




void Song::playAndRecord()
{
	playSong();
	m_recording = true;
}




void Song::playBB()
{
	if( isStopped() == false )
	{
		stop();
	}

	m_playMode = Mode_PlayBB;
	m_playing = true;
	m_paused = false;

	m_vstSyncController.setPlaybackState( true );

	savePos();

	emit playbackStateChanged();
}




void Song::playPattern( const Pattern* patternToPlay, bool loop )
{
	if( isStopped() == false )
	{
		stop();
	}

	m_patternToPlay = patternToPlay;
	m_loopPattern = loop;

	if( m_patternToPlay != NULL )
	{
		m_playMode = Mode_PlayPattern;
		m_playing = true;
		m_paused = false;
	}

	savePos();

	emit playbackStateChanged();
}




void Song::updateLength()
{
	m_length = 0;
	m_tracksMutex.lockForRead();
	for( TrackList::const_iterator it = tracks().begin();
						it != tracks().end(); ++it )
	{
		if( Engine::getSong()->isExporting() &&
				( *it )->isMuted() )
		{
			continue;
		}

		const tact_t cur = ( *it )->length();
		if( cur > m_length )
		{
			m_length = cur;
		}
	}
	m_tracksMutex.unlock();

	emit lengthChanged( m_length );
}




void Song::setPlayPos( tick_t ticks, PlayModes playMode )
{
	tick_t ticksFromPlayMode = m_playPos[playMode].getTicks();
	m_elapsedTicks += ticksFromPlayMode - ticks;
	m_elapsedMilliSeconds += MidiTime::ticksToMilliseconds( ticks - ticksFromPlayMode, getTempo() );
	m_playPos[playMode].setTicks( ticks );
	m_playPos[playMode].setCurrentFrame( 0.0f );

// send a signal if playposition changes during playback
	if( isPlaying() )
	{
		emit playbackPositionChanged();
		emit updateSampleTracks();
	}
}




void Song::togglePause()
{
	if( m_paused == true )
	{
		m_playing = true;
		m_paused = false;
	}
	else
	{
		m_playing = false;
		m_paused = true;
	}

	m_vstSyncController.setPlaybackState( m_playing );

	emit playbackStateChanged();
}




void Song::stop()
{
	// do not stop/reset things again if we're stopped already
	if( m_playMode == Mode_None )
	{
		return;
	}

	TimeLineWidget * tl = m_playPos[m_playMode].m_timeLine;
	m_paused = false;
	m_recording = true;

	if( tl != NULL )
	{

		switch( tl->behaviourAtStop() )
		{
			case TimeLineWidget::BackToZero:
				m_playPos[m_playMode].setTicks( 0 );
				m_elapsedMilliSeconds = 0;
				if( gui && gui->songEditor() &&
						( tl->autoScroll() == TimeLineWidget::AutoScrollEnabled ) )
				{
					gui->songEditor()->m_editor->updatePosition(0);
				}
				break;

			case TimeLineWidget::BackToStart:
				if( tl->savedPos() >= 0 )
				{
					m_playPos[m_playMode].setTicks( tl->savedPos().getTicks() );
					setToTime(tl->savedPos());

					if( gui && gui->songEditor() &&
							( tl->autoScroll() == TimeLineWidget::AutoScrollEnabled ) )
					{
						gui->songEditor()->m_editor->updatePosition( MidiTime(tl->savedPos().getTicks() ) );
					}
					tl->savePos( -1 );
				}
				break;

			case TimeLineWidget::KeepStopPosition:
			default:
				break;
		}
	}
	else
	{
		m_playPos[m_playMode].setTicks( 0 );
		m_elapsedMilliSeconds = 0;
	}
	m_playing = false;

	m_playPos[m_playMode].setCurrentFrame( 0 );

	m_vstSyncController.setPlaybackState( m_exporting );
	m_vstSyncController.setAbsolutePosition( m_playPos[m_playMode].getTicks() );

	// remove all note-play-handles that are active
	Engine::mixer()->clear();

	m_playMode = Mode_None;

	emit playbackStateChanged();
}




void Song::startExport()
{
	stop();
	if(m_renderBetweenMarkers)
	{
		m_playPos[Mode_PlaySong].setTicks( m_playPos[Mode_PlaySong].m_timeLine->loopBegin().getTicks() );
	}
	else
	{
		m_playPos[Mode_PlaySong].setTicks( 0 );
	}

	playSong();

	m_exporting = true;

	m_vstSyncController.setPlaybackState( true );
}




void Song::stopExport()
{
	stop();
	m_exporting = false;
	m_exportLoop = false;

	m_vstSyncController.setPlaybackState( m_playing );
}




void Song::insertBar()
{
	m_tracksMutex.lockForRead();
	for( TrackList::const_iterator it = tracks().begin();
					it != tracks().end(); ++it )
	{
		( *it )->insertTact( m_playPos[Mode_PlaySong] );
	}
	m_tracksMutex.unlock();
}




void Song::removeBar()
{
	m_tracksMutex.lockForRead();
	for( TrackList::const_iterator it = tracks().begin();
					it != tracks().end(); ++it )
	{
		( *it )->removeTact( m_playPos[Mode_PlaySong] );
	}
	m_tracksMutex.unlock();
}




void Song::addBBTrack()
{
	Track * t = Track::create( Track::BBTrack, this );
	Engine::getBBTrackContainer()->setCurrentBB( dynamic_cast<BBTrack *>( t )->index() );
}




void Song::addSampleTrack()
{
	( void )Track::create( Track::SampleTrack, this );
}




void Song::addAutomationTrack()
{
	( void )Track::create( Track::AutomationTrack, this );
}




bpm_t Song::getTempo()
{
	return ( bpm_t )m_tempoModel.value();
}




AutomationPattern * Song::tempoAutomationPattern()
{
	return AutomationPattern::globalAutomationPattern( &m_tempoModel );
}


AutomatedValueMap Song::automatedValuesAt(MidiTime time, int tcoNum) const
{
	return TrackContainer::automatedValuesFromTracks(TrackList(tracks()) << m_globalAutomationTrack, time, tcoNum);
}




void Song::clearProject()
{
	Engine::projectJournal()->setJournalling( false );

	if( m_playing )
	{
		stop();
	}

	for( int i = 0; i < Mode_Count; i++ )
	{
		setPlayPos( 0, ( PlayModes )i );
	}


	Engine::mixer()->requestChangeInModel();

	if( gui && gui->getBBEditor() )
	{
		gui->getBBEditor()->trackContainerView()->clearAllTracks();
	}
	if( gui && gui->songEditor() )
	{
		gui->songEditor()->m_editor->clearAllTracks();
	}
	if( gui && gui->fxMixerView() )
	{
		gui->fxMixerView()->clear();
	}
	QCoreApplication::sendPostedEvents();
	Engine::getBBTrackContainer()->clearAllTracks();
	clearAllTracks();
	clearSongMetaData();

	Engine::fxMixer()->clear();

	if( gui && gui->automationEditor() )
	{
		gui->automationEditor()->setCurrentPattern( NULL );
	}

	if( gui && gui->pianoRoll() )
	{
		gui->pianoRoll()->reset();
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

	Engine::mixer()->doneChangeInModel();

	if( gui && gui->getProjectNotes() )
	{
		gui->getProjectNotes()->clear();
	}

	removeAllControllers();

	emit dataChanged();

	Engine::projectJournal()->clearJournal();

	Engine::projectJournal()->setJournalling( true );

	InstrumentTrackView::cleanupWindowCache();
}




// create new file
void Song::createNewProject()
{

	QString defaultTemplate = ConfigManager::inst()->userTemplateDir()
						+ "default.mpt";


	if( QFile::exists( defaultTemplate ) )
	{
		createNewProjectFromTemplate( defaultTemplate );
		return;
	}

	defaultTemplate = ConfigManager::inst()->factoryProjectsDir()
						+ "templates/default.mpt";
	if( QFile::exists( defaultTemplate ) )
	{
		createNewProjectFromTemplate( defaultTemplate );
		return;
	}

	m_loadingProject = true;

	clearProject();

	Engine::projectJournal()->setJournalling( false );

	m_fileName = m_oldFileName = "";

	Track * t;
	t = Track::create( Track::InstrumentTrack, this );
	dynamic_cast<InstrumentTrack * >( t )->loadInstrument(
					"tripleoscillator" );
	t = Track::create( Track::InstrumentTrack,
						Engine::getBBTrackContainer() );
	dynamic_cast<InstrumentTrack * >( t )->loadInstrument(
						"kicker" );
	Track::create( Track::SampleTrack, this );
	Track::create( Track::BBTrack, this );
	Track::create( Track::AutomationTrack, this );

	m_tempoModel.setInitValue( DefaultTempo );
	m_timeSigModel.reset();
	m_masterVolumeModel.setInitValue( 100 );
	m_masterPitchModel.setInitValue( 0 );

	QCoreApplication::instance()->processEvents();

	m_loadingProject = false;

	Engine::getBBTrackContainer()->updateAfterTrackAdd();

	Engine::projectJournal()->setJournalling( true );

	QCoreApplication::sendPostedEvents();

	m_modified = false;
	m_loadOnLaunch = false;

	if( gui->mainWindow() )
	{
		gui->mainWindow()->resetWindowTitle();
	}
}




void Song::createNewProjectFromTemplate( const QString & templ )
{
	loadProject( templ );
	// clear file-name so that user doesn't overwrite template when
	// saving...
	m_fileName = m_oldFileName = "";
	// update window title
	m_loadOnLaunch = false;
	if( gui->mainWindow() )
	{
		gui->mainWindow()->resetWindowTitle();
	}
}




// load given song
void Song::loadProject( const QString & fileName )
{
	QDomNode node;

	m_loadingProject = true;

	Engine::projectJournal()->setJournalling( false );

	m_oldFileName = m_fileName;
	m_fileName = fileName;

	DataFile dataFile( m_fileName );
	// if file could not be opened, head-node is null and we create
	// new project
	if( dataFile.head().isNull() )
	{
		if( m_loadOnLaunch )
		{
			createNewProject();
		}
		m_fileName = m_oldFileName;
		return;
	}

	m_oldFileName = m_fileName;

	clearProject();

	clearErrors();

	DataFile::LocaleHelper localeHelper( DataFile::LocaleHelper::ModeLoad );

	Engine::mixer()->requestChangeInModel();

	// get the header information from the DOM
	m_tempoModel.loadSettings( dataFile.head(), "bpm" );
	m_timeSigModel.loadSettings( dataFile.head(), "timesig" );
	m_masterVolumeModel.loadSettings( dataFile.head(), "mastervol" );
	m_masterPitchModel.loadSettings( dataFile.head(), "masterpitch" );

	if(m_metaData.load(dataFile,dataFile.head()))
		emit metaDataChanged();

	if( m_playPos[Mode_PlaySong].m_timeLine )
	{
		// reset loop-point-state
		m_playPos[Mode_PlaySong].m_timeLine->toggleLoopPoints( 0 );
	}

	if( !dataFile.content().firstChildElement( "track" ).isNull() )
	{
		m_globalAutomationTrack->restoreState( dataFile.content().
						firstChildElement( "track" ) );
	}

	//Backward compatibility for LMMS <= 0.4.15
	PeakController::initGetControllerBySetting();

	// Load mixer first to be able to set the correct range for FX channels
	node = dataFile.content().firstChildElement( Engine::fxMixer()->nodeName() );
	if( !node.isNull() )
	{
		Engine::fxMixer()->restoreState( node.toElement() );
		if( gui )
		{
			// refresh FxMixerView
			gui->fxMixerView()->refreshDisplay();
		}
	}

	node = dataFile.content().firstChild();

	QDomNodeList tclist=dataFile.content().elementsByTagName("trackcontainer");
	m_nLoadingTrack=0;
	for( int i=0,n=tclist.count(); i<n; ++i )
	{
		QDomNode nd=tclist.at(i).firstChild();
		while(!nd.isNull())
		{
			if( nd.isElement() && nd.nodeName() == "track" )
			{
				++m_nLoadingTrack;
				if( nd.toElement().attribute("type").toInt() == Track::BBTrack )
				{
					n += nd.toElement().elementsByTagName("bbtrack").at(0)
						.toElement().firstChildElement().childNodes().count();
				}
				nd=nd.nextSibling();
			}
		}
	}

	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == "trackcontainer" )
			{
				( (JournallingObject *)( this ) )->restoreState( node.toElement() );
			}
			else if( node.nodeName() == "controllers" )
			{
				restoreControllerStates( node.toElement() );
			}
			else if( gui )
			{
				if( node.nodeName() == gui->getControllerRackView()->nodeName() )
				{
					gui->getControllerRackView()->restoreState( node.toElement() );
				}
				else if( node.nodeName() == gui->pianoRoll()->nodeName() )
				{
					gui->pianoRoll()->restoreState( node.toElement() );
				}
				else if( node.nodeName() == gui->automationEditor()->m_editor->nodeName() )
				{
					gui->automationEditor()->m_editor->restoreState( node.toElement() );
				}
				else if( node.nodeName() == gui->getProjectNotes()->nodeName() )
				{
					 gui->getProjectNotes()->SerializingObject::restoreState( node.toElement() );
				}
				else if( node.nodeName() == m_playPos[Mode_PlaySong].m_timeLine->nodeName() )
				{
					m_playPos[Mode_PlaySong].m_timeLine->restoreState( node.toElement() );
				}
			}
		}
		node = node.nextSibling();
	}

	// quirk for fixing projects with broken positions of TCOs inside
	// BB-tracks
	Engine::getBBTrackContainer()->fixIncorrectPositions();

	// Connect controller links to their controllers
	// now that everything is loaded
	ControllerConnection::finalizeConnections();

	// resolve all IDs so that autoModels are automated
	AutomationPattern::resolveAllIDs();


	Engine::mixer()->doneChangeInModel();

	ConfigManager::inst()->addRecentlyOpenedProject( fileName );

	Engine::projectJournal()->setJournalling( true );

	emit projectLoaded();

	if ( hasErrors())
	{
		if ( gui )
		{
			QMessageBox::warning( NULL, tr("LMMS Error report"), errorSummary(),
							QMessageBox::Ok );
		}
		else
		{
			QTextStream(stderr) << Engine::getSong()->errorSummary() << endl;
		}
	}

	m_loadingProject = false;
	m_modified = false;
	m_loadOnLaunch = false;

	if( gui && gui->mainWindow() )
	{
		gui->mainWindow()->resetWindowTitle();
	}
}


// only save current song as _filename and do nothing else
bool Song::saveProjectFile( const QString & filename )
{
	DataFile::LocaleHelper localeHelper( DataFile::LocaleHelper::ModeSave );

	DataFile dataFile( DataFile::SongProject );

	m_tempoModel.saveSettings( dataFile, dataFile.head(), "bpm" );
	m_timeSigModel.saveSettings( dataFile, dataFile.head(), "timesig" );
	m_masterVolumeModel.saveSettings( dataFile, dataFile.head(), "mastervol" );
	m_masterPitchModel.saveSettings( dataFile, dataFile.head(), "masterpitch" );

	m_metaData.save(dataFile,dataFile.head());

	saveState( dataFile, dataFile.content() );

	m_globalAutomationTrack->saveState( dataFile, dataFile.content() );
	Engine::fxMixer()->saveState( dataFile, dataFile.content() );
	if( gui )
	{
		gui->getControllerRackView()->saveState( dataFile, dataFile.content() );
		gui->pianoRoll()->saveState( dataFile, dataFile.content() );
		gui->automationEditor()->m_editor->saveState( dataFile, dataFile.content() );
		gui->getProjectNotes()->SerializingObject::saveState( dataFile, dataFile.content() );
		m_playPos[Mode_PlaySong].m_timeLine->saveState( dataFile, dataFile.content() );
	}

	saveControllerStates( dataFile, dataFile.content() );

	return dataFile.writeFile( filename );
}



// save current song and update the gui
bool Song::guiSaveProject()
{
	DataFile dataFile( DataFile::SongProject );
	m_fileName = dataFile.nameWithExtension( m_fileName );
	if( saveProjectFile( m_fileName ) && gui != nullptr )
	{
		TextFloat::displayMessage( tr( "Project saved" ),
					tr( "The project %1 is now saved."
							).arg( m_fileName ),
				embed::getIconPixmap( "project_save", 24, 24 ),
									2000 );
		ConfigManager::inst()->addRecentlyOpenedProject( m_fileName );
		m_modified = false;
		gui->mainWindow()->resetWindowTitle();
	}
	else if( gui != nullptr )
	{
		TextFloat::displayMessage( tr( "Project NOT saved." ),
				tr( "The project %1 was not saved!" ).arg(
							m_fileName ),
				embed::getIconPixmap( "error" ), 4000 );
		return false;
	}

	return true;
}




// save current song in given filename
bool Song::guiSaveProjectAs( const QString & _file_name )
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




void Song::importProject()
{
	FileDialog ofd( NULL, tr( "Import file" ),
			ConfigManager::inst()->userProjectsDir(),
			tr("MIDI sequences") +
			" (*.mid *.midi *.rmi);;" +
			tr("Hydrogen projects") +
			" (*.h2song);;" +
			tr("All file types") +
			" (*.*)");

	ofd.setFileMode( FileDialog::ExistingFiles );
	if( ofd.exec () == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		ImportFilter::import( ofd.selectedFiles()[0], this );
	}
	m_loadOnLaunch = false;
}




void Song::saveControllerStates( QDomDocument & doc, QDomElement & element )
{
	// save settings of controllers
	QDomElement controllersNode = doc.createElement( "controllers" );
	element.appendChild( controllersNode );
	for( int i = 0; i < m_controllers.size(); ++i )
	{
		m_controllers[i]->saveState( doc, controllersNode );
	}
}




void Song::restoreControllerStates( const QDomElement & element )
{
	QDomNode node = element.firstChild();
	while( !node.isNull() )
	{
		Controller * c = Controller::create( node.toElement(), this );
		Q_ASSERT( c != NULL );

		addController( c );

		node = node.nextSibling();
	}
}



void Song::removeAllControllers()
{
	while (m_controllers.size() != 0)
	{
		removeController(m_controllers.at(0));
	}

	m_controllers.clear();
}



void Song::exportProjectTracks()
{
	exportProject( true );
}

void Song::exportProject( bool multiExport )
{
	if( isEmpty() )
	{
		QMessageBox::information( gui->mainWindow(),
				tr( "Empty project" ),
				tr( "This project is empty so exporting makes "
					"no sense. Please put some items into "
					"Song Editor first!" ) );
		return;
	}

	FileDialog efd( gui->mainWindow() );

	if ( multiExport )
	{
		efd.setFileMode( FileDialog::Directory);
		efd.setWindowTitle( tr( "Select directory for writing exported tracks..." ) );
		if( !m_fileName.isEmpty() )
		{
			efd.setDirectory( QFileInfo( m_fileName ).absolutePath() );
		}
	}
	else
	{
		efd.setFileMode( FileDialog::AnyFile );
		int idx = 0;
		QStringList types;
		while( ProjectRenderer::fileEncodeDevices[idx].m_fileFormat != ProjectRenderer::NumFileFormats)
		{
			if(ProjectRenderer::fileEncodeDevices[idx].isAvailable()) {
				types << tr(ProjectRenderer::fileEncodeDevices[idx].m_description);
			}
			++idx;
		}
		efd.setNameFilters( types );
		QString baseFilename;
		if( !m_fileName.isEmpty() )
		{
			efd.setDirectory( QFileInfo( m_fileName ).absolutePath() );
			baseFilename = QFileInfo( m_fileName ).completeBaseName();
		}
		else
		{
			efd.setDirectory( ConfigManager::inst()->userProjectsDir() );
			baseFilename = tr( "untitled" );
		}
		efd.selectFile( baseFilename + ProjectRenderer::fileEncodeDevices[0].m_extension );
		efd.setWindowTitle( tr( "Select file for project-export..." ) );
	}

	QString suffix = "wav";
	efd.setDefaultSuffix( suffix );
	efd.setAcceptMode( FileDialog::AcceptSave );

	if( efd.exec() == QDialog::Accepted && !efd.selectedFiles().isEmpty() &&
					 !efd.selectedFiles()[0].isEmpty() )
	{

		QString exportFileName = efd.selectedFiles()[0];
		if ( !multiExport )
		{
			int stx = efd.selectedNameFilter().indexOf( "(*." );
			int etx = efd.selectedNameFilter().indexOf( ")" );

			if ( stx > 0 && etx > stx )
			{
				// Get first extension from selected dropdown.
				// i.e. ".wav" from "WAV-File (*.wav), Dummy-File (*.dum)"
				suffix = efd.selectedNameFilter().mid( stx + 2, etx - stx - 2 ).split( " " )[0].trimmed();
				exportFileName.remove( "." + suffix, Qt::CaseInsensitive );
				if ( efd.selectedFiles()[0].endsWith( suffix ) )
				{
					if( VersionedSaveDialog::fileExistsQuery( exportFileName + suffix,
							tr( "Save project" ) ) )
					{
						exportFileName += suffix;
					}
				}
			}
		}

		ExportProjectDialog epd( exportFileName, gui->mainWindow(), multiExport );
		epd.exec();
	}
}


void Song::exportProjectMidi()
{
	if( isEmpty() )
	{
		QMessageBox::information( gui->mainWindow(),
				tr( "Empty project" ),
				tr( "This project is empty so exporting makes "
					"no sense. Please put some items into "
					"Song Editor first!" ) );
		return;
	}

	FileDialog efd( gui->mainWindow() );

	efd.setFileMode( FileDialog::AnyFile );

	QStringList types;
	types << tr("MIDI File (*.mid)");
	efd.setNameFilters( types );
	QString base_filename;
	if( !m_fileName.isEmpty() )
	{
		efd.setDirectory( QFileInfo( m_fileName ).absolutePath() );
		base_filename = QFileInfo( m_fileName ).completeBaseName();
	}
	else
	{
		efd.setDirectory( ConfigManager::inst()->userProjectsDir() );
		base_filename = tr( "untitled" );
	}
	efd.selectFile( base_filename + ".mid" );
	efd.setDefaultSuffix( "mid");
	efd.setWindowTitle( tr( "Select file for project-export..." ) );

	efd.setAcceptMode( FileDialog::AcceptSave );


	if( efd.exec() == QDialog::Accepted && !efd.selectedFiles().isEmpty() && !efd.selectedFiles()[0].isEmpty() )
	{
		const QString suffix = ".mid";

		QString export_filename = efd.selectedFiles()[0];
		if (!export_filename.endsWith(suffix)) export_filename += suffix;

		// NOTE start midi export

		// instantiate midi export plugin
		TrackContainer::TrackList tracks;
		tracks += Engine::getSong()->tracks();
		tracks += Engine::getBBTrackContainer()->tracks();
		ExportFilter *exf = dynamic_cast<ExportFilter *> (Plugin::instantiate("midiexport", NULL, NULL));
		if (exf==NULL) {
			qDebug() << "failed to load midi export filter!";
			return;
		}
		exf->tryExport(tracks, Engine::getSong()->getTempo(), export_filename);
	}
}



void Song::updateFramesPerTick()
{
	Engine::updateFramesPerTick();
}




void Song::setModified()
{
	if( !m_loadingProject )
	{
		m_modified = true;
		if( gui != nullptr && gui->mainWindow() &&
			QThread::currentThread() == gui->mainWindow()->thread() )
		{
			gui->mainWindow()->resetWindowTitle();
		}
	}
}




void Song::addController( Controller * controller )
{
	if( controller && !m_controllers.contains( controller ) )
	{
		m_controllers.append( controller );
		emit controllerAdded( controller );

		this->setModified();
	}
}




void Song::removeController( Controller * controller )
{
	int index = m_controllers.indexOf( controller );
	if( index != -1 )
	{
		m_controllers.remove( index );

		emit controllerRemoved( controller );
		delete controller;

		this->setModified();
	}
}




void Song::clearErrors()
{
	m_errors.clear();
}



void Song::collectError( const QString error )
{
	m_errors.append( error );
}



bool Song::hasErrors()
{
	return ( m_errors.length() > 0 );
}



QString Song::errorSummary()
{
	QString errors = m_errors.join("\n") + '\n';

	errors.prepend( "\n\n" );
	errors.prepend( tr( "The following errors occured while loading: " ) );

	return errors;
}

QString Song::songMetaData(const QString& k)
{
	return m_metaData.get(k);
}

void Song::setSongMetaData(const QString& k,const QString& v)
{
	if(m_metaData.set(k,v))
	{
		setModified();
		emit metaDataChanged(k,v);
	}
}

void Song::clearSongMetaData()
{
	if(m_metaData.clear())
	{
		setModified();
		emit metaDataChanged();
	}
}
