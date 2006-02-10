/*
 * mixer.cpp - audio-device-independent mixer for LMMS
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "mixer.h"
#include "play_handle.h"
#include "song_editor.h"
#include "templates.h"
#include "envelope_and_lfo_widget.h"
#include "buffer_allocator.h"
#include "debug.h"
#include "config_mgr.h"
#include "audio_port.h"
#include "sample_play_handle.h"
#include "piano_roll.h"
#include "micro_timer.h"

#include "audio_device.h"
#include "midi_client.h"

// platform-specific audio-interface-classes
#include "audio_alsa.h"
#include "audio_jack.h"
#include "audio_oss.h"
#include "audio_sdl.h"
#include "audio_dummy.h"

// platform-specific midi-interface-classes
#include "midi_alsa_raw.h"
#include "midi_alsa_seq.h"
#include "midi_oss.h"
#include "midi_dummy.h"



sample_rate_t SAMPLE_RATES[QUALITY_LEVELS] = { 44100, 88200 } ;


mixer::mixer( engine * _engine ) :
	QObject(),
	engineObject( _engine ),
	m_framesPerAudioBuffer( DEFAULT_BUFFER_SIZE ),
	m_curBuf( NULL ),
	m_nextBuf( NULL ),
	m_cpuLoad( 0 ),
	m_qualityLevel( DEFAULT_QUALITY_LEVEL ),
	m_masterGain( 1.0f ),
	m_audioDev( NULL ),
	m_oldAudioDev( NULL ),
	m_mixMutex(),
	m_mixMutexLockLevel( 0 )
{
	if( configManager::inst()->value( "mixer", "framesperaudiobuffer"
						).toInt() >= 32 )
	{
		m_framesPerAudioBuffer = configManager::inst()->value( "mixer",
					"framesperaudiobuffer" ).toInt();
	}
	else
	{
		configManager::inst()->setValue( "mixer",
							"framesperaudiobuffer",
				QString::number( m_framesPerAudioBuffer ) );
	}

	m_curBuf = bufferAllocator::alloc<surroundSampleFrame>(
						m_framesPerAudioBuffer );
	m_nextBuf = bufferAllocator::alloc<surroundSampleFrame>(
						m_framesPerAudioBuffer );

	// now clear our two output-buffers before using them...
	clearAudioBuffer( m_curBuf, m_framesPerAudioBuffer );
	clearAudioBuffer( m_nextBuf, m_framesPerAudioBuffer );

}




mixer::~mixer()
{
	delete m_audioDev;
	delete m_midiClient;

	bufferAllocator::free( m_curBuf );
	bufferAllocator::free( m_nextBuf );
}




void mixer::initDevices( void )
{
	m_audioDev = tryAudioDevices();
	m_midiClient = tryMIDIClients();
}




void mixer::startProcessing( void )
{
	m_audioDev->startProcessing();
}




void mixer::stopProcessing( void )
{
	m_audioDev->stopProcessing();
}




bool mixer::criticalXRuns( void ) const
{
	return( ( m_cpuLoad >= 98 &&
				eng()->getSongEditor()->realTimeTask() == TRUE ) );
}




const surroundSampleFrame * mixer::renderNextBuffer( void )
{
	microTimer timer;

	static songEditor::playPos last_metro_pos = -1;

	songEditor::playPos p = eng()->getSongEditor()->getPlayPos(
						songEditor::PLAY_PATTERN );
	if( eng()->getSongEditor()->playMode() == songEditor::PLAY_PATTERN &&
		eng()->getPianoRoll()->isRecording() == TRUE &&
		p != last_metro_pos && p.getTact64th() % 16 == 0 )
	{
		addPlayHandle( new samplePlayHandle( "misc/metronome01.ogg",
								eng() ) );
		last_metro_pos = p;
	}

	// now we have to make sure no other thread does anything bad
	// while we're acting...
	m_mixMutex.lock();

	// remove all play-handles that have to be deleted and delete
	// them if they still exist...
	// maybe this algorithm could be optimized...
	while( !m_playHandlesToRemove.empty() )
	{
		playHandleVector::iterator it = m_playHandles.begin();

		while( it != m_playHandles.end() )
		{
			if( *it == m_playHandlesToRemove.front() )
			{
				m_playHandles.erase( it );
				delete m_playHandlesToRemove.front();
				break;
			}
			++it;
		}

		m_playHandlesToRemove.erase( m_playHandlesToRemove.begin() );
	}

	// now swap the buffers... current buffer becomes next (last)
	// buffer and the next buffer becomes current (first) buffer
	qSwap( m_curBuf, m_nextBuf );

	// clear last audio-buffer
	clearAudioBuffer( m_curBuf, m_framesPerAudioBuffer );

//	if( criticalXRuns() == FALSE )
	{
		csize idx = 0;
		while( idx < m_playHandles.size() )
		{
			register playHandle * n = m_playHandles[idx];
			// delete play-handle if it played completely
			if( n->done() )
			{
				delete n;
				m_playHandles.erase( m_playHandles.begin() +
									idx );
			}
			else
			{
				// play all uncompletely-played play-handles...
				n->play();
				++idx;
			}
		}

		eng()->getSongEditor()->processNextBuffer();

		for( vvector<audioPort *>::iterator it = m_audioPorts.begin();
						it != m_audioPorts.end(); ++it )
		{
			if( ( *it )->m_bufferUsage != audioPort::NONE )
			{
				processBuffer( ( *it )->firstBuffer(),
						( *it )->nextFxChannel() );
				( *it )->nextPeriod();
			}
		}
	}


	emit nextAudioBuffer( m_curBuf, m_framesPerAudioBuffer );

	m_mixMutex.unlock();


	// and trigger LFOs
	envelopeAndLFOWidget::triggerLFO( eng() );

	const float new_cpu_load = timer.elapsed() / 10000.0f * sampleRate() /
							m_framesPerAudioBuffer;
	m_cpuLoad = tLimit( (int) ( new_cpu_load + m_cpuLoad ) / 2, 0, 100 );

	return( m_curBuf );
}




// removes all play-handles. this is neccessary, when the song is stopped ->
// all remaining notes etc. would be played until their end
void mixer::clear( bool _everything )
{
	// TODO: m_midiClient->noteOffAll();
	for( playHandleVector::iterator it = m_playHandles.begin();
					it != m_playHandles.end(); ++it )
	{
		// we must not delete instrument-play-handles as they exist
		// during the whole lifetime of an instrument - exception if
		// parameter _everything is true (which is the case when
		// clearing song for example)
		if( _everything == TRUE ||
			( *it )->type() != playHandle::INSTRUMENT_PLAY_HANDLE )
		{
			m_playHandlesToRemove.push_back( *it );
		}
	}
}




void FASTCALL mixer::clearAudioBuffer( sampleFrame * _ab,
							const f_cnt_t _frames )
{
	memset( _ab, 0, sizeof( *_ab ) * _frames );
}



#ifndef DISABLE_SURROUND
void FASTCALL mixer::clearAudioBuffer( surroundSampleFrame * _ab,
							const f_cnt_t _frames )
{
	memset( _ab, 0, sizeof( *_ab ) * _frames );
}
#endif



void FASTCALL mixer::bufferToPort( const sampleFrame * _buf,
					const fpab_t _frames,
					const fpab_t _frames_ahead,
					const volumeVector & _volume_vector,
						audioPort * _port )
{
	const fpab_t start_frame = _frames_ahead % m_framesPerAudioBuffer;
	fpab_t end_frame = start_frame + _frames;
	const fpab_t loop1_frame = tMin( end_frame, m_framesPerAudioBuffer );

	for( fpab_t frame = start_frame; frame < loop1_frame; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < SURROUND_CHANNELS; ++chnl )
		{
			_port->firstBuffer()[frame][chnl] +=
					_buf[frame - start_frame][chnl %
							DEFAULT_CHANNELS] *
						_volume_vector.vol[chnl];
		}
	}

	if( end_frame > m_framesPerAudioBuffer )
	{
		fpab_t frames_done = m_framesPerAudioBuffer - start_frame;
		end_frame = tMin( end_frame -= m_framesPerAudioBuffer,
						m_framesPerAudioBuffer );
		for( fpab_t frame = 0; frame < end_frame; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < SURROUND_CHANNELS;
									++chnl )
			{
				_port->secondBuffer()[frame][chnl] +=
					_buf[frames_done + frame][chnl %
							DEFAULT_CHANNELS] *
						_volume_vector.vol[chnl];
			}
		}
		// we used both buffers so set flags
		_port->m_bufferUsage = audioPort::BOTH;
	}
	else if( _port->m_bufferUsage == audioPort::NONE )
	{
		// only first buffer touched
		_port->m_bufferUsage = audioPort::FIRST;
	}

}




void mixer::setHighQuality( bool _hq_on )
{
	// delete (= close) our audio-device
	delete m_audioDev;

	// set new quality-level...
	m_qualityLevel = ( _hq_on == TRUE ) ? HIGH_QUALITY_LEVEL :
							DEFAULT_QUALITY_LEVEL;

	// and re-open device
	m_audioDev = tryAudioDevices();
	m_audioDev->startProcessing();

	emit( sampleRateChanged() );

}




void FASTCALL mixer::setAudioDevice( audioDevice * _dev, bool _hq )
{
	m_audioDev->stopProcessing();

	m_oldAudioDev = m_audioDev;

	if( _dev == NULL )
	{
		printf( "param _dev == NULL in mixer::setAudioDevice(...). "
					"Trying any working audio-device\n" );
		m_audioDev = tryAudioDevices();
	}
	else
	{
		m_audioDev = _dev;
	}

	m_qualityLevel = _hq ? HIGH_QUALITY_LEVEL : DEFAULT_QUALITY_LEVEL;
	emit sampleRateChanged();
}




void mixer::restoreAudioDevice( void )
{
	if( m_oldAudioDev != NULL )
	{
		delete m_audioDev;	// dtor automatically calls
					// stopProcessing()
		m_audioDev = m_oldAudioDev;
		for( Uint8 qli = DEFAULT_QUALITY_LEVEL;
						qli < QUALITY_LEVELS; ++qli )
		{
			if( SAMPLE_RATES[qli] == m_audioDev->sampleRate() )
			{
				m_qualityLevel =
					static_cast<qualityLevels>( qli );
				emit sampleRateChanged();
				break;
			}
		}
		m_oldAudioDev = NULL;
		m_audioDev->startProcessing();
	}
}




void mixer::checkValidityOfPlayHandles( void )
{
	playHandleVector::iterator it = m_playHandles.begin();
	while( it != m_playHandles.end() )
	{
		( *it )->checkValidity();
		++it;
	}
}





audioDevice * mixer::tryAudioDevices( void )
{
	bool success_ful = FALSE;
	audioDevice * dev = NULL;
	QString dev_name = configManager::inst()->value( "mixer", "audiodev" );

#ifdef ALSA_SUPPORT
	if( dev_name == audioALSA::name() || dev_name == "" )
	{
		dev = new audioALSA( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
							success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioALSA::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef OSS_SUPPORT
	if( dev_name == audioOSS::name() || dev_name == "" )
	{
		dev = new audioOSS( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
							success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioOSS::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef JACK_SUPPORT
	if( dev_name == audioJACK::name() || dev_name == "" )
	{
		dev = new audioJACK( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
							success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioJACK::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef SDL_AUDIO_SUPPORT
	if( dev_name == audioSDL::name() || dev_name == "" )
	{
		dev = new audioSDL( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
							success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioSDL::name();
			return( dev );
		}
		delete dev;
	}
#endif

	// add more device-classes here...
	//dev = new audioXXXX( SAMPLE_RATES[m_qualityLevel], success_ful, this );
	//if( sucess_ful )
	//{
	//	return( dev );
	//}
	//delete dev

	printf( "No audio-driver working - falling back to dummy-audio-"
		"driver\nYou can render your songs and listen to the output "
		"files...\n" );

	m_audioDevName = audioDummy::name();

	return( new audioDummy( SAMPLE_RATES[m_qualityLevel], success_ful,
								this ) );
}




midiClient * mixer::tryMIDIClients( void )
{
	QString client_name = configManager::inst()->value( "mixer",
								"mididev" );

#ifdef ALSA_SUPPORT
	if( client_name == midiALSASeq::name() || client_name == "" )
	{
		midiALSASeq * malsas = new midiALSASeq( eng() );
		if( malsas->isRunning() )
		{
			m_midiClientName = midiALSASeq::name();
			return( malsas );
		}
		delete malsas;
	}

	if( client_name == midiALSARaw::name() || client_name == "" )
	{
		midiALSARaw * malsar = new midiALSARaw( eng() );
		if( malsar->isRunning() )
		{
			m_midiClientName = midiALSARaw::name();
			return( malsar );
		}
		delete malsar;
	}
#endif

#ifdef OSS_SUPPORT
	if( client_name == midiOSS::name() || client_name == "" )
	{
		midiOSS * moss = new midiOSS( eng() );
		if( moss->isRunning() )
		{
			m_midiClientName = midiOSS::name();
			return( moss );
		}
		delete moss;
	}
#endif

	printf( "Couldn't create MIDI-client, neither with ALSA nor with "
		"OSS. Will use dummy-MIDI-client.\n" );

	m_midiClientName = midiDummy::name();

	return( new midiDummy( eng() ) );
}




void mixer::processBuffer( const surroundSampleFrame * _buf,
						fx_ch_t/* _fx_chnl */ )
{
	// TODO: effect-implementation
	for( fpab_t frame = 0; frame < m_framesPerAudioBuffer; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < SURROUND_CHANNELS; ++chnl )
		{
			m_curBuf[frame][chnl] += _buf[frame][chnl];
		}
	}
}


#include "mixer.moc"

