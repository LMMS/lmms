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

#include <algorithm>
#include <cmath>
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
#include "FxMixer.h"
#include "FxMixerView.h"
#include "GuiApplication.h"
#include "ExportFilter.h"
#include "Pattern.h"
#include "PianoRoll.h"
#include "ProjectJournal.h"
#include "ProjectNotes.h"
#include "SongEditor.h"
#include "TimeLineWidget.h"
#include "PeakController.h"


tick_t TimePos::s_ticksPerBar = DefaultTicksPerBar;



Song::Song() :
	TrackContainer(),
	m_globalAutomationTrack( dynamic_cast<AutomationTrack *>(
				Track::create( Track::HiddenAutomationTrack,
								this ) ) ),
	m_tempoModel( DefaultTempo, MinTempo, MaxTempo, this, tr( "Tempo" ) ),
	m_timeSigModel( this ),
	m_oldTicksPerBar( DefaultTicksPerBar ),
	m_masterVolumeModel( 100, 0, 200, this, tr( "Master volume" ) ),
	m_masterPitchModel( 0, -12, 12, this, tr( "Master pitch" ) ),
	m_nLoadingTrack( 0 ),
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
	m_savingProject( false ),
	m_loadingProject( false ),
	m_isCancelled( false ),
	m_playMode( Mode_None ),
	m_length( 0 ),
	m_patternToPlay( NULL ),
	m_loopPattern( false ),
	m_elapsedTicks( 0 ),
	m_elapsedBars( 0 ),
	m_loopRenderCount(1),
	m_loopRenderRemaining(1)
{
	for(int i = 0; i < Mode_Count; ++i) m_elapsedMilliSeconds[i] = 0;
	connect( &m_tempoModel, SIGNAL( dataChanged() ),
			this, SLOT( setTempo() ), Qt::DirectConnection );
	connect( &m_tempoModel, SIGNAL( dataUnchanged() ),
			this, SLOT( setTempo() ), Qt::DirectConnection );
	connect( &m_timeSigModel, SIGNAL( dataChanged() ),
			this, SLOT( setTimeSignature() ), Qt::DirectConnection );


	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ), this,
						SLOT( updateFramesPerTick() ) );

	connect( &m_masterVolumeModel, SIGNAL( dataChanged() ),
			this, SLOT( masterVolumeChanged() ), Qt::DirectConnection );
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
	TimePos::setTicksPerBar( ticksPerBar() );
	emit timeSignatureChanged( m_oldTicksPerBar, ticksPerBar() );
	emit dataChanged();
	m_oldTicksPerBar = ticksPerBar();

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
	m_vstSyncController.setPlaybackJumped(false);

	// If nothing is playing, there is nothing to do
	if (!m_playing) { return; }

	// At the beginning of the song, we have to reset the LFOs
	if (m_playMode == Mode_PlaySong && getPlayPos() == 0)
	{
		EnvelopeAndLfoParameters::instances()->reset();
	}

	TrackList trackList;
	int clipNum = -1; // The number of the clip that will be played

	// Determine the list of tracks to play and the clip number
	switch (m_playMode)
	{
		case Mode_PlaySong:
			trackList = tracks();
			break;

		case Mode_PlayBB:
			if (Engine::getBBTrackContainer()->numOfBBs() > 0)
			{
				clipNum = Engine::getBBTrackContainer()->currentBB();
				trackList.push_back(BBTrack::findBBTrack(clipNum));
			}
			break;

		case Mode_PlayPattern:
			if (m_patternToPlay)
			{
				clipNum = m_patternToPlay->getTrack()->getTCONum(m_patternToPlay);
				trackList.push_back(m_patternToPlay->getTrack());
			}
			break;

		default:
			return;
	}

	// If we have no tracks to play, there is nothing to do
	if (trackList.empty()) { return; }

	// If the playback position is outside of the range [begin, end), move it to
	// begin and inform interested parties.
	// Returns true if the playback position was moved, else false.
	const auto enforceLoop = [this](const TimePos& begin, const TimePos& end)
	{
		if (getPlayPos() < begin || getPlayPos() >= end)
		{
			setToTime(begin);
			m_vstSyncController.setPlaybackJumped(true);
			emit updateSampleTracks();
			return true;
		}
		return false;
	};

	const auto timeline = getPlayPos().m_timeLine;
	const auto loopEnabled = !m_exporting && timeline && timeline->loopPointsEnabled();

	// Ensure playback begins within the loop if it is enabled
	if (loopEnabled) { enforceLoop(timeline->loopBegin(), timeline->loopEnd()); }

	// Inform VST plugins if the user moved the play head
	if (getPlayPos().jumped())
	{
		m_vstSyncController.setPlaybackJumped(true);
		getPlayPos().setJumped(false);
	}

	const auto framesPerTick = Engine::framesPerTick();
	const auto framesPerPeriod = Engine::mixer()->framesPerPeriod();

	f_cnt_t frameOffsetInPeriod = 0;

	while (frameOffsetInPeriod < framesPerPeriod)
	{
		auto frameOffsetInTick = getPlayPos().currentFrame();

		// If a whole tick has elapsed, update the frame and tick count, and check any loops
		if (frameOffsetInTick >= framesPerTick)
		{
			// Transfer any whole ticks from the frame count to the tick count
			const auto elapsedTicks = static_cast<int>(frameOffsetInTick / framesPerTick);
			getPlayPos().setTicks(getPlayPos().getTicks() + elapsedTicks);
			frameOffsetInTick -= elapsedTicks * framesPerTick;
			getPlayPos().setCurrentFrame(frameOffsetInTick);

			// If we are playing a BB track, or a pattern with no loop enabled,
			// loop back to the beginning when we reach the end
			if (m_playMode == Mode_PlayBB)
			{
				enforceLoop(TimePos{0}, TimePos{Engine::getBBTrackContainer()->lengthOfCurrentBB(), 0});
			}
			else if (m_playMode == Mode_PlayPattern && m_loopPattern && !loopEnabled)
			{
				enforceLoop(TimePos{0}, m_patternToPlay->length());
			}

			// Handle loop points, and inform VST plugins of the loop status
			if (loopEnabled || m_loopRenderRemaining > 1)
			{
				m_vstSyncController.startCycle(
					timeline->loopBegin().getTicks(), timeline->loopEnd().getTicks());

				// Loop if necessary, and decrement the remaining loops if we did
				if (enforceLoop(timeline->loopBegin(), timeline->loopEnd())
					&& m_loopRenderRemaining > 1)
				{
					m_loopRenderRemaining--;
				}
			}
			else
			{
				m_vstSyncController.stopCycle();
			}
		}

		const f_cnt_t framesUntilNextPeriod = framesPerPeriod - frameOffsetInPeriod;
		const f_cnt_t framesUntilNextTick = static_cast<f_cnt_t>(std::ceil(framesPerTick - frameOffsetInTick));

		// We want to proceed to the next buffer or tick, whichever is closer
		const auto framesToPlay = std::min(framesUntilNextPeriod, framesUntilNextTick);

		if (frameOffsetInPeriod == 0)
		{
			// First frame of buffer: update VST sync position.
			// This must be done after we've corrected the frame/tick count,
			// but before actually playing any frames.
			m_vstSyncController.setAbsolutePosition(getPlayPos().getTicks()
				+ getPlayPos().currentFrame() / static_cast<double>(framesPerTick));
			m_vstSyncController.update();
		}

		if (static_cast<f_cnt_t>(frameOffsetInTick) == 0)
		{
			// First frame of tick: process automation and play tracks
			processAutomations(trackList, getPlayPos(), framesToPlay);
			for (const auto track : trackList)
			{
				track->play(getPlayPos(), framesToPlay, frameOffsetInPeriod, clipNum);
			}
		}

		// Update frame counters
		frameOffsetInPeriod += framesToPlay;
		frameOffsetInTick += framesToPlay;
		getPlayPos().setCurrentFrame(frameOffsetInTick);
		m_elapsedMilliSeconds[m_playMode] += TimePos::ticksToMilliseconds(framesToPlay / framesPerTick, getTempo());
		m_elapsedBars = m_playPos[Mode_PlaySong].getBar();
		m_elapsedTicks = (m_playPos[Mode_PlaySong].getTicks() % ticksPerBar()) / 48;
	}
}


