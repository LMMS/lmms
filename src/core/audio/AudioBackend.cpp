/*
 * AudioBackend.cpp - base-class for audio-devices used by LMMS-mixer
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

#include "AudioBackend.h"
#include "AudioOutputContext.h"
#include "config_mgr.h"
#include "debug.h"
#include "Cpu.h"



AudioBackend::AudioBackend( const ch_cnt_t _channels,
								AudioOutputContext * context ) :
	m_supportsCapture( false ),
	m_context( context ),
	m_sampleRate( mixer()->processingSampleRate() ),
	m_channels( _channels ),
	m_buffer( CPU::allocFrames( mixer()->framesPerPeriod() ) )
{
}




AudioBackend::~AudioBackend()
{
	CPU::freeFrames( m_buffer );
}




int AudioBackend::processNextBuffer()
{
	const int frames = getNextBuffer( m_buffer );
	if( frames )
	{
		writeBuffer( m_buffer, frames, mixer()->masterGain() );
	}
	return frames;
}




int AudioBackend::getNextBuffer( sampleFrameA * _ab )
{
	return outputContext()->getCurrentOutputBuffer( _ab, sampleRate() );
}




void AudioBackend::stopProcessing()
{
	// flush AudioOutputContext's FIFO
	while( processNextBuffer() )
	{
	}
}




void AudioBackend::applyQualitySettings()
{
}




void AudioBackend::registerPort( AudioPort * )
{
}




void AudioBackend::unregisterPort( AudioPort * _port )
{
}




void AudioBackend::renamePort( AudioPort * )
{
}




void AudioBackend::clearS16Buffer( intSampleFrameA * _outbuf, const fpp_t _frames )
{
	CPU::memClear( _outbuf, _frames * sizeof( *_outbuf ) );
//	memset( _outbuf, 0,  _frames * channels() * BYTES_PER_INT_SAMPLE );
}




bool AudioBackend::hqAudio() const
{
	return configManager::inst()->value( "mixer", "hqaudio" ).toInt();
}




const Mixer * AudioBackend::mixer() const
{
	return outputContext()->mixer();
}




Mixer * AudioBackend::mixer()
{
	return outputContext()->mixer();
}


