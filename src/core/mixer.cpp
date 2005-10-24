/*
 * mixer.cpp - audio-device-independent mixer for LMMS
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
#include "midi_oss.h"
#include "midi_dummy.h"



Uint32 SAMPLE_RATES[QUALITY_LEVELS] = { 44100, 88200 } ;

mixer * mixer::s_instanceOfMe = NULL;


mixer::mixer() :
#ifndef QT4
	QObject(),
#endif
	QThread(),
	m_silence(),
#ifndef DISABLE_SURROUND
	m_surroundSilence(),
#endif
	m_framesPerAudioBuffer( DEFAULT_BUFFER_SIZE ),
	m_buffer1( NULL ),
	m_buffer2( NULL ),
	m_curBuf( NULL ),
	m_nextBuf( NULL ),
	m_discardCurBuf( FALSE ),
	m_qualityLevel( DEFAULT_QUALITY_LEVEL ),
	m_masterOutput( 1.0f ),
	m_quit( FALSE ),
	m_audioDev( NULL ),
	m_oldAudioDev( NULL )
{
	// small hack because code calling mixer::inst() is called out of ctor
	s_instanceOfMe = this;

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

	m_buffer1 = bufferAllocator::alloc<surroundSampleFrame>(
						m_framesPerAudioBuffer );
	m_buffer2 = bufferAllocator::alloc<surroundSampleFrame>(
						m_framesPerAudioBuffer );

	m_curBuf = m_buffer1;
	m_nextBuf = m_buffer2;


	m_audioDev = tryAudioDevices();
	m_midiClient = tryMIDIClients();


	for( int i = 0; i < MAX_SAMPLE_PACKETS; ++i )
	{
		m_samplePackets[i].m_buffer = NULL;
		m_samplePackets[i].m_state = samplePacket::UNUSED;
	}

	m_silence = bufferAllocator::alloc<sampleFrame>(
						m_framesPerAudioBuffer );
#ifndef DISABLE_SURROUND
	m_surroundSilence = bufferAllocator::alloc<surroundSampleFrame>(
						m_framesPerAudioBuffer );
#endif
	for( Uint32 frame = 0; frame < m_framesPerAudioBuffer; ++frame )
	{
		for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			m_silence[frame][chnl] = 0.0f;
		}
#ifndef DISABLE_SURROUND
		for( Uint8 chnl = 0; chnl < SURROUND_CHANNELS; ++chnl )
		{
			m_surroundSilence[frame][chnl] = 0.0f;
		}
#endif
	}

	// now clear our two output-buffers before using them...
	clearAudioBuffer( m_buffer1, m_framesPerAudioBuffer );
	clearAudioBuffer( m_buffer2, m_framesPerAudioBuffer );

}




mixer::~mixer()
{
	delete m_audioDev;

	bufferAllocator::free( m_buffer1 );
	bufferAllocator::free( m_buffer2 );

	for( int i = 0; i < MAX_SAMPLE_PACKETS; ++i )
	{
		if( m_samplePackets[i].m_state != samplePacket::UNUSED )
		{
			bufferAllocator::free( m_samplePackets[i].m_buffer );
		}
	}

	bufferAllocator::free( m_silence );
#ifndef DISABLE_SURROUND
	bufferAllocator::free( m_surroundSilence );
#endif
}




void mixer::quitThread( void )
{
	// make sure there're no mutexes locked anymore...
	m_safetySyncMutex.unlock();
	m_devMutex.unlock();

	// now tell mixer-thread to quit
	m_quit = TRUE;

	wait( 1000 );
	terminate();
}




void mixer::run( void )
{

	while( m_quit == FALSE )
	{

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

			m_playHandlesToRemove.erase(
						m_playHandlesToRemove.begin() );

		}

		// now we have to make sure no other thread does anything bad
		// while we're acting...
		m_safetySyncMutex.lock();

		csize idx = 0;
		while( idx < m_playHandles.size() )
		{
			register playHandle * n = m_playHandles[idx];
			if( n->done() )
			{
				// delete all play-handles which have
				// played completely now
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

		songEditor::inst()->processNextBuffer();

		// check for samples-packets that have to be mixed in
		// the current audio-buffer
		for( int i = 0; i < MAX_SAMPLE_PACKETS; ++i )
		{
			if( m_samplePackets[i].m_state == samplePacket::READY )
			{
				if( m_samplePackets[i].m_framesAhead <=
							m_framesPerAudioBuffer )
				{
					// found one! mix it...
					mixSamplePacket( &m_samplePackets[i] );
					// now this audio-sample can be used
					// again
					bufferAllocator::free(
						m_samplePackets[i].m_buffer );
					m_samplePackets[i].m_state =
							samplePacket::UNUSED;
				}
				else
				{
					m_samplePackets[i].m_framesAhead -=
							m_framesPerAudioBuffer;
				}

			}

		}

		if( !m_discardCurBuf )
		{
			m_devMutex.lock();
			// write actual data to our current output-device
			// (blocking!)
			m_audioDev->writeBuffer( m_curBuf,
							m_framesPerAudioBuffer,
						SAMPLE_RATES[m_qualityLevel],
							m_masterOutput );
			m_devMutex.unlock();
		}
		else
		{
			m_discardCurBuf = FALSE;
		}

		emit nextAudioBuffer( m_curBuf, m_framesPerAudioBuffer );


		m_safetySyncMutex.unlock();


		// clear last audio-buffer
		clearAudioBuffer( m_curBuf, m_framesPerAudioBuffer );

		// now swap the buffers... current buffer becomes next (last)
		// buffer and the next buffer becomes current (first) buffer
		qSwap( m_curBuf, m_nextBuf );

		// and trigger LFOs
		envelopeAndLFOWidget::triggerLFO();
	}

}




// removes all play-handles. this is neccessary, when the song is stopped ->
// all remaining notes etc. would be played until their end
void mixer::clear( void )
{
	// TODO: m_midiClient->noteOffAll();
	for( playHandleVector::iterator it = m_playHandles.begin();
					it != m_playHandles.end(); ++it )
	{
		m_playHandlesToRemove.push_back( *it );
	}
}




void FASTCALL mixer::clearAudioBuffer( sampleFrame * _ab, Uint32 _frames )
{
	if( _frames == m_framesPerAudioBuffer )
	{
		memcpy( _ab, m_silence, m_framesPerAudioBuffer *
							BYTES_PER_FRAME );
	}
	else
	{
		for( Uint32 frame = 0; frame < _frames; ++frame )
		{
			for( Uint8 ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				_ab[frame][ch] = 0.0f;
			}
		}
	}
}



#ifndef DISABLE_SURROUND
void FASTCALL mixer::clearAudioBuffer( surroundSampleFrame * _ab,
								Uint32 _frames )
{
	if( _frames == m_framesPerAudioBuffer )
	{
		memcpy( _ab, m_surroundSilence, m_framesPerAudioBuffer *
						BYTES_PER_SURROUND_FRAME );
	}
	else
	{
		for( Uint32 frame = 0; frame < _frames; ++frame )
		{
			for( Uint8 ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				_ab[frame][ch] = 0.0f;
			}
		}
	}
}
#endif



void FASTCALL mixer::addBuffer( sampleFrame * _buf, Uint32 _frames,
						Uint32 _frames_ahead,
						volumeVector & _volume_vector )
{
#ifdef LMMS_DEBUG
	bool success = FALSE;
#endif
	for ( Uint16 i = 0; i < MAX_SAMPLE_PACKETS; ++i )
	{
		if( m_samplePackets[i].m_state == samplePacket::UNUSED )
		{
			m_samplePackets[i].m_state = samplePacket::FILLING;
			m_samplePackets[i].m_frames = _frames;//m_framesPerAudioBuffer;
			m_samplePackets[i].m_framesDone  = 0;
			m_samplePackets[i].m_framesAhead  = _frames_ahead;

			m_samplePackets[i].m_buffer =
				bufferAllocator::alloc<surroundSampleFrame>(
						m_framesPerAudioBuffer );
			// now we have to make a surround-buffer out of a
			// stereo-buffer (could be done more easily if there
			// would be no volume-vector...)
			for( Uint32 frame = 0; frame < _frames/*m_framesPerAudioBuffer*/;
								++frame )
			{
				for( Uint8 chnl = 0; chnl < SURROUND_CHANNELS;
									++chnl )
				{
			m_samplePackets[i].m_buffer[frame][chnl] =
					_buf[frame][chnl%DEFAULT_CHANNELS] *
						_volume_vector.vol[chnl];
				}
			}

			m_samplePackets[i].m_state = samplePacket::READY;
