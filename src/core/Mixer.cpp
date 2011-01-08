/*
 * Mixer.cpp - Mixer for audio processing and rendering
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

#include <math.h>

#include "AudioOutputContext.h"
#include "Mixer.h"
#include "FxMixer.h"
#include "MixerWorkerThread.h"
#include "play_handle.h"
#include "song.h"
#include "templates.h"
#include "EnvelopeAndLfoParameters.h"
#include "note_play_handle.h"
#include "InstrumentTrack.h"
#include "debug.h"
#include "engine.h"
#include "config_mgr.h"
#include "sample_play_handle.h"
#include "piano_roll.h"
#include "Cpu.h"
#include "MicroTimer.h"

// platform-specific audio-interface-classes
#include "AudioAlsa.h"
#include "AudioJack.h"
#include "AudioOss.h"
#include "AudioPortAudio.h"
#include "AudioPulseAudio.h"
#include "AudioSdl.h"
#include "AudioDummy.h"

// platform-specific midi-interface-classes
#include "MidiAlsaRaw.h"
#include "MidiAlsaSeq.h"
#include "MidiOss.h"
#include "MidiWinMM.h"
#include "MidiDummy.h"

#ifdef LMMS_HAVE_PTHREAD_H
#include <pthread.h>
#endif


Mixer::Mixer() :
	m_framesPerPeriod( qBound<int>( 32,
					configManager::inst()->value( "mixer", "framesperaudiobuffer" ).toInt(),
					DEFAULT_BUFFER_SIZE ) ),
	m_workingBuf( NULL ),
	m_inputBufferRead( 0 ),
	m_inputBufferWrite( 1 ),
	m_readBuf( NULL ),
	m_writeBuf( NULL ),
	m_cpuLoad( 0 ),
	m_workers(),
	m_numWorkers( QThread::idealThreadCount()-1 ),
	m_queueReadyWaitCond(),
	m_masterGain( 1.0f ),
	m_audioOutputContext( NULL ),
	m_defaultAudioOutputContext( NULL ),
	m_globalMutex( QMutex::Recursive )
{
	for( int i = 0; i < 2; ++i )
	{
		m_inputBufferFrames[i] = 0;
		m_inputBufferSize[i] = DEFAULT_BUFFER_SIZE * 100;
		m_inputBuffer[i] = CPU::allocFrames(
						DEFAULT_BUFFER_SIZE * 100 );
		clearAudioBuffer( m_inputBuffer[i], m_inputBufferSize[i] );
	}

	m_workingBuf = CPU::allocFrames( m_framesPerPeriod );
	for( Uint8 i = 0; i < 3; i++ )
	{
		m_readBuf = CPU::allocFrames( m_framesPerPeriod );
		clearAudioBuffer( m_readBuf, m_framesPerPeriod );
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

	// initialize default AudioOutputContext
	m_defaultAudioOutputContext = new AudioOutputContext( this, NULL,
							AudioOutputContext::QualitySettings::Preset_Draft );
	m_audioOutputContext = m_defaultAudioOutputContext;
}




Mixer::~Mixer()
{
	for( int w = 0; w < m_numWorkers; ++w )
	{
		m_workers[w]->quit();
	}
	MixerWorkerThread::startAndWaitForJobs();
	for( int w = 0; w < m_numWorkers; ++w )
	{
		m_workers[w]->wait( 500 );
	}

	delete m_audioOutputContext;
	delete m_midiClient;

	for( Uint8 i = 0; i < 3; i++ )
	{
		CPU::freeFrames( m_bufferPool[i] );
	}

	CPU::freeFrames( m_workingBuf );

	for( int i = 0; i < 2; ++i )
	{
		CPU::freeFrames( m_inputBuffer[i] );
	}
}




void Mixer::initDevices()
{
	audioOutputContext()->setAudioBackend( tryAudioBackends() );
	m_midiClient = tryMidiClients();
}




void Mixer::setAudioOutputContext( AudioOutputContext * context )
{
	stopProcessing();

	m_audioOutputContext = context;

	//m_audioDev->applyQualitySettings();

	emit sampleRateChanged();

	startProcessing();
}




void Mixer::startProcessing()
{
	if( m_audioOutputContext )
	{
		m_audioOutputContext->startProcessing();
	}
}




void Mixer::stopProcessing()
{
	if( m_audioOutputContext )
	{
		m_audioOutputContext->stopProcessing();
	}
}




sample_rate_t Mixer::baseSampleRate() const
{
	sample_rate_t sr =
		configManager::inst()->value( "mixer", "samplerate" ).toInt();
	if( sr < 44100 )
	{
		sr = 44100;
	}
	return sr;
}




sample_rate_t Mixer::outputSampleRate() const
{
	if( audioOutputContext()->audioBackend() )
	{
		return audioOutputContext()->audioBackend()->sampleRate();
	}
	return baseSampleRate();
}




sample_rate_t Mixer::inputSampleRate() const
{
	return outputSampleRate();
}




sample_rate_t Mixer::processingSampleRate() const
{
	return outputSampleRate() *
			audioOutputContext()->qualitySettings().sampleRateMultiplier();
}




bool Mixer::criticalXRuns() const
{
	return m_cpuLoad >= 99 && engine::getSong()->realTimeTask() == true;
}




void Mixer::pushInputFrames( sampleFrame * _ab, const f_cnt_t _frames )
{
	lockInputFrames();

	f_cnt_t frames = m_inputBufferFrames[ m_inputBufferWrite ];
	int size = m_inputBufferSize[ m_inputBufferWrite ];
	sampleFrame * buf = m_inputBuffer[ m_inputBufferWrite ];

	if( frames + _frames > size )
	{
		size = qMax( size * 2, frames + _frames );
		sampleFrame * ab = CPU::allocFrames( size );
		CPU::memCpy( ab, buf, frames * sizeof( sampleFrame ) );
		CPU::freeFrames( buf );

		m_inputBufferSize[ m_inputBufferWrite ] = size;
		m_inputBuffer[ m_inputBufferWrite ] = ab;

		buf = ab;
	}

	CPU::memCpy( &buf[ frames ], _ab, _frames * sizeof( sampleFrame ) );
	m_inputBufferFrames[ m_inputBufferWrite ] += _frames;

	unlockInputFrames();
}




sampleFrameA * Mixer::renderNextBuffer()
{
	MicroTimer timer;
	static song::playPos last_metro_pos = -1;

	FxMixer * fxm = engine::fxMixer();

	song::playPos p = engine::getSong()->getPlayPos( song::Mode_PlayPattern );
	if( engine::getSong()->playMode() == song::Mode_PlayPattern &&
		engine::getPianoRoll()->isRecording() == true &&
		p != last_metro_pos && p.getTicks() %
					(DefaultTicksPerTact / 4 ) == 0 )
	{
		addPlayHandle( new samplePlayHandle( "misc/metronome01.ogg" ) );
		last_metro_pos = p;
	}

	lockInputFrames();
	// swap buffer
	m_inputBufferWrite = ( m_inputBufferWrite + 1 ) % 2;
	m_inputBufferRead =  ( m_inputBufferRead + 1 ) % 2;
	// clear new write buffer
	m_inputBufferFrames[ m_inputBufferWrite ] = 0;
	unlockInputFrames();


	// now we have to make sure no other thread does anything bad
	// while we're acting...
	lock();

	// remove all play-handles that have to be deleted and delete
	// them if they still exist...
	// maybe this algorithm could be optimized...
	ConstPlayHandleList::Iterator it_rem = m_playHandlesToRemove.begin();
	while( it_rem != m_playHandlesToRemove.end() )
	{
		PlayHandleList::Iterator it = qFind( m_playHandles.begin(),
						m_playHandles.end(), *it_rem );

		if( it != m_playHandles.end() )
		{
			delete *it;
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
	clearAudioBuffer( m_writeBuf, m_framesPerPeriod );

	// prepare master mix (clear internal buffers etc.)
	fxm->prepareMasterMix();

	// create play-handles for new notes, samples etc.
	engine::getSong()->processNextBuffer();


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
		if( ( *it )->done() )
		{
			delete *it;
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
	fxm->masterMix( m_writeBuf );

	unlock();


	emit nextAudioBuffer();

	// and trigger LFOs
	EnvelopeAndLfoParameters::instances()->trigger();
	Controller::triggerFrameCounter();

	const float new_cpu_load = timer.elapsed() / 10000.0f *
				processingSampleRate() / m_framesPerPeriod;
	m_cpuLoad = tLimit( (int) ( new_cpu_load * 0.1f + m_cpuLoad * 0.9f ), 0,
									100 );

	return m_readBuf;
}




// removes all play-handles. this is necessary, when the song is stopped ->
// all remaining notes etc. would be played until their end
void Mixer::clear()
{
	// TODO: m_midiClient->noteOffAll();
	lock();
	for( PlayHandleList::Iterator it = m_playHandles.begin();
					it != m_playHandles.end(); ++it )
	{
		// we must not delete instrument-play-handles as they exist
		// during the whole lifetime of an instrument
		if( ( *it )->type() != playHandle::InstrumentPlayHandle )
		{
			m_playHandlesToRemove.push_back( *it );
		}
	}
	unlock();
}




void Mixer::bufferToPort( const sampleFrame * _buf,
					const fpp_t _frames,
					const f_cnt_t _offset,
					stereoVolumeVector _vv,
						AudioPort * _port )
{
	const int start_frame = _offset % m_framesPerPeriod;
	int end_frame = start_frame + _frames;
	const int loop1_frame = qMin<int>( end_frame, m_framesPerPeriod );

	_port->lockFirstBuffer();
	CPU::unalignedBufMixLRCoeff( _port->firstBuffer() + start_frame,
					_buf, _vv.vol[0], _vv.vol[1],
						loop1_frame - start_frame );
	_port->unlockFirstBuffer();

	_port->lockSecondBuffer();
	if( end_frame > m_framesPerPeriod )
	{
		const int frames_done = m_framesPerPeriod - start_frame;
		end_frame -= m_framesPerPeriod;
		end_frame = qMin<int>( end_frame, m_framesPerPeriod );
		CPU::unalignedBufMixLRCoeff( _port->secondBuffer(),
						_buf+frames_done,
						_vv.vol[0], _vv.vol[1],
						end_frame );
		// we used both buffers so set flags
		_port->m_bufferUsage = AudioPort::BothBuffers;
	}
	else if( _port->m_bufferUsage == AudioPort::NoUsage )
	{
		// only first buffer touched
		_port->m_bufferUsage = AudioPort::FirstBuffer;
	}
	_port->unlockSecondBuffer();
}




void Mixer::clearAudioBuffer( sampleFrame * _ab, const f_cnt_t _frames,
							const f_cnt_t _offset )
{
	if( likely( (size_t)( _ab+_offset ) % 16 == 0 && _frames % 8 == 0 ) )
	{
		CPU::memClear( _ab+_offset, sizeof( *_ab ) * _frames );
	}
	else
	{
		memset( _ab+_offset, 0, sizeof( *_ab ) * _frames );
	}
}




float Mixer::peakValueLeft( sampleFrame * _ab, const f_cnt_t _frames )
{
	float p = 0.0f;
	for( f_cnt_t f = 0; f < _frames; ++f )
	{
		if( _ab[f][0] > p )
		{
			p = _ab[f][0];
		}
		else if( -_ab[f][0] > p )
		{
			p = -_ab[f][0];
		}
	}
	return p;
}




float Mixer::peakValueRight( sampleFrame * _ab, const f_cnt_t _frames )
{
	float p = 0.0f;
	for( f_cnt_t f = 0; f < _frames; ++f )
	{
		if( _ab[f][1] > p )
		{
			p = _ab[f][1];
		}
		else if( -_ab[f][1] > p )
		{
			p = -_ab[f][1];
		}
	}
	return p;
}




void Mixer::removeAudioPort( AudioPort * _port )
{
	QVector<AudioPort *>::Iterator it = qFind( m_audioPorts.begin(),
							m_audioPorts.end(),
							_port );
	if( it != m_audioPorts.end() )
	{
		lock();
		m_audioPorts.erase( it );
		unlock();
	}
}




void Mixer::removePlayHandle( playHandle * _ph )
{
	lock();
	// check thread affinity as we must not delete play-handles
	// which were created in a thread different than mixer thread
	if( _ph->affinityMatters() &&
				_ph->affinity() == QThread::currentThread() )
	{
		PlayHandleList::Iterator it =
				qFind( m_playHandles.begin(),
						m_playHandles.end(), _ph );
		if( it != m_playHandles.end() )
		{
			m_playHandles.erase( it );
			delete _ph;
		}
	}
	else
	{
		m_playHandlesToRemove.push_back( _ph );
	}
	unlock();
}




void Mixer::removePlayHandles( track * _track, playHandle::Type _type )
{
	lock();
	PlayHandleList::Iterator it = m_playHandles.begin();
	while( it != m_playHandles.end() )
	{
		if( ( _type == playHandle::NumPlayHandleTypes ||
			( *it )->type() == _type ) &&
				( *it )->isFromTrack( _track ) )
		{
			delete *it;
			it = m_playHandles.erase( it );
		}
		else
		{
			++it;
		}
	}
	unlock();
}




AudioBackend * Mixer::tryAudioBackends()
{
	bool success_ful = false;
	AudioBackend * dev = NULL;
	QString dev_name = configManager::inst()->value( "mixer", "audiodev" );

	if( dev_name == AudioDummy::name() )
	{
		dev_name = "";
	}

#ifdef LMMS_HAVE_ALSA
	if( dev_name == AudioAlsa::name() || dev_name == "" )
	{
		dev = new AudioAlsa( success_ful, audioOutputContext() );
		if( success_ful )
		{
			m_audioDevName = AudioAlsa::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PORTAUDIO
	if( dev_name == AudioPortAudio::name() || dev_name == "" )
	{
		dev = new AudioPortAudio( success_ful, audioOutputContext() );
		if( success_ful )
		{
			m_audioDevName = AudioPortAudio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PULSEAUDIO
	if( dev_name == AudioPulseAudio::name() || dev_name == "" )
	{
		dev = new AudioPulseAudio( success_ful, audioOutputContext() );
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
		dev = new AudioOss( success_ful, audioOutputContext() );
		if( success_ful )
		{
			m_audioDevName = AudioOss::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_JACK
	if( dev_name == AudioJack::name() || dev_name == "" )
	{
		dev = new AudioJack( success_ful, audioOutputContext() );
		if( success_ful )
		{
			m_audioDevName = AudioJack::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_SDL
	if( dev_name == AudioSdl::name() || dev_name == "" )
	{
		dev = new AudioSdl( success_ful, audioOutputContext() );
		if( success_ful )
		{
			m_audioDevName = AudioSdl::name();
			return dev;
		}
		delete dev;
	}
#endif

	// add more device-classes here...
	//dev = new audioXXXX( SAMPLE_RATES[m_qualityLevel], success_ful, audioOutputContext() );
	//if( sucess_ful )
	//{
	//	return dev;
	//}
	//delete dev

	printf( "No audio-driver working - falling back to dummy-audio-"
		"driver\nYou can render your songs and listen to the output "
		"files...\n" );

	m_audioDevName = AudioDummy::name();

	return new AudioDummy( success_ful, audioOutputContext() );
}




MidiClient * Mixer::tryMidiClients()
{
	QString client_name = configManager::inst()->value( "mixer",
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

	printf( "Couldn't create MIDI-client, neither with ALSA nor with "
		"OSS. Will use dummy-MIDI-client.\n" );

	m_midiClientName = MidiDummy::name();

	return new MidiDummy;
}



#include "moc_Mixer.cxx"

