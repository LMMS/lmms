/*
 * ladspa_effect.cpp - class for processing LADSPA effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QMessageBox>

#else

#include "qmessagebox.h"
#define indexOf find

#endif


#include "ladspa_effect.h"
#include "mixer.h"
#include "config_mgr.h"
#include "buffer_allocator.h"
#include "audio_device.h"
#include "ladspa_control.h"
#include "ladspa_subplugin_features.h"


#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor ladspaeffect_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"LADSPA Effect",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"plugin for using arbitrary LADSPA-effects "
				"inside LMMS." ),
	"Danny McRae <khjklujn/at/users.sourceforge.net>",
	0x0100,
	plugin::Effect,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	new ladspaSubPluginFeatures( plugin::Effect )
} ;

}


ladspaEffect::ladspaEffect( const descriptor::subPluginFeatures::key * _key ) :
	effect( &ladspaeffect_plugin_descriptor, _key ),
	m_effName( "none" ),
	m_key( subPluginKeyToLadspaKey( _key )
		/* ladspa_key_t( _cdata->settings.attribute( "label" ),
				_cdata->settings.attribute( "lib" ) )*/ ),
	m_ladspa( engine::getLADSPAManager() )
{
	if( m_ladspa->getDescription( m_key ) == NULL )
	{
		QMessageBox::warning( 0, "Effect", 
			"Unknown LADSPA plugin requested: " + m_key.first, 
			QMessageBox::Ok, QMessageBox::NoButton );
		setOkay( FALSE );
		return;
	}
	
	setPublicName( m_ladspa->getShortName( m_key ) );
	
	// Calculate how many processing units are needed.
	const ch_cnt_t lmms_chnls = engine::getMixer()->audioDev()->channels();
	m_effectChannels = m_ladspa->getDescription( m_key )->inputChannels;
	setProcessorCount( lmms_chnls / m_effectChannels );
	
	// Categorize the ports, and create the buffers.
	m_portCount = m_ladspa->getPortCount( m_key );
	
	for( ch_cnt_t proc = 0; proc < getProcessorCount(); proc++ )
	{
		multi_proc_t ports;
		for( Uint16 port = 0; port < m_portCount; port++ )
		{
			port_desc_t * p = new portDescription;
			
			p->name = m_ladspa->getPortName( m_key, port );
			p->proc = proc;
			p->port_id = port;
			p->control = NULL;
			
			// Determine the port's category.
			if( m_ladspa->isPortAudio( m_key, port ) )
			{
		// Nasty manual memory management--was having difficulty
		// with some prepackaged plugins that were segfaulting
		// during cleanup.  It was easier to troubleshoot with the
		// memory management all taking place in one file.
				p->buffer = 
		new LADSPA_Data[engine::getMixer()->framesPerAudioBuffer()];
				
				if( p->name.toUpper().contains( "IN" ) && 
					m_ladspa->isPortInput( m_key, port ) )
				{
					p->rate = CHANNEL_IN;
				}
				else if( p->name.toUpper().contains( "OUT" ) && 
					m_ladspa->isPortOutput( m_key, port ) )
				{
					p->rate = CHANNEL_OUT;
				}
				else if( m_ladspa->isPortInput( m_key, port ) )
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
				
				if( m_ladspa->isPortInput( m_key, port ) )
				{
					p->rate = CONTROL_RATE_INPUT;
				}
				else
				{
					p->rate = CONTROL_RATE_OUTPUT;
				}
			}
			
			p->scale = 1.0f;
			if( m_ladspa->isPortToggled( m_key, port ) )
			{
				p->data_type = TOGGLED;
			}
			else if( m_ladspa->isInteger( m_key, port ) )
			{
				p->data_type = INTEGER;
			}
			else if( p->name.toUpper().contains( "(SECONDS)" ) )
			{
				p->data_type = TIME;
				p->scale = 1000.0f;
				int loc = p->name.toUpper().indexOf(
								"(SECONDS)" );
				p->name.replace( loc, 9, "(ms)" );
			}
			else if( p->name.toUpper().contains( "(S)" ) )
			{
				p->data_type = TIME;
				p->scale = 1000.0f;
				int loc = p->name.toUpper().indexOf( "(S)" );
				p->name.replace( loc, 3, "(ms)" );
			}
			else if( p->name.toUpper().contains( "(MS)" ) )
			{
				p->data_type = TIME;
				int loc = p->name.toUpper().indexOf( "(MS)" );
				p->name.replace( loc, 4, "(ms)" );
			}
			else
			{
				p->data_type = FLOAT;
			}
			
			// Get the range and default values.
			p->max = m_ladspa->getUpperBound( m_key, port );
			if( p->max == NOHINT )
			{
				p->max = p->name.toUpper() == "GAIN" ? 10.0f :
					1.0f;
			}
			
			if( m_ladspa->areHintsSampleRateDependent( 
								m_key, port ) )
			{
				p->max *= engine::getMixer()->sampleRate();
			}
			
			p->min = m_ladspa->getLowerBound( m_key, port );
			if( p->min == NOHINT )
			{
				p->min = 0.0f;
			}
			
			if( m_ladspa->areHintsSampleRateDependent( 
								m_key, port ) )
			{
				p->min *= engine::getMixer()->sampleRate();
			}
				
			p->def = m_ladspa->getDefaultSetting( m_key, port );
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
			
			p->max *= p->scale;
			p->min *= p->scale;
			p->def *= p->scale;
			
			p->value = p->def;
			
			
			ports.append( p );
			
	// For convenience, keep a separate list of the ports that are used 
	// to control the processors.
			if( p->rate == AUDIO_RATE_INPUT || 
					p->rate == CONTROL_RATE_INPUT )
			{
				p->control_id = m_controls.count();
				m_controls.append( p );
			}
		}
		m_ports.append( ports );
	}
	
	// Instantiate the processing units.
	m_descriptor = m_ladspa->getDescriptor( m_key );
	if( m_descriptor == NULL )
	{
		QMessageBox::warning( 0, "Effect", 
			"Can't get LADSPA descriptor function: " + m_key.first,
			QMessageBox::Ok, QMessageBox::NoButton );
		setOkay( FALSE );
		return;
	}
	if( m_descriptor->run == NULL )
	{
		QMessageBox::warning( 0, "Effect", 
			"Plugin has no processor: " + m_key.first,
			QMessageBox::Ok, QMessageBox::NoButton );
		setDontRun( TRUE );
	}
	for( ch_cnt_t proc = 0; proc < getProcessorCount(); proc++ )
	{
		LADSPA_Handle effect = m_ladspa->instantiate( m_key,
					engine::getMixer()->sampleRate() );
		if( effect == NULL )
		{
			QMessageBox::warning( 0, "Effect", 
				"Can't get LADSPA instance: " + m_key.first,
				QMessageBox::Ok, QMessageBox::NoButton );
			setOkay( FALSE );
			return;
		}
		m_handles.append( effect );
		
	}
	
	// Connect the ports.
	for( ch_cnt_t proc = 0; proc < getProcessorCount(); proc++ )
	{
		for( Uint16 port = 0; port < m_portCount; port++ )
		{
			if( !m_ladspa->connectPort( m_key, 
			     			m_handles[proc], 
						port, 
						m_ports[proc][port]->buffer ) )
			{
				QMessageBox::warning( 0, "Effect", 
				"Failed to connect port: " + m_key.first, 
				QMessageBox::Ok, QMessageBox::NoButton );
				setDontRun( TRUE );
				return;
			}
		}
	}
	
	// Activate the processing units.
	for( ch_cnt_t proc = 0; proc < getProcessorCount(); proc++ )
	{
		m_ladspa->activate( m_key, m_handles[proc] );
	}
}




