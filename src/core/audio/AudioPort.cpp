/*
 * AudioPort.cpp - base-class for objects providing sound at a port
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

#include "AudioPort.h"
#include "AudioDevice.h"
#include "AudioEngine.h"
#include "EffectChain.h"
#include "Mixer.h"
#include "Engine.h"
#include "MixHelpers.h"
#include "BufferManager.h"

namespace lmms
{

AudioPort::AudioPort( const QString & _name, bool _has_effect_chain,
		FloatModel * volumeModel, FloatModel * panningModel,
		BoolModel * mutedModel ) :
	m_bufferUsage( false ),
	m_portBuffer(BufferManager::acquire()),
	m_extOutputEnabled( false ),
	m_nextMixerChannel( 0 ),
	m_name( "unnamed port" ),
	m_effects( _has_effect_chain ? new EffectChain( nullptr ) : nullptr ),
	m_volumeModel( volumeModel ),
	m_panningModel( panningModel ),
	m_mutedModel( mutedModel )
{
	Engine::audioEngine()->addAudioPort( this );
	setExtOutputEnabled( true );
}




AudioPort::~AudioPort()
{
	setExtOutputEnabled( false );
	Engine::audioEngine()->removeAudioPort( this );
	BufferManager::release(m_portBuffer.data());
}




void AudioPort::setExtOutputEnabled( bool _enabled )
{
	if( _enabled != m_extOutputEnabled )
	{
		m_extOutputEnabled = _enabled;
		if( m_extOutputEnabled )
		{
			Engine::audioEngine()->audioDev()->registerPort( this );
		}
		else
		{
			Engine::audioEngine()->audioDev()->unregisterPort( this );
		}
	}
}




void AudioPort::setName( const QString & _name )
{
	m_name = _name;
	Engine::audioEngine()->audioDev()->renamePort( this );
}




bool AudioPort::processEffects()
{
	if( m_effects )
	{
		bool more = m_effects->processAudioBuffer(m_portBuffer.data(), m_portBuffer.size(), m_bufferUsage );
		return more;
	}
	return false;
}


void AudioPort::doProcessing()
{
	if( m_mutedModel && m_mutedModel->value() )
	{
		return;
	}

	const fpp_t fpp = Engine::audioEngine()->framesPerPeriod();
	assert(m_portBuffer.size() == fpp);

	// clear the buffer
	zeroSampleFrames(m_portBuffer.data(), m_portBuffer.size());

	//qDebug( "Playhandles: %d", m_playHandles.size() );
	for( PlayHandle * ph : m_playHandles ) // now we mix all playhandle buffers into the audioport buffer
	{
		if (auto phBuffer = ph->buffer(); phBuffer.data() != nullptr)
		{
			assert(phBuffer.size() == m_portBuffer.size());
			if (ph->usesBuffer()
				&& (ph->type() == PlayHandle::Type::NotePlayHandle
					|| !MixHelpers::isSilent(phBuffer.data(), phBuffer.size())))
			{
				m_bufferUsage = true;
				MixHelpers::add(m_portBuffer.data(), phBuffer.data(), fpp);
			}
			ph->releaseBuffer(); 	// gets rid of playhandle's buffer and sets
									// pointer to null, so if it doesn't get re-acquired we know to skip it next time
		}
	}

	if( m_bufferUsage )
	{
		// handle volume and panning
		// has both vol and pan models
		if( m_volumeModel && m_panningModel )
		{
			ValueBuffer * volBuf = m_volumeModel->valueBuffer();
			ValueBuffer * panBuf = m_panningModel->valueBuffer();

			// both vol and pan have s.ex.data:
			if( volBuf && panBuf )
			{
				for( f_cnt_t f = 0; f < fpp; ++f )
				{
					float v = volBuf->values()[ f ] * 0.01f;
					float p = panBuf->values()[ f ] * 0.01f;
					m_portBuffer[f][0] *= ( p <= 0 ? 1.0f : 1.0f - p ) * v;
					m_portBuffer[f][1] *= ( p >= 0 ? 1.0f : 1.0f + p ) * v;
				}
			}

			// only vol has s.ex.data:
			else if( volBuf )
			{
				float p = m_panningModel->value() * 0.01f;
				float l = ( p <= 0 ? 1.0f : 1.0f - p );
				float r = ( p >= 0 ? 1.0f : 1.0f + p );
				for( f_cnt_t f = 0; f < fpp; ++f )
				{
					float v = volBuf->values()[ f ] * 0.01f;
					m_portBuffer[f][0] *= v * l;
					m_portBuffer[f][1] *= v * r;
				}
			}

			// only pan has s.ex.data:
			else if( panBuf )
			{
				float v = m_volumeModel->value() * 0.01f;
				for( f_cnt_t f = 0; f < fpp; ++f )
				{
					float p = panBuf->values()[ f ] * 0.01f;
					m_portBuffer[f][0] *= ( p <= 0 ? 1.0f : 1.0f - p ) * v;
					m_portBuffer[f][1] *= ( p >= 0 ? 1.0f : 1.0f + p ) * v;
				}
			}

			// neither has s.ex.data:
			else
			{
				float p = m_panningModel->value() * 0.01f;
				float v = m_volumeModel->value() * 0.01f;
				for( f_cnt_t f = 0; f < fpp; ++f )
				{
					m_portBuffer[f][0] *= ( p <= 0 ? 1.0f : 1.0f - p ) * v;
					m_portBuffer[f][1] *= ( p >= 0 ? 1.0f : 1.0f + p ) * v;
				}
			}
		}

		// has vol model only
		else if( m_volumeModel )
		{
			ValueBuffer * volBuf = m_volumeModel->valueBuffer();

			if( volBuf )
			{
				for( f_cnt_t f = 0; f < fpp; ++f )
				{
					float v = volBuf->values()[ f ] * 0.01f;
					m_portBuffer[f][0] *= v;
					m_portBuffer[f][1] *= v;
				}
			}
			else
			{
				float v = m_volumeModel->value() * 0.01f;
				for( f_cnt_t f = 0; f < fpp; ++f )
				{
					m_portBuffer[f][0] *= v;
					m_portBuffer[f][1] *= v;
				}
			}
		}
	}
	// as of now there's no situation where we only have panning model but no volume model
	// if we have neither, we don't have to do anything here - just pass the audio as is

	// handle effects
	const bool me = processEffects();
	if( me || m_bufferUsage )
	{
		Engine::mixer()->mixToChannel(m_portBuffer.data(), m_nextMixerChannel); 	// send output to mixer
																			// TODO: improve the flow here - convert to pull model
		m_bufferUsage = false;
	}
}


void AudioPort::addPlayHandle( PlayHandle * handle )
{
	m_playHandleLock.lock();
		m_playHandles.append( handle );
	m_playHandleLock.unlock();
}


void AudioPort::removePlayHandle( PlayHandle * handle )
{
	m_playHandleLock.lock();
		PlayHandleList::Iterator it =	std::find( m_playHandles.begin(), m_playHandles.end(), handle );
		if( it != m_playHandles.end() )
		{
			m_playHandles.erase( it );
		}
	m_playHandleLock.unlock();
}

} // namespace lmms