#ifdef LMMS_DEBUG
			success = TRUE;
#endif
			break;
		}
	}
#ifdef LMMS_DEBUG
	if( success == FALSE )
	{
		qWarning( "No sample-packets left in mixer::addBuffer(...)!\n" );
	}
#endif
}




void mixer::setHighQuality( bool _hq_on )
{
	m_safetySyncMutex.lock();

	// delete (= close) our audio-device
	delete m_audioDev;

	// set new quality-level...
	if( _hq_on == TRUE )
	{
		m_qualityLevel = HIGH_QUALITY_LEVEL;
	}
	else
	{
		m_qualityLevel = DEFAULT_QUALITY_LEVEL;
	}
	// and re-open device
	m_audioDev = tryAudioDevices();

	m_safetySyncMutex.unlock();

	emit( sampleRateChanged() );

}




void FASTCALL mixer::setAudioDevice( audioDevice * _dev, bool _hq )
{

	m_devMutex.lock();

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

	m_qualityLevel = _hq ? 1 : 0;
	emit sampleRateChanged();

	m_devMutex.unlock();

}




void mixer::restoreAudioDevice( void )
{
	m_devMutex.lock();

	if( m_oldAudioDev != NULL )
	{
		delete m_audioDev;
		m_audioDev = m_oldAudioDev;
		for( Uint8 qli = 0; qli < QUALITY_LEVELS; ++qli )
		{
			if( SAMPLE_RATES[qli] == m_audioDev->sampleRate() )
			{
				m_qualityLevel = qli;
				emit sampleRateChanged();
				break;
			}
		}
		m_oldAudioDev = NULL;
		m_discardCurBuf = TRUE;
	}

	m_devMutex.unlock();
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




void FASTCALL mixer::mixSamplePacket( samplePacket * _sp )
{
	Uint32 start_frame = _sp->m_framesAhead % m_framesPerAudioBuffer;
	Uint32 end_frame = start_frame + _sp->m_frames;//m_framesPerAudioBuffer;

	if( end_frame <= m_framesPerAudioBuffer )
	{
		for( Uint32 frame = start_frame; frame < end_frame; ++frame )
		{
			for( Uint8 chnl = 0; chnl < SURROUND_CHANNELS; ++chnl )
			{
				m_curBuf[frame][chnl] +=
					_sp->m_buffer[frame-start_frame][chnl];
			}
		}
	}
	else
	{
		for( Uint32 frame = start_frame; frame <
					m_framesPerAudioBuffer; ++frame )
		{
			for( Uint8 chnl = 0; chnl < SURROUND_CHANNELS; ++chnl )
			{
				m_curBuf[frame][chnl] +=
					_sp->m_buffer[frame-start_frame][chnl];
			}
		}

		Uint32 frames_done = m_framesPerAudioBuffer - start_frame;
		end_frame = tMin( end_frame -= m_framesPerAudioBuffer,
						m_framesPerAudioBuffer );

		for( Uint32 frame = 0; frame < end_frame; ++frame )
		{
			for( Uint8 chnl = 0; chnl < SURROUND_CHANNELS; ++chnl )
			{
				m_nextBuf[frame][chnl] +=
					_sp->m_buffer[frames_done+frame][chnl];
			}
		}
	}
}




audioDevice * mixer::tryAudioDevices( void )
{
	//m_discardCurBuf = TRUE;

	bool success_ful = FALSE;
	audioDevice * dev = NULL;
	QString dev_name = configManager::inst()->value( "mixer", "audiodev" );

#ifdef OSS_SUPPORT
	if( dev_name == audioOSS::name() || dev_name == "" )
	{
		dev = new audioOSS( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
								success_ful );
		if( success_ful )
		{
			m_audioDevName = audioOSS::name();
			return( dev );
		}
		delete dev;
	}
#endif

#ifdef ALSA_SUPPORT
	if( dev_name == audioALSA::name() || dev_name == "" )
	{
		dev = new audioALSA( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
								success_ful );
		if( success_ful )
		{
			m_audioDevName = audioALSA::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef JACK_SUPPORT
	if( dev_name == audioJACK::name() || dev_name == "" )
	{
		dev = new audioJACK( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
								success_ful );
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
								success_ful );
		if( success_ful )
		{
			m_audioDevName = audioSDL::name();
			return( dev );
		}
		delete dev;
	}
#endif

	// add more device-classes here...
	//dev = new audioXXXX( SAMPLE_RATES[m_qualityLevel], success_ful );
	//if( sucess_ful )
	//{
	//	return( dev );
	//}
	//delete dev

	printf( "No audio-driver working - falling back to dummy-audio-"
		"driver\nYou can render your songs and listen to the output "
		"files...\n" );

	m_audioDevName = audioDummy::name();

	return( new audioDummy( SAMPLE_RATES[m_qualityLevel], success_ful ) );
}




midiClient * mixer::tryMIDIClients( void )
{
	QString client_name = configManager::inst()->value( "mixer",
								"midiclient" );

#ifdef ALSA_SUPPORT
	if( client_name == midiALSARaw::name() || client_name == "" )
	{
		midiALSARaw * malsa = new midiALSARaw();
		if( malsa->isRunning() )
		{
			m_midiClientName = midiALSARaw::name();
			return( malsa );
		}
		delete malsa;
	}
#endif

#ifdef OSS_SUPPORT
	if( client_name == midiOSS::name() || client_name == "" )
	{
		midiOSS * moss = new midiOSS();
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

	return( new midiDummy() );
}



#include "mixer.moc"

