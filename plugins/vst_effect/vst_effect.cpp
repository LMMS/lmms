/*
 * vst_effect.cpp - class for handling VST effect plugins
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef QT4

#include <QtGui/QMessageBox>

#else

#include <qmessagebox.h>

#endif


#include "vst_effect.h"
#include "vst_subplugin_features.h"
#include "song_editor.h"
#include "text_float.h"
#include "buffer_allocator.h"


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
	0x0100,
	plugin::Effect,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	new vstSubPluginFeatures( plugin::Effect )
} ;

}


vstEffect::vstEffect( effect::constructionData * _cdata ) :
	effect( &vsteffect_plugin_descriptor, _cdata ),
	m_plugin( NULL ),
	m_pluginMutex(),
	m_key( _cdata->key )
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




bool FASTCALL vstEffect::processAudioBuffer( surroundSampleFrame * _buf, 
							const fpab_t _frames )
{
	if( isBypassed() || !isRunning () )
	{
		return( FALSE );
	}

	if( m_plugin )
	{
		sampleFrame * buf = bufferAllocator::alloc<sampleFrame>(
								_frames );
		for( fpab_t f = 0; f < _frames; ++f )
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
		for( fpab_t f = 0; f < _frames; ++f )
		{
			for( ch_cnt_t ch = 0; ch < SURROUND_CHANNELS; ++ch )
			{
				_buf[f][ch] = getDryLevel() * _buf[f][ch] +
					getWetLevel() *
						buf[f][ch%DEFAULT_CHANNELS];
				out_sum += _buf[f][ch]*_buf[f][ch];
			}
		}
		bufferAllocator::free( buf );
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
	m_plugin = new remoteVSTPlugin( _plugin, eng() );
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
	m_plugin->showEditor();
	remoteVSTPlugin::connect( eng()->getSongEditor(),
				SIGNAL( tempoChanged( bpm_t ) ),
			 m_plugin, SLOT( setTempo( bpm_t ) ) );
	m_plugin->setTempo( eng()->getSongEditor()->getTempo() );
	if( m_plugin->pluginWidget() != NULL )
	{
/*#ifdef QT4
		m_plugin->pluginWidget()->setWindowIcon(
				getInstrumentTrack()->windowIcon() );
#else
		m_plugin->pluginWidget()->setWindowIcon(
				*( getInstrumentTrack()->windowIcon() ) );
#endif*/
		m_plugin->hideEditor();
	}
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
plugin * lmms_plugin_main( void * _data )
{
	return( new vstEffect(
			static_cast<effect::constructionData *>( _data ) ) );
}

}

