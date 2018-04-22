/*
 * Mixer.cpp - audio-device-independent mixer for LMMS
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

#include "Mixer.h"

#include "denormals.h"

#include "lmmsconfig.h"

#include "AudioPort.h"
#include "FxMixer.h"
#include "MixerWorkerThread.h"
#include "Song.h"
#include "EnvelopeAndLfoParameters.h"
#include "NotePlayHandle.h"
#include "ConfigManager.h"
#include "SamplePlayHandle.h"
#include "Memory.h"
#include "BufferPool.h"
#include "LocklessList.h"

// platform-specific audio-interface-classes
#include "AudioAlsa.h"
#include "AudioJack.h"
#include "AudioOss.h"
#include "AudioSndio.h"
#include "AudioPortAudio.h"
#include "AudioSoundIo.h"
#include "AudioPulseAudio.h"
#include "AudioSdl.h"
#include "AudioDummy.h"

// platform-specific midi-interface-classes
#include "MidiAlsaRaw.h"
#include "MidiAlsaSeq.h"
#include "MidiJack.h"
#include "MidiOss.h"
#include "MidiSndio.h"
#include "MidiWinMM.h"
#include "MidiApple.h"
#include "MidiDummy.h"


static thread_local bool s_renderingThread;




Mixer::Mixer( bool renderOnly ) :
	m_renderOnly( renderOnly ),
	m_framesPerPeriod( DEFAULT_BUFFER_SIZE ),
	m_inputBufferRead( 0 ),
	m_inputBufferWrite( 1 ),
	m_readBuf( NULL ),
	m_writeBuf( NULL ),
	m_workers(),
	m_numWorkers( QThread::idealThreadCount()-1 ),
	m_newPlayHandles( new LocklessStack<PlayHandle*>(PlayHandle::MaxNumber) ),
	m_qualitySettings( qualitySettings::Mode_Draft ),
	m_masterGain( 1.0f ),
	m_isProcessing( false ),
	m_audioDev( NULL ),
	m_oldAudioDev( NULL ),
	m_audioDevStartFailed( false ),
	m_profiler(),
	m_metronomeActive(false),
	m_clearSignal( false ),
	m_changesSignal( false ),
	m_changes( 0 ),
	m_doChangesMutex( QMutex::Recursive ),
	m_waitingForWrite( false )
{
	for( int i = 0; i < 2; ++i )
	{
		m_inputBufferFrames[i] = 0;
		m_inputBufferSize[i] = DEFAULT_BUFFER_SIZE * 100;
		m_inputBuffer[i] = new sampleFrame[ DEFAULT_BUFFER_SIZE * 100 ];
		BufferPool::clear( m_inputBuffer[i], m_inputBufferSize[i] );
	}

	// determine FIFO size and number of frames per period
	int fifoSize = 1;

	// if not only rendering (that is, using the GUI), load the buffer
	// size from user configuration
	if( renderOnly == false )
	{
		m_framesPerPeriod = 
			( fpp_t ) ConfigManager::inst()->
				value( "mixer", "framesperaudiobuffer" ).toInt();

		// if the value read from user configuration is not set or
		// lower than the minimum allowed, use the default value and
		// save it to the configuration
		if( m_framesPerPeriod < MINIMUM_BUFFER_SIZE )
		{
			ConfigManager::inst()->setValue( "mixer",
						"framesperaudiobuffer",
						QString::number( DEFAULT_BUFFER_SIZE ) );

			m_framesPerPeriod = DEFAULT_BUFFER_SIZE;
		}
		else if( m_framesPerPeriod > DEFAULT_BUFFER_SIZE )
		{
			fifoSize = m_framesPerPeriod / DEFAULT_BUFFER_SIZE;
			m_framesPerPeriod = DEFAULT_BUFFER_SIZE;
		}
	}

	// allocte the FIFO from the determined size
	m_fifo = new fifo( fifoSize );

	// now that framesPerPeriod is fixed initialize global BufferPool
	BufferPool::init( m_framesPerPeriod );

	AlignedAllocator<surroundSampleFrame> alloc;
	for( int i = 0; i < 3; i++ )
	{
		m_readBuf = alloc.allocate( m_framesPerPeriod );

		BufferPool::clear( m_readBuf, m_framesPerPeriod );
		m_bufferPool.push_back( m_readBuf );
	}

	for( int i = 0; i < m_numWorkers+1; ++i )
	{
		MixerWorkerThread * wt = new MixerWorkerThread( this );
		if( i < m_numWorkers )
		{
			wt->start( QThread::TimeCriticalPriority );
		}
		m_workers.push_back( wt );
	}

	m_poolDepth = 2;
	m_readBuffer = 0;
	m_writeBuffer = 1;
}




Mixer::~Mixer()
{
	runChangesInModel();

	for( int w = 0; w < m_numWorkers; ++w )
	{
		m_workers[w]->quit();
	}

	MixerWorkerThread::startAndWaitForJobs();

	for( int w = 0; w < m_numWorkers; ++w )
	{
		m_workers[w]->wait( 500 );
	}

	while( m_fifo->available() )
	{
		delete[] m_fifo->read();
	}
	delete m_fifo;

	delete m_audioDev;
	delete m_midiClient;

	AlignedAllocator<surroundSampleFrame> alloc;
	for( int i = 0; i < 3; i++ )
	{
		alloc.deallocate( m_bufferPool[i], m_framesPerPeriod );
	}

	for( int i = 0; i < 2; ++i )
	{
		delete[] m_inputBuffer[i];
	}
}




void Mixer::initDevices()
{
	bool success_ful = false;
	if( m_renderOnly ) {
		m_audioDev = new AudioDummy( success_ful, this );
		m_audioDevName = AudioDummy::name();
		m_midiClient = new MidiDummy;
		m_midiClientName = MidiDummy::name();
	} else {
		m_audioDev = tryAudioDevices();
		m_midiClient = tryMidiClients();
	}
}




void Mixer::startProcessing( bool _needs_fifo )
{
	if( _needs_fifo )
	{
		m_fifoWriter = new fifoWriter( this, m_fifo );
		m_fifoWriter->start( QThread::HighPriority );
	}
	else
	{
		m_fifoWriter = NULL;
	}

	m_audioDev->startProcessing();

	m_isProcessing = true;
}




void Mixer::stopProcessing()
{
	m_isProcessing = false;

	if( m_fifoWriter != NULL )
	{
		m_fifoWriter->finish();
		m_fifoWriter->wait();
		m_audioDev->stopProcessing();
		delete m_fifoWriter;
		m_fifoWriter = NULL;
	}
	else
	{
		m_audioDev->stopProcessing();
	}
}




sample_rate_t Mixer::baseSampleRate() const
{
	sample_rate_t sr =
		ConfigManager::inst()->value( "mixer", "samplerate" ).toInt();
	if( sr < 44100 )
	{
		sr = 44100;
	}
	return sr;
}




sample_rate_t Mixer::outputSampleRate() const
{
	return m_audioDev != NULL ? m_audioDev->sampleRate() :
							baseSampleRate();
}




sample_rate_t Mixer::inputSampleRate() const
{
	return m_audioDev != NULL ? m_audioDev->sampleRate() :
							baseSampleRate();
}




sample_rate_t Mixer::processingSampleRate() const
{
	return outputSampleRate() * m_qualitySettings.sampleRateMultiplier();
}




bool Mixer::criticalXRuns() const
{
	return cpuLoad() >= 99 && Engine::getSong()->isExporting() == false;
}




void Mixer::pushInputFrames( sampleFrame * _ab, const f_cnt_t _frames )
{
	requestChangeInModel();

	f_cnt_t frames = m_inputBufferFrames[ m_inputBufferWrite ];
	int size = m_inputBufferSize[ m_inputBufferWrite ];
	sampleFrame * buf = m_inputBuffer[ m_inputBufferWrite ];

	if( frames + _frames > size )
	{
		size = qMax( size * 2, frames + _frames );
		sampleFrame * ab = new sampleFrame[ size ];
		memcpy( ab, buf, frames * sizeof( sampleFrame ) );
		delete [] buf;

		m_inputBufferSize[ m_inputBufferWrite ] = size;
		m_inputBuffer[ m_inputBufferWrite ] = ab;

		buf = ab;
	}

	memcpy( &buf[ frames ], _ab, _frames * sizeof( sampleFrame ) );
	m_inputBufferFrames[ m_inputBufferWrite ] += _frames;

	doneChangeInModel();
}




const surroundSampleFrame * Mixer::renderNextBuffer()
{
	m_profiler.startPeriod();

	s_renderingThread = true;

	static Song::PlayPos last_metro_pos = -1;

	Song *song = Engine::getSong();

	Song::PlayModes currentPlayMode = song->playMode();
	Song::PlayPos p = song->getPlayPos( currentPlayMode );

	bool playModeSupportsMetronome = currentPlayMode == Song::Mode_PlayPattern ||
					 currentPlayMode == Song::Mode_PlaySong ||
					 currentPlayMode == Song::Mode_PlayBB;

	if( playModeSupportsMetronome && m_metronomeActive && !song->isExporting() &&
		p != last_metro_pos &&
			// Stop crash with metronome if empty project
				Engine::getSong()->countTracks() )
	{
		tick_t ticksPerTact = MidiTime::ticksPerTact();
		if ( p.getTicks() % (ticksPerTact / 1 ) == 0 )
		{
			addPlayHandle( new SamplePlayHandle( "misc/metronome02.ogg" ) );
		}
		else if ( p.getTicks() % (ticksPerTact /
			song->getTimeSigModel().getNumerator() ) == 0 )
		{
			addPlayHandle( new SamplePlayHandle( "misc/metronome01.ogg" ) );
		}
		last_metro_pos = p;
	}

	// swap buffer
	m_inputBufferWrite = ( m_inputBufferWrite + 1 ) % 2;
	m_inputBufferRead =  ( m_inputBufferRead + 1 ) % 2;

	// clear new write buffer
	m_inputBufferFrames[ m_inputBufferWrite ] = 0;

	if( m_clearSignal )
	{
		m_clearSignal = false;
		clearInternal();
	}

	// remove all play-handles that have to be deleted and delete
	// them if they still exist...
	// maybe this algorithm could be optimized...
	ConstPlayHandleList::Iterator it_rem = m_playHandlesToRemove.begin();
	while( it_rem != m_playHandlesToRemove.end() )
	{
		PlayHandleList::Iterator it = qFind( m_playHandles.begin(), m_playHandles.end(), *it_rem );

		if( it != m_playHandles.end() )
		{
			( *it )->audioPort()->removePlayHandle( ( *it ) );
			if( ( *it )->type() == PlayHandle::TypeNotePlayHandle )
			{
				NotePlayHandlePool.destroy( (NotePlayHandle*) *it );
			}
			else delete *it;
			m_playHandles.erase( it );
		}

		it_rem = m_playHandlesToRemove.erase( it_rem );
	}

	// rotate buffers
	m_writeBuffer = ( m_writeBuffer + 1 ) % m_poolDepth;
	m_readBuffer = ( m_readBuffer + 1 ) % m_poolDepth;

	m_writeBuf = m_bufferPool[m_writeBuffer];
	m_readBuf = m_bufferPool[m_readBuffer];

	// clear last audio-buffer
	BufferPool::clear( m_writeBuf, m_framesPerPeriod );

	// prepare master mix (clear internal buffers etc.)
	FxMixer * fxMixer = Engine::fxMixer();
	fxMixer->prepareMasterMix();

	// create play-handles for new notes, samples etc.
	song->processNextBuffer();

	// add all play-handles that have to be added
	PlayHandle* ph;
	while ( m_newPlayHandles->pop(ph) )
	{
		m_playHandles += ph;
	}

	// STAGE 1: run and render all play handles
	MixerWorkerThread::fillJobQueue<PlayHandleList>( m_playHandles );
	MixerWorkerThread::startAndWaitForJobs();

	// removed all play handles which are done
	for( PlayHandleList::Iterator it = m_playHandles.begin();
						it != m_playHandles.end(); )
	{
		if( ( *it )->affinityMatters() &&
			( *it )->affinity() != QThread::currentThread() )
		{
			++it;
			continue;
		}
		if( ( *it )->isFinished() )
		{
			( *it )->audioPort()->removePlayHandle( ( *it ) );
			if( ( *it )->type() == PlayHandle::TypeNotePlayHandle )
			{
				NotePlayHandlePool.destroy( (NotePlayHandle*) *it );
			}
			else delete *it;
			it = m_playHandles.erase( it );
		}
		else
		{
			++it;
		}
	}

	// STAGE 2: process effects of all instrument- and sampletracks
	MixerWorkerThread::fillJobQueue<QVector<AudioPort *> >( m_audioPorts );
	MixerWorkerThread::startAndWaitForJobs();


	// STAGE 3: do master mix in FX mixer
	fxMixer->masterMix( m_writeBuf );


	emit nextAudioBuffer( m_readBuf );

	runChangesInModel();

	// and trigger LFOs
	EnvelopeAndLfoParameters::instances()->trigger();
	Controller::triggerFrameCounter();
	AutomatableModel::incrementPeriodCounter();

	s_renderingThread = false;

	m_profiler.finishPeriod( processingSampleRate(), m_framesPerPeriod );

	return m_readBuf;
}




void Mixer::clear()
{
	m_clearSignal = true;
}




void Mixer::clearNewPlayHandles()
{
	requestChangeInModel();
	PlayHandle* ph;
	while ( m_newPlayHandles->pop(ph) ) {}
	doneChangeInModel();
}



// removes all play-handles. this is necessary, when the song is stopped ->
// all remaining notes etc. would be played until their end
void Mixer::clearInternal()
{
	// TODO: m_midiClient->noteOffAll();
	for( PlayHandleList::Iterator it = m_playHandles.begin(); it != m_playHandles.end(); ++it )
	{
		// we must not delete instrument-play-handles as they exist
		// during the whole lifetime of an instrument
		if( ( *it )->type() != PlayHandle::TypeInstrumentPlayHandle )
		{
			m_playHandlesToRemove.push_back( *it );
		}
	}
}




void Mixer::getPeakValues( sampleFrame * _ab, const f_cnt_t _frames, float & peakLeft, float & peakRight ) const
{
	peakLeft = 0.0f;
	peakRight = 0.0f;

	for( f_cnt_t f = 0; f < _frames; ++f )
	{
		float const absLeft = qAbs( _ab[f][0] );
		float const absRight = qAbs( _ab[f][1] );
		if (absLeft > peakLeft)
		{
			peakLeft = absLeft;
		}

		if (absRight > peakRight)
		{
			peakRight = absRight;
		}
	}
}




void Mixer::changeQuality( const struct qualitySettings & _qs )
{
	// don't delete the audio-device
	stopProcessing();

	m_qualitySettings = _qs;
	m_audioDev->applyQualitySettings();

	emit sampleRateChanged();
	emit qualitySettingsChanged();

	startProcessing();
}




void Mixer::setAudioDevice( AudioDevice * _dev )
{
	stopProcessing();

	if( _dev == NULL )
	{
		printf( "param _dev == NULL in Mixer::setAudioDevice(...). "
					"Trying any working audio-device\n" );
		m_audioDev = tryAudioDevices();
	}
	else
	{
		m_audioDev = _dev;
	}

	emit sampleRateChanged();

	startProcessing();
}




void Mixer::setAudioDevice( AudioDevice * _dev,
				const struct qualitySettings & _qs,
				bool _needs_fifo )
{
	// don't delete the audio-device
	stopProcessing();

	m_qualitySettings = _qs;

	if( _dev == NULL )
	{
		printf( "param _dev == NULL in Mixer::setAudioDevice(...). "
					"Trying any working audio-device\n" );
		m_audioDev = tryAudioDevices();
	}
	else
	{
		m_audioDev = _dev;
	}

	emit qualitySettingsChanged();
	emit sampleRateChanged();

	startProcessing( _needs_fifo );
}




void Mixer::storeAudioDevice()
{
	if( !m_oldAudioDev )
	{
		m_oldAudioDev = m_audioDev;
	}
}




void Mixer::restoreAudioDevice()
{
	if( m_oldAudioDev != NULL )
	{
		stopProcessing();
		delete m_audioDev;

		m_audioDev = m_oldAudioDev;
		emit sampleRateChanged();

		m_oldAudioDev = NULL;
		startProcessing();
	}
}




void Mixer::removeAudioPort( AudioPort * _port )
{
	requestChangeInModel();
	QVector<AudioPort *>::Iterator it = qFind( m_audioPorts.begin(),
							m_audioPorts.end(),
							_port );
	if( it != m_audioPorts.end() )
	{
		m_audioPorts.erase( it );
	}
	doneChangeInModel();
}


bool Mixer::addPlayHandle( PlayHandle* handle )
{
	if( criticalXRuns() == false )
	{
		m_newPlayHandles->push( handle );
		handle->audioPort()->addPlayHandle( handle );
		return true;
	}

	if( handle->type() == PlayHandle::TypeNotePlayHandle )
	{
		NotePlayHandlePool.destroy( (NotePlayHandle*)handle );
	}
	else delete handle;

	return false;
}


void Mixer::removePlayHandle( PlayHandle * _ph )
{
	requestChangeInModel();
	// check thread affinity as we must not delete play-handles
	// which were created in a thread different than mixer thread
	if( _ph->affinityMatters() &&
				_ph->affinity() == QThread::currentThread() )
	{
		_ph->audioPort()->removePlayHandle( _ph );
		bool removedFromList = false;
		// Check m_newPlayHandles first because doing it the other way around
		// creates a race condition
		m_newPlayHandles->apply([_ph](decltype(m_newPlayHandles)::element_type::Container container) {
			for (auto it=container.begin(); it != container.end(); it++) {
				if (*it == _ph) {
					container.erase(it);
					break;
				}
			}
		});
		// Now check m_playHandles
		PlayHandleList::Iterator it = qFind( m_playHandles.begin(),
					m_playHandles.end(), _ph );
		if( it != m_playHandles.end() )
		{
			m_playHandles.erase( it );
			removedFromList = true;
		}
		// Only deleting PlayHandles that were actually found in the list
		// "fixes crash when previewing a preset under high load"
		// (See tobydox's 2008 commit 4583e48)
		if ( removedFromList )
		{
			if( _ph->type() == PlayHandle::TypeNotePlayHandle )
			{
				NotePlayHandlePool.destroy( (NotePlayHandle*) _ph );
			}
			else delete _ph;
		}
	}
	else
	{
		m_playHandlesToRemove.push_back( _ph );
	}
	doneChangeInModel();
}




void Mixer::removePlayHandlesOfTypes( Track * _track, const quint8 types )
{
	requestChangeInModel();
	PlayHandleList::Iterator it = m_playHandles.begin();
	while( it != m_playHandles.end() )
	{
		if( ( *it )->isFromTrack( _track ) && ( ( *it )->type() & types ) )
		{
			( *it )->audioPort()->removePlayHandle( ( *it ) );
			if( ( *it )->type() == PlayHandle::TypeNotePlayHandle )
			{
				NotePlayHandlePool.destroy( (NotePlayHandle*) *it );
			}
			else delete *it;
			it = m_playHandles.erase( it );
		}
		else
		{
			++it;
		}
	}
	doneChangeInModel();
}




void Mixer::requestChangeInModel()
{
	if( s_renderingThread )
		return;

	m_changesMutex.lock();
	m_changes++;
	m_changesMutex.unlock();

	m_doChangesMutex.lock();
	m_waitChangesMutex.lock();
	if ( m_isProcessing && !m_waitingForWrite && !m_changesSignal )
	{
		m_changesSignal = true;
		m_changesRequestCondition.wait( &m_waitChangesMutex );
	}
	m_waitChangesMutex.unlock();
}




void Mixer::doneChangeInModel()
{
	if( s_renderingThread )
		return;

	m_changesMutex.lock();
	bool moreChanges = --m_changes;
	m_changesMutex.unlock();

	if( !moreChanges )
	{
		m_changesSignal = false;
		m_changesMixerCondition.wakeOne();
	}
	m_doChangesMutex.unlock();
}




void Mixer::runChangesInModel()
{
	if( m_changesSignal )
	{
		m_waitChangesMutex.lock();
		m_changesRequestCondition.wakeOne();
		m_changesMixerCondition.wait( &m_waitChangesMutex );
		m_waitChangesMutex.unlock();
	}
}




AudioDevice * Mixer::tryAudioDevices()
{
	bool success_ful = false;
	AudioDevice * dev = NULL;
	QString dev_name = ConfigManager::inst()->value( "mixer", "audiodev" );

	m_audioDevStartFailed = false;

#ifdef LMMS_HAVE_SDL
	if( dev_name == AudioSdl::name() || dev_name == "" )
	{
		dev = new AudioSdl( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSdl::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_ALSA
	if( dev_name == AudioAlsa::name() || dev_name == "" )
	{
		dev = new AudioAlsa( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioAlsa::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PULSEAUDIO
	if( dev_name == AudioPulseAudio::name() || dev_name == "" )
	{
		dev = new AudioPulseAudio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioPulseAudio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_OSS
	if( dev_name == AudioOss::name() || dev_name == "" )
	{
		dev = new AudioOss( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioOss::name();
			return dev;
		}
		delete dev;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if( dev_name == AudioSndio::name() || dev_name == "" )
	{
		dev = new AudioSndio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSndio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_JACK
	if( dev_name == AudioJack::name() || dev_name == "" )
	{
		dev = new AudioJack( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioJack::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PORTAUDIO
	if( dev_name == AudioPortAudio::name() || dev_name == "" )
	{
		dev = new AudioPortAudio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioPortAudio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_SOUNDIO
	if( dev_name == AudioSoundIo::name() || dev_name == "" )
	{
		dev = new AudioSoundIo( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSoundIo::name();
			return dev;
		}
		delete dev;
	}
#endif


	// add more device-classes here...
	//dev = new audioXXXX( SAMPLE_RATES[m_qualityLevel], success_ful, this );
	//if( sucess_ful )
	//{
	//	return dev;
	//}
	//delete dev

	if( dev_name != AudioDummy::name() )
	{
		printf( "No audio-driver working - falling back to dummy-audio-"
			"driver\nYou can render your songs and listen to the output "
			"files...\n" );

		m_audioDevStartFailed = true;
	}

	m_audioDevName = AudioDummy::name();

	return new AudioDummy( success_ful, this );
}




MidiClient * Mixer::tryMidiClients()
{
	QString client_name = ConfigManager::inst()->value( "mixer",
								"mididev" );

#ifdef LMMS_HAVE_ALSA
	if( client_name == MidiAlsaSeq::name() || client_name == "" )
	{
		MidiAlsaSeq * malsas = new MidiAlsaSeq;
		if( malsas->isRunning() )
		{
			m_midiClientName = MidiAlsaSeq::name();
			return malsas;
		}
		delete malsas;
	}

	if( client_name == MidiAlsaRaw::name() || client_name == "" )
	{
		MidiAlsaRaw * malsar = new MidiAlsaRaw;
		if( malsar->isRunning() )
		{
			m_midiClientName = MidiAlsaRaw::name();
			return malsar;
		}
		delete malsar;
	}
#endif

#ifdef LMMS_HAVE_JACK
	if( client_name == MidiJack::name() || client_name == "" )
	{
		MidiJack * mjack = new MidiJack;
		if( mjack->isRunning() )
		{
			m_midiClientName = MidiJack::name();
			return mjack;
		}
		delete mjack;
	}
#endif

#ifdef LMMS_HAVE_OSS
	if( client_name == MidiOss::name() || client_name == "" )
	{
		MidiOss * moss = new MidiOss;
		if( moss->isRunning() )
		{
			m_midiClientName = MidiOss::name();
			return moss;
		}
		delete moss;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if( client_name == MidiSndio::name() || client_name == "" )
	{
		MidiSndio * msndio = new MidiSndio;
		if( msndio->isRunning() )
		{
			m_midiClientName = MidiSndio::name();
			return msndio;
		}
		delete msndio;
	}
#endif

#ifdef LMMS_BUILD_WIN32
	if( client_name == MidiWinMM::name() || client_name == "" )
	{
		MidiWinMM * mwmm = new MidiWinMM;
//		if( moss->isRunning() )
		{
			m_midiClientName = MidiWinMM::name();
			return mwmm;
		}
		delete mwmm;
	}
#endif

#ifdef LMMS_BUILD_APPLE
    printf( "trying midi apple...\n" );
    if( client_name == MidiApple::name() || client_name == "" )
    {
        MidiApple * mapple = new MidiApple;
        m_midiClientName = MidiApple::name();
        printf( "Returning midi apple\n" );
        return mapple;
    }
    printf( "midi apple didn't work: client_name=%s\n", client_name.toUtf8().constData());
#endif

	printf( "Couldn't create MIDI-client, neither with ALSA nor with "
		"OSS. Will use dummy-MIDI-client.\n" );

	m_midiClientName = MidiDummy::name();

	return new MidiDummy;
}









Mixer::fifoWriter::fifoWriter( Mixer* mixer, fifo * _fifo ) :
	m_mixer( mixer ),
	m_fifo( _fifo ),
	m_writing( true )
{
}




void Mixer::fifoWriter::finish()
{
	m_writing = false;
}




void Mixer::fifoWriter::run()
{
	disable_denormals();

#if 0
#ifdef LMMS_BUILD_LINUX
#ifdef LMMS_HAVE_SCHED_H
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( 0, &mask );
	sched_setaffinity( 0, sizeof( mask ), &mask );
#endif
#endif
#endif

	const fpp_t frames = m_mixer->framesPerPeriod();
	while( m_writing )
	{
		surroundSampleFrame * buffer = new surroundSampleFrame[frames];
		const surroundSampleFrame * b = m_mixer->renderNextBuffer();
		memcpy( buffer, b, frames * sizeof( surroundSampleFrame ) );
		write( buffer );
	}

	write( NULL );
}




void Mixer::fifoWriter::write( surroundSampleFrame * buffer )
{
	m_mixer->m_waitChangesMutex.lock();
	m_mixer->m_waitingForWrite = true;
	m_mixer->m_waitChangesMutex.unlock();
	m_mixer->runChangesInModel();

	m_fifo->write( buffer );

	m_mixer->m_doChangesMutex.lock();
	m_mixer->m_waitingForWrite = false;
	m_mixer->m_doChangesMutex.unlock();
}



