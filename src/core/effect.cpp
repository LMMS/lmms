#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect.cpp - base-class for effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtXml/QDomElement>

#include <cstdio>

#include "effect.h"
#include "engine.h"
#include "dummy_effect.h"
#include "effect_chain.h"
#include "effect_view.h"


effect::effect( const plugin::descriptor * _desc,
			model * _parent,
			const descriptor::subPluginFeatures::key * _key ) :
	plugin( _desc, _parent ),
	m_key( _key ? *_key : descriptor::subPluginFeatures::key()  ),
	m_processors( 1 ),
	m_okay( TRUE ),
	m_noRun( FALSE ),
	m_running( FALSE ),
	m_bufferCount( 0 ),
	m_enabledModel( TRUE, this, tr( "Effect enabled" ) ),
	m_wetDryModel( 1.0f, -1.0f, 1.0f, 0.01f, this, tr( "Wet/Dry mix" ) ),
	m_gateModel( 0.0f, 0.0f, 1.0f, 0.01f, this, tr( "Gate" ) ),
	m_autoQuitModel( 1.0f, 1.0f, 8000.0f, 100.0f, 1.0f, this, tr( "Decay" ) )
{
	m_srcState[0] = m_srcState[1] = NULL;
	reinitSRC();
}




effect::~effect()
{
	for( int i = 0; i < 2; ++i )
	{
		if( m_srcState[i] != NULL )
		{
			src_delete( m_srcState[i] );
		}
	}
}




void effect::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "on", m_enabledModel.value() );
	_this.setAttribute( "wet", m_wetDryModel.value() );
	_this.setAttribute( "autoquit", m_autoQuitModel.value() );
	_this.setAttribute( "gate", m_gateModel.value() );
	getControls()->saveState( _doc, _this );
}




void effect::loadSettings( const QDomElement & _this )
{
	m_enabledModel.setValue( (float) _this.attribute( "on" ).toInt() );
	m_wetDryModel.setValue( _this.attribute( "wet" ).toFloat() );
	m_autoQuitModel.setValue( _this.attribute( "autoquit" ).toFloat() );
	m_gateModel.setValue( _this.attribute( "gate" ).toFloat() );

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( getControls()->nodeName() == node.nodeName() )
			{
				getControls()->restoreState( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
}





effect * effect::instantiate( const QString & _plugin_name,
				model * _parent,
				descriptor::subPluginFeatures::key * _key )
{
	plugin * p = plugin::instantiate( _plugin_name, _parent, _key );
	// check whether instantiated plugin is an effect
	if( dynamic_cast<effect *>( p ) != NULL )
	{
		// everything ok, so return pointer
		return( dynamic_cast<effect *>( p ) );
	}

	// not quite... so delete plugin and return dummy effect
	delete p;
	return( new dummyEffect( _parent ) );
}




void effect::checkGate( double _out_sum )
{
	// Check whether we need to continue processing input.  Restart the
	// counter if the threshold has been exceeded.
	if( _out_sum <= getGate()+0.000001 )
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
}




pluginView * effect::instantiateView( QWidget * _parent )
{
	return( new effectView( this, _parent ) );
}

	


void effect::reinitSRC( void )
{
	for( int i = 0; i < 2; ++i )
	{
		if( m_srcState[i] != NULL )
		{
			src_delete( m_srcState[i] );
		}
		int error;
		if( ( m_srcState[i] = src_new(
			engine::getMixer()->currentQualitySettings().
							libsrcInterpolation(),
					DEFAULT_CHANNELS, &error ) ) == NULL )
		{
			fprintf( stderr, "Error: src_new() failed in effect.cpp!\n" );
		}
	}
}




void effect::resample( int _i, const sampleFrame * _src_buf,
							sample_rate_t _src_sr,
				sampleFrame * _dst_buf, sample_rate_t _dst_sr,
								f_cnt_t _frames )
{
	if( m_srcState[_i] == NULL )
	{
		return;
	}
	m_srcData[_i].input_frames = _frames;
	m_srcData[_i].output_frames = engine::getMixer()->framesPerPeriod();
	m_srcData[_i].data_in = (float *) _src_buf[0];
	m_srcData[_i].data_out = _dst_buf[0];
	m_srcData[_i].src_ratio = (double) _dst_sr / _src_sr;
	m_srcData[_i].end_of_input = 0;
	int error;
	if( ( error = src_process( m_srcState[_i], &m_srcData[_i] ) ) )
	{
		fprintf( stderr, "effect::resample(): error while resampling: %s\n",
							src_strerror( error ) );
	}
}


#endif
