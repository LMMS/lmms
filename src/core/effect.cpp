#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect.cpp - class for processing LADSPA effects
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include "qmessagebox.h"

#include "effect.h"
#include "mixer.h"
#include "config_mgr.h"
#include "buffer_allocator.h"
#include "audio_device.h"
#include "ladspa_control.h"


effect::effect( const ladspa_key_t & _key, engine * _engine ) :
	engineObject( _engine ),
	m_key( _key ),
	m_ladspa( _engine->getLADSPAManager() ),
	m_output( NULL ),
	m_okay( TRUE ),
	m_noRun( FALSE ),
	m_running( FALSE ),
	m_bypass( FALSE ),
	m_bufferCount( 0 ),
	m_silenceTimeout( 10 ),
	m_wetDry( 1.0f ),
	m_gate( 0.0f )
{
	if( m_ladspa->getDescription( _key ) == NULL )
	{
		QMessageBox::warning( 0, "Effect", "Uknown LADSPA plugin requested: " + _key.first, QMessageBox::Ok, QMessageBox::NoButton );
		m_okay = FALSE;
		return;
	}
	
	m_name = m_ladspa->getShortName( _key );
	
	// Calculate how many processing units are needed.
	ch_cnt_t lmms_chnls = eng()->getMixer()->audioDev()->channels();
	m_effectChannels = m_ladspa->getDescription( _key )->inputChannels;
	m_processors = lmms_chnls / m_effectChannels;
	
	// Categorize the ports, and create the buffers.
	m_bufferSize =  eng()->getMixer()->framesPerAudioBuffer();
	m_portCount = m_ladspa->getPortCount( _key );
	
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		multi_proc_t ports;
		for( Uint16 port = 0; port < m_portCount; port++ )
		{
			port_desc_t * p = new portDescription;
			
			p->name = m_ladspa->getPortName( _key, port );
			p->proc = proc;
			p->port_id = port;
			
			// Determine the port's category.
			if( m_ladspa->isPortAudio( _key, port ) )
			{
				// Nasty manual memory management--was having difficulty
				// with some prepackaged plugins that were segfaulting
				// during cleanup.  It was easier to troubleshoot with the
				// memory management all taking place in one file.
				p->buffer = new LADSPA_Data[m_bufferSize];
				
				if( p->name.upper().contains( "IN" ) && m_ladspa->isPortInput( _key, port ) )
				{
					p->rate = CHANNEL_IN;
				}
				else if( p->name.upper().contains( "OUT" ) && m_ladspa->isPortOutput( _key, port ) )
				{
					p->rate = CHANNEL_OUT;
				}
				else if( m_ladspa->isPortInput( _key, port ) )
				{
					p->rate = AUDIO_RATE_INPUT;
				}
				else
				{
					p->rate = AUDIO_RATE_OUTPUT;
				}
			}
			else
			{
				p->buffer = new LADSPA_Data;
				
				if( m_ladspa->isPortInput( _key, port ) )
				{
					p->rate = CONTROL_RATE_INPUT;
				}
				else
				{
					p->rate = CONTROL_RATE_OUTPUT;
				}
			}
			
			if( m_ladspa->isPortToggled( _key, port ) )
			{
				p->data_type = TOGGLED;
			}
			else if( m_ladspa->isInteger( _key, port ) )
			{
				p->data_type = INTEGER;
			}
			else
			{
				p->data_type = FLOAT;
			}
			
			// Get the range and default values.
			p->is_scaled = m_ladspa->areHintsSampleRateDependent( _key, port );
			p->max = m_ladspa->getUpperBound( _key, port );
			if( p->max == NOHINT )
			{
				p->max = 999999.0f;
			}
			else if( p->is_scaled )
			{
				p->max *= eng()->getMixer()->sampleRate();
			}
			
			p->min = m_ladspa->getLowerBound( _key, port );
			if( p->min == NOHINT )
			{
				p->min = -999999.0f;
			}
			else if( p->is_scaled )
			{
				p->min *= eng()->getMixer()->sampleRate();
			}
				
			p->def = m_ladspa->getDefaultSetting( _key, port );
			if( p->def == NOHINT )
			{
				if( p->data_type != TOGGLED )
				{
					p->def = ( p->min + p->max ) / 2.0f;
				}
				else
				{
					p->def = 1.0f;
				}
			}
			
			p->value = p->def;
			
			
			ports.append( p );
			
			// For convenience, keep a separate list of the ports that are used 
			// to control the processors.
			if( p->rate == AUDIO_RATE_INPUT || p->rate == CONTROL_RATE_INPUT )
			{
				p->control_id = m_controls.count();
				m_controls.append( p );
			}
		}
		m_ports.append( ports );
	}
	
	// Instantiate the processing units.
	m_descriptor = m_ladspa->getDescriptor( _key );
	if( m_descriptor == NULL )
	{
		QMessageBox::warning( 0, "Effect", "Can't get LADSPA descriptor function: " + _key.first, QMessageBox::Ok, QMessageBox::NoButton );
		m_okay = FALSE;
		return;
	}
	if( m_descriptor->run == NULL )
	{
		QMessageBox::warning( 0, "Effect", "Plugin has no processor: " + _key.first, QMessageBox::Ok, QMessageBox::NoButton );
		m_noRun = TRUE;
	}
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		LADSPA_Handle effect = m_ladspa->instantiate( _key, eng()->getMixer()->sampleRate() );
		if( effect == NULL )
		{
			QMessageBox::warning( 0, "Effect", "Can't get LADSPA instance: " + _key.first, QMessageBox::Ok, QMessageBox::NoButton );
			m_okay = FALSE;
			return;
		}
		m_handles.append( effect );
		
	}
	
	// Connect the ports.
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		for( Uint16 port = 0; port < m_portCount; port++ )
		{
			if( !m_ladspa->connectPort( _key, m_handles[proc], port, m_ports[proc][port]->buffer ) )
			{
				QMessageBox::warning( 0, "Effect", "Failed to connect port: " + _key.first, QMessageBox::Ok, QMessageBox::NoButton );
				m_noRun = TRUE;
			}
		}
	}
	
	// Activate the processing units.
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		m_ladspa->activate( _key, m_handles[proc] );
	}
}




