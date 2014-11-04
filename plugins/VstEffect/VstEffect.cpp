/*
 * VstEffect.cpp - class for handling VST effect plugins
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtGui/QMessageBox>

#include "VstEffect.h"
#include "song.h"
#include "text_float.h"
#include "VstSubPluginFeatures.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT vsteffect_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"VST",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"plugin for using arbitrary VST effects inside LMMS." ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0200,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	new VstSubPluginFeatures( Plugin::Effect )
} ;

}


VstEffect::VstEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &vsteffect_plugin_descriptor, _parent, _key ),
	m_plugin( NULL ),
	m_pluginMutex(),
	m_key( *_key ),
	m_vstControls( this )
{
	if( !m_key.attributes["file"].isEmpty() )
	{
		openPlugin( m_key.attributes["file"] );
	}
	setDisplayName( m_key.attributes["file"].section( ".dll", 0, 0 ).isEmpty()
		? m_key.name : m_key.attributes["file"].section( ".dll", 0, 0 ) );
}




VstEffect::~VstEffect()
{
	closePlugin();
}




bool VstEffect::processAudioBuffer( sampleFrame * _buf, const fpp_t _frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return false;
	}

	if( m_plugin )
	{
		const float d = dryLevel();
#ifdef __GNUC__
		sampleFrame buf[_frames];
#else
		sampleFrame * buf = new sampleFrame[_frames];
#endif
		memcpy( buf, _buf, sizeof( sampleFrame ) * _frames );
		m_pluginMutex.lock();
		m_plugin->process( buf, buf );
		m_pluginMutex.unlock();

		double out_sum = 0.0;
		const float w = wetLevel();
		for( fpp_t f = 0; f < _frames; ++f )
		{
			_buf[f][0] = w*buf[f][0] + d*_buf[f][0];
			_buf[f][1] = w*buf[f][1] + d*_buf[f][1];
		}
		for( fpp_t f = 0; f < _frames; ++f )
		{
			out_sum += _buf[f][0]*_buf[f][0] + _buf[f][1]*_buf[f][1];
		}
#ifndef __GNUC__
		delete[] buf;
#endif

		checkGate( out_sum / _frames );
	}
	return isRunning();
}




void VstEffect::openPlugin( const QString & _plugin )
{
	textFloat * tf = textFloat::displayMessage(
		VstPlugin::tr( "Loading plugin" ),
		VstPlugin::tr( "Please wait while loading VST plugin..." ),
			PLUGIN_NAME::getIconPixmap( "logo", 24, 24 ), 0 );
	m_pluginMutex.lock();
	m_plugin = new VstPlugin( _plugin );
	if( m_plugin->failed() )
	{
		m_pluginMutex.unlock();
		closePlugin();
		delete tf;
		QMessageBox::information( NULL,
			VstPlugin::tr( "Failed loading VST plugin" ),
			VstPlugin::tr( "The VST plugin %1 could not be loaded for some reason." ).arg( _plugin ),
			QMessageBox::Ok );
		return;
	}

	VstPlugin::connect( engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ), m_plugin, SLOT( setTempo( bpm_t ) ) );
	m_plugin->setTempo( engine::getSong()->getTempo() );

	m_pluginMutex.unlock();

	delete tf;

	m_key.attributes["file"] = _plugin;
}



void VstEffect::closePlugin()
{
	m_pluginMutex.lock();
	if( m_plugin && m_plugin->pluginWidget() != NULL )
	{
		delete m_plugin->pluginWidget();
	}
	delete m_plugin;
	m_plugin = NULL;
	m_pluginMutex.unlock();
}





extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model * _parent, void * _data )
{
	return new VstEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) );
}

}