ladspaEffect::~ladspaEffect()
{
	if( !isOkay() )
	{
		return;
	}
	
	lock();
	
	for( ch_cnt_t proc = 0; proc < getProcessorCount(); proc++ )
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
	
	unlock();
}




bool FASTCALL ladspaEffect::processAudioBuffer( surroundSampleFrame * _buf, 
							const fpab_t _frames )
{
	if( !isOkay() || dontRun() || !isRunning() || 
					isBypassed() || !tryLock() )
	{
		return( FALSE );
	}
	
	// Copy the LMMS audio buffer to the LADSPA input buffer and initialize
	// the control ports.  Need to change this to handle non-in-place-broken
	// plugins--would speed things up to use the same buffer for both
	// LMMS and LADSPA.
	ch_cnt_t channel = 0;
	for( ch_cnt_t proc = 0; proc < getProcessorCount(); proc++)
	{
		for( Uint16 port = 0; port < m_portCount; port++ )
		{
			switch( m_ports[proc][port]->rate )
			{
				case CHANNEL_IN:
					for( fpab_t frame = 0; 
						frame < _frames; frame++ )
					{
						m_ports[proc][port]->buffer[frame] = 
							_buf[frame][channel];
					}
					channel++;
					break;
				case AUDIO_RATE_INPUT:
					m_ports[proc][port]->value = 
						static_cast<LADSPA_Data>( 
							m_ports[proc][port]->control->getValue() /
								m_ports[proc][port]->scale );
					// This only supports control rate ports, so the audio rates are
					// treated as though they were control rate by setting the
					// port buffer to all the same value.
					for( fpab_t frame = 0; 
						frame < _frames; frame++ )
					{
						m_ports[proc][port]->buffer[frame] = 
							m_ports[proc][port]->value;
					}
					break;
				case CONTROL_RATE_INPUT:
					if( m_ports[proc][port]->control ==
									NULL )
					{
						break;
					}
					m_ports[proc][port]->value = 
						static_cast<LADSPA_Data>( 
							m_ports[proc][port]->control->getValue() /
								m_ports[proc][port]->scale );
					m_ports[proc][port]->buffer[0] = 
						m_ports[proc][port]->value;
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
	for( ch_cnt_t proc = 0; proc < getProcessorCount(); proc++ )
	{
		(m_descriptor->run)(m_handles[proc], _frames);
	}
	
	// Copy the LADSPA output buffers to the LMMS buffer.
	double out_sum = 0.0;
	channel = 0;
	for( ch_cnt_t proc = 0; proc < getProcessorCount(); proc++)
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
					for( fpab_t frame = 0; 
						frame < _frames; frame++ )
					{
						_buf[frame][channel] = 
							getDryLevel() * 
							_buf[frame][channel] +
							getWetLevel() *
							m_ports[proc][port]->buffer[frame];
						out_sum += 
							_buf[frame][channel] *
							_buf[frame][channel];
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
	if( out_sum <= getGate() )
	{
		incrementBufferCount();
		if( getBufferCount() > getTimeout() )
		{
			stopRunning();
			resetBufferCount();
		}
	}
	else
	{
		resetBufferCount();
	}
	
	unlock();
	return( isRunning() );
}




void FASTCALL ladspaEffect::setControl( Uint16 _control, LADSPA_Data _value )
{
	if( !isOkay() )
	{
		return;
	}
	lock();
	m_controls[_control]->value = _value;
	unlock();
}


#undef indexOf

extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new ladspaEffect(
		static_cast<const plugin::descriptor::subPluginFeatures::key *>(
								_data ) ) );
}

}