void Song::processAutomations(const TrackList &tracklist, TimePos timeStart, fpp_t)
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
		TimePos relTime = timeStart - p->startPosition();
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

void Song::setModified(bool value)
{
	if( !m_loadingProject && m_modified != value)
	{
		m_modified = value;
		emit modified();
	}
}

bool Song::isExportDone() const
{
	return !isExporting() || m_playPos[m_playMode] >= m_exportSongEnd;
}

int Song::getExportProgress() const
{
	TimePos pos = m_playPos[m_playMode];
    
	if (pos >= m_exportSongEnd)
	{
		return 100;
	}
	else if (pos <= m_exportSongBegin)
	{
		return 0;
	}
	else if (pos >= m_exportLoopEnd)
	{
		pos = (m_exportLoopBegin-m_exportSongBegin) + (m_exportLoopEnd - m_exportLoopBegin) *
			m_loopRenderCount + (pos - m_exportLoopEnd);
	}
	else if ( pos >= m_exportLoopBegin )
	{
		pos = (m_exportLoopBegin-m_exportSongBegin) + ((m_exportLoopEnd - m_exportLoopBegin) *
			(m_loopRenderCount - m_loopRenderRemaining)) + (pos - m_exportLoopBegin);
	}
	else
	{
		pos = (pos - m_exportSongBegin);
	}

	return (float)pos/(float)m_exportEffectiveLength*100.0f;
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
	for (auto track : tracks())
	{
		if (m_exporting && track->isMuted())
		{
			continue;
		}

		const bar_t cur = track->length();
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
	m_elapsedMilliSeconds[playMode] += TimePos::ticksToMilliseconds( ticks - ticksFromPlayMode, getTempo() );
	m_playPos[playMode].setTicks( ticks );
	m_playPos[playMode].setCurrentFrame( 0.0f );
	m_playPos[playMode].setJumped( true );

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

	if( tl )
	{
		switch( tl->behaviourAtStop() )
		{
			case TimeLineWidget::BackToZero:
				m_playPos[m_playMode].setTicks(0);
				m_elapsedMilliSeconds[m_playMode] = 0;
				break;

			case TimeLineWidget::BackToStart:
				if( tl->savedPos() >= 0 )
				{
					m_playPos[m_playMode].setTicks(tl->savedPos().getTicks());
					setToTime(tl->savedPos());

					tl->savePos( -1 );
				}
				break;

			case TimeLineWidget::KeepStopPosition:
				break;
		}
	}
	else
	{
		m_playPos[m_playMode].setTicks( 0 );
		m_elapsedMilliSeconds[m_playMode] = 0;
	}
	m_playing = false;

	m_elapsedMilliSeconds[Mode_None] = m_elapsedMilliSeconds[m_playMode];
	m_playPos[Mode_None].setTicks(m_playPos[m_playMode].getTicks());

	m_playPos[m_playMode].setCurrentFrame( 0 );

	m_vstSyncController.setPlaybackState( m_exporting );
	m_vstSyncController.setAbsolutePosition(
		m_playPos[m_playMode].getTicks()
		+ m_playPos[m_playMode].currentFrame()
		/ (double) Engine::framesPerTick() );

	// remove all note-play-handles that are active
	Engine::mixer()->clear();

	m_playMode = Mode_None;

	emit stopped();
	emit playbackStateChanged();
}




void Song::startExport()
{
	stop();

	m_exporting = true;
	updateLength();

	if (m_renderBetweenMarkers)
	{
		m_exportSongBegin = m_exportLoopBegin = m_playPos[Mode_PlaySong].m_timeLine->loopBegin();
		m_exportSongEnd = m_exportLoopEnd = m_playPos[Mode_PlaySong].m_timeLine->loopEnd();

		m_playPos[Mode_PlaySong].setTicks( m_playPos[Mode_PlaySong].m_timeLine->loopBegin().getTicks() );
	}
	else
	{
		m_exportSongEnd = TimePos(m_length, 0);
        
		// Handle potentially ridiculous loop points gracefully.
		if (m_loopRenderCount > 1 && m_playPos[Mode_PlaySong].m_timeLine->loopEnd() > m_exportSongEnd) 
		{
			m_exportSongEnd = m_playPos[Mode_PlaySong].m_timeLine->loopEnd();
		}

		if (!m_exportLoop) 
			m_exportSongEnd += TimePos(1,0);
        
		m_exportSongBegin = TimePos(0,0);
		// FIXME: remove this check once we load timeline in headless mode
		if (m_playPos[Mode_PlaySong].m_timeLine)
		{
			m_exportLoopBegin = m_playPos[Mode_PlaySong].m_timeLine->loopBegin() < m_exportSongEnd &&
				m_playPos[Mode_PlaySong].m_timeLine->loopEnd() <= m_exportSongEnd ?
				m_playPos[Mode_PlaySong].m_timeLine->loopBegin() : TimePos(0,0);
			m_exportLoopEnd = m_playPos[Mode_PlaySong].m_timeLine->loopBegin() < m_exportSongEnd &&
				m_playPos[Mode_PlaySong].m_timeLine->loopEnd() <= m_exportSongEnd ?
				m_playPos[Mode_PlaySong].m_timeLine->loopEnd() : TimePos(0,0);
		}

		m_playPos[Mode_PlaySong].setTicks( 0 );
	}

	m_exportEffectiveLength = (m_exportLoopBegin - m_exportSongBegin) + (m_exportLoopEnd - m_exportLoopBegin) 
		* m_loopRenderCount + (m_exportSongEnd - m_exportLoopEnd);
	m_loopRenderRemaining = m_loopRenderCount;

	playSong();

	m_vstSyncController.setPlaybackState( true );
}




void Song::stopExport()
{
	stop();
	m_exporting = false;

	m_vstSyncController.setPlaybackState( m_playing );
}




void Song::insertBar()
{
	m_tracksMutex.lockForRead();
	for( TrackList::const_iterator it = tracks().begin();
					it != tracks().end(); ++it )
	{
		( *it )->insertBar( m_playPos[Mode_PlaySong] );
	}
	m_tracksMutex.unlock();
}




void Song::removeBar()
{
	m_tracksMutex.lockForRead();
	for( TrackList::const_iterator it = tracks().begin();
					it != tracks().end(); ++it )
	{
		( *it )->removeBar( m_playPos[Mode_PlaySong] );
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


AutomatedValueMap Song::automatedValuesAt(TimePos time, int tcoNum) const
{
	return TrackContainer::automatedValuesFromTracks(TrackList{m_globalAutomationTrack} << tracks(), time, tcoNum);
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

	m_oldFileName = "";
	setProjectFileName("");

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

	setModified(false);
	m_loadOnLaunch = false;
}




void Song::createNewProjectFromTemplate( const QString & templ )
{
	loadProject( templ );
	// clear file-name so that user doesn't overwrite template when
	// saving...
	m_oldFileName = "";
	setProjectFileName("");
	// update window title
	m_loadOnLaunch = false;
}




// load given song
void Song::loadProject( const QString & fileName )
{
	QDomNode node;

	m_loadingProject = true;

	Engine::projectJournal()->setJournalling( false );

	m_oldFileName = m_fileName;
	setProjectFileName(fileName);

	DataFile dataFile( m_fileName );
	// if file could not be opened, head-node is null and we create
	// new project
	if( dataFile.head().isNull() )
	{
		if( m_loadOnLaunch )
		{
			createNewProject();
		}
		setProjectFileName(m_oldFileName);
		return;
	}

	m_oldFileName = m_fileName;

	clearProject();

	clearErrors();

	Engine::mixer()->requestChangeInModel();

	// get the header information from the DOM
	m_tempoModel.loadSettings( dataFile.head(), "bpm" );
	m_timeSigModel.loadSettings( dataFile.head(), "timesig" );
	m_masterVolumeModel.loadSettings( dataFile.head(), "mastervol" );
	m_masterPitchModel.loadSettings( dataFile.head(), "masterpitch" );

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

	while( !node.isNull() && !isCancelled() )
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

	// Remove dummy controllers that was added for correct connections
	m_controllers.erase(std::remove_if(m_controllers.begin(), m_controllers.end(),
		[](Controller* c){return c->type() == Controller::DummyController;}),
		m_controllers.end());

	// resolve all IDs so that autoModels are automated
	AutomationPattern::resolveAllIDs();


	Engine::mixer()->doneChangeInModel();

	ConfigManager::inst()->addRecentlyOpenedProject( fileName );

	Engine::projectJournal()->setJournalling( true );

	emit projectLoaded();

	if( isCancelled() )
	{
		m_isCancelled = false;
		createNewProject();
		return;
	}

	if ( hasErrors())
	{
		if ( gui )
		{
			QMessageBox::warning( NULL, tr("LMMS Error report"), errorSummary(),
							QMessageBox::Ok );
		}
		else
		{
#if (QT_VERSION >= QT_VERSION_CHECK(5,15,0))
			QTextStream(stderr) << Engine::getSong()->errorSummary() << Qt::endl;
#else
			QTextStream(stderr) << Engine::getSong()->errorSummary() << endl;
#endif
		}
	}

	m_loadingProject = false;
	setModified(false);
	m_loadOnLaunch = false;
}


// only save current song as _filename and do nothing else
bool Song::saveProjectFile( const QString & filename )
{
	DataFile dataFile( DataFile::SongProject );
	m_savingProject = true;

	m_tempoModel.saveSettings( dataFile, dataFile.head(), "bpm" );
	m_timeSigModel.saveSettings( dataFile, dataFile.head(), "timesig" );
	m_masterVolumeModel.saveSettings( dataFile, dataFile.head(), "mastervol" );
	m_masterPitchModel.saveSettings( dataFile, dataFile.head(), "masterpitch" );

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

	m_savingProject = false;

	return dataFile.writeFile( filename );
}



// Save the current song
bool Song::guiSaveProject()
{
	DataFile dataFile( DataFile::SongProject );
	QString fileNameWithExtension = dataFile.nameWithExtension( m_fileName );
	setProjectFileName(fileNameWithExtension);

	bool const saveResult = saveProjectFile( m_fileName );

	if( saveResult )
	{
		setModified(false);
	}

	return saveResult;
}




// Save the current song with the given filename
bool Song::guiSaveProjectAs( const QString & _file_name )
{
	QString o = m_oldFileName;
	m_oldFileName = m_fileName;
	setProjectFileName(_file_name);

	bool saveResult = guiSaveProject();
	// After saving as, restore default save options.
	m_saveOptions.setDefaultOptions();

	if(!saveResult)
	{
		// Saving failed. Restore old filenames.
		setProjectFileName(m_oldFileName);
		m_oldFileName = o;

		return false;
	}

	m_oldFileName = m_fileName;

	return true;
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
	while( !node.isNull() && !isCancelled() )
	{
		Controller * c = Controller::create( node.toElement(), this );
		if (c) {addController(c);}
		else
		{
			// Fix indices to ensure correct connections
			m_controllers.append(Controller::create(
				Controller::DummyController, this));
		}

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



void Song::exportProjectMidi(QString const & exportFileName) const
{
	// instantiate midi export plugin
	TrackContainer::TrackList const & tracks = this->tracks();
	TrackContainer::TrackList const & tracks_BB = Engine::getBBTrackContainer()->tracks();

	ExportFilter *exf = dynamic_cast<ExportFilter *> (Plugin::instantiate("midiexport", nullptr, nullptr));
	if (exf)
	{
		exf->tryExport(tracks, tracks_BB, getTempo(), m_masterPitchModel.value(), exportFileName);
	}
	else
	{
		qDebug() << "failed to load midi export filter!";
	}

}



void Song::updateFramesPerTick()
{
	Engine::updateFramesPerTick();
}




void Song::setModified()
{
	setModified(true);
}

void Song::setProjectFileName(QString const & projectFileName)
{
	if (m_fileName != projectFileName)
	{
		m_fileName = projectFileName;
		emit projectFileNameChanged();
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
	if (!m_errors.contains(error)) { m_errors[error] = 1; }
	else { m_errors[ error ]++; }
}



bool Song::hasErrors()
{
	return !(m_errors.isEmpty());
}



QString Song::errorSummary()
{
	QString errors;

	auto i = m_errors.constBegin();
	while (i != m_errors.constEnd())
	{
		errors.append( i.key() );
		if( i.value() > 1 )
		{
			errors.append( tr(" (repeated %1 times)").arg( i.value() ) );
		}
		errors.append("\n");
		++i;
	}

	errors.prepend( "\n\n" );
	errors.prepend( tr( "The following errors occurred while loading: " ) );

	return errors;
}

bool Song::isSavingProject() const {
	return m_savingProject;
}
