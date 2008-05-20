/*
 * vst_effect.cpp - class for handling VST effect plugins
 *
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


#include "vst_effect.h"

#include <QtGui/QMessageBox>

#include "song.h"
#include "text_float.h"
#include "vst_subplugin_features.h"


#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor vsteffect_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"VST Effect",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"plugin for using arbitrary VST-effects "
				"inside LMMS." ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0200,
	plugin::Effect,
	new pluginPixmapLoader( "logo" ),
	new vstSubPluginFeatures( plugin::Effect )
} ;

}


vstEffect::vstEffect( model * _parent,
			const descriptor::subPluginFeatures::key * _key ) :
	effect( &vsteffect_plugin_descriptor, _parent, _key ),
	m_plugin( NULL ),
	m_pluginMutex(),
	m_key( *_key ),
	m_vstControls( this )
{
	if( !m_key.user.toString().isEmpty() )
	{
		openPlugin( m_key.user.toString() );
	}
}




vstEffect::~vstEffect()
{
	closePlugin();
}




bool vstEffect::processAudioBuffer( sampleFrame * _buf, const fpp_t _frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( FALSE );
	}

	if( m_plugin )
	{
		sampleFrame * buf = new sampleFrame[_frames];
		for( fpp_t f = 0; f < _frames; ++f )
		{
			for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				buf[f][ch] = _buf[f][ch];
			}
		}
		m_pluginMutex.lock();
		m_plugin->process( buf, buf, TRUE );
		m_pluginMutex.unlock();

		double out_sum = 0.0;
		const float d = getDryLevel();
		const float w = getWetLevel();
		for( fpp_t f = 0; f < _frames; ++f )
		{
			_buf[f][0] = d * _buf[f][0] + w * buf[f][0];
			_buf[f][1] = d * _buf[f][1] + w * buf[f][1];
			out_sum += _buf[f][0]*_buf[f][0] + _buf[f][1]*_buf[f][1];
		}
		delete[] buf;

		checkGate( out_sum / _frames );
	}
	return( isRunning() );
}




void vstEffect::openPlugin( const QString & _plugin )
{
	textFloat * tf = textFloat::displayMessage(
		remoteVSTPlugin::tr( "Loading plugin" ),
		remoteVSTPlugin::tr(
				"Please wait while loading VST-plugin..." ),
			PLUGIN_NAME::getIconPixmap( "logo", 24, 24 ), 0 );
	m_pluginMutex.lock();
	m_plugin = new remoteVSTPlugin( _plugin );
	if( m_plugin->failed() )
	{
		m_pluginMutex.unlock();
		closePlugin();
		delete tf;
		QMessageBox::information( NULL,
			remoteVSTPlugin::tr( "Failed loading VST-plugin" ),
			remoteVSTPlugin::tr( "The VST-plugin %1 could not "
					"be loaded for some reason.\n"
					"If it runs with other VST-"
					"software under Linux, please "
					"contact an LMMS-developer!"
					).arg( _plugin ),
						QMessageBox::Ok );
		return;
	}
	remoteVSTPlugin::connect( engine::getSong(),
				SIGNAL( tempoChanged( bpm_t ) ),
			 m_plugin, SLOT( setTempo( bpm_t ) ) );
	m_plugin->setTempo( engine::getSong()->getTempo() );
	m_pluginMutex.unlock();
	delete tf;
}



void vstEffect::closePlugin( void )
{
	m_pluginMutex.lock();
	delete m_plugin;
	m_plugin = NULL;
	m_pluginMutex.unlock();
}





extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( model * _parent, void * _data )
{
	return( new vstEffect( _parent,
		static_cast<const plugin::descriptor::subPluginFeatures::key *>(
								_data ) ) );
}

}