effect::~effect()
{
	if( !m_okay )
	{
		return;
	}
	
	m_processLock.lock();
	
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		m_ladspa->deactivate( m_key, m_handles[proc] );
		m_ladspa->cleanup( m_key, m_handles[proc] );
		for( Uint16 port = 0; port < m_portCount; port++ )
		{
			free( m_ports[proc][port]->buffer );
			free( m_ports[proc][port] );
		}
		m_ports[proc].clear();
	}
	m_ports.clear();
	m_handles.clear();
	
	m_processLock.unlock();
}




bool FASTCALL effect::processAudioBuffer( surroundSampleFrame * _buf, const fpab_t _frames )
{
	if( !m_okay || m_noRun || !m_running || m_bypass || !m_processLock.tryLock() )
	{
		return( FALSE );
	}
	
	// Copy the LMMS audio buffer to the LADSPA input buffer and initialize
	// the control ports.  Need to change this to handle non-in-place-broken
	// plugins--would speed things up to use the same buffer for both
	// LMMS and LADSPA.
	ch_cnt_t channel = 0;
	for( ch_cnt_t proc = 0; proc < m_processors; proc++)
	{
		for( Uint16 port = 0; port < m_portCount; port++ )
		{
			switch( m_ports[proc][port]->rate )
			{
				case CHANNEL_IN:
					for( fpab_t frame = 0; frame < _frames; frame++ )
					{
						m_ports[proc][port]->buffer[frame] = _buf[frame][channel];
					}
					channel++;
					break;
				case AUDIO_RATE_INPUT:
					m_ports[proc][port]->value = static_cast<LADSPA_Data>( m_ports[proc][port]->control->getValue() );
					// This only supports control rate ports, so the audio rates are
					// treated as though they were control rate by setting the
					// port buffer to all the same value.
					for( fpab_t frame = 0; frame < _frames; frame++ )
					{
						m_ports[proc][port]->buffer[frame] = m_ports[proc][port]->value;
					}
					break;
				case CONTROL_RATE_INPUT:
					m_ports[proc][port]->value = static_cast<LADSPA_Data>( m_ports[proc][port]->control->getValue() );
					m_ports[proc][port]->buffer[0] = m_ports[proc][port]->value;
					break;
				case CHANNEL_OUT:
				case AUDIO_RATE_OUTPUT:
				case CONTROL_RATE_OUTPUT:
					break;
				default:
					break;
			}
		}
	}
	
	// Process the buffers.
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		(m_descriptor->run)(m_handles[proc], _frames);
	}
	
	// Copy the LADSPA output buffers to the LMMS buffer.
	double out_sum = 0.0;
	channel = 0;
	for( ch_cnt_t proc = 0; proc < m_processors; proc++)
	{
		for( Uint16 port = 0; port < m_portCount; port++ )
		{
			switch( m_ports[proc][port]->rate )
			{
				case CHANNEL_IN:
				case AUDIO_RATE_INPUT:
				case CONTROL_RATE_INPUT:
					break;
				case CHANNEL_OUT:
					for( fpab_t frame = 0; frame < _frames; frame++ )
					{
						_buf[frame][channel] = ( 1.0f - m_wetDry ) * _buf[frame][channel] + m_wetDry * m_ports[proc][port]->buffer[frame];
						out_sum += _buf[frame][channel] * _buf[frame][channel];
					}
					channel++;
					break;
				case AUDIO_RATE_OUTPUT:
				case CONTROL_RATE_OUTPUT:
					break;
				default:
					break;
			}
		}
	}
	
	// Check whether we need to continue processing input.  Restart the
	// counter if the threshold has been exceeded.
	if( out_sum <= ( m_gate ) )
	{
		m_bufferCount++;
		if( m_bufferCount > m_silenceTimeout )
		{
			m_running = FALSE;
			m_bufferCount = 0;
		}
	}
	else
	{
		m_bufferCount = 0;
	}
	
	m_processLock.unlock();
	return( m_running );
}



void FASTCALL effect::setControl( Uint16 _control, LADSPA_Data _value )
{
	if( !m_okay )
	{
		return;
	}
	m_processLock.lock();
	m_controls[_control]->value = _value;
	m_processLock.unlock();
}




void FASTCALL effect::setGate( float _level )
{
	m_processLock.lock();
	m_gate = _level * _level * m_processors * m_bufferSize;
	m_processLock.unlock();
}



#endif

#endif
