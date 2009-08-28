/*
 * ZynAddSubFx.cpp - ZynAddSubxFX-embedding plugin
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "lmmsconfig.h"

#include <Qt/QtXml>
#include <QtCore/QTemporaryFile>
#include <QtGui/QDropEvent>
#include <QtGui/QPushButton>

#include "ResourceFileMapper.h"

#include "ZynAddSubFx.h"
#include "engine.h"
#include "mmp.h"
#include "instrument_play_handle.h"
#include "InstrumentTrack.h"
#include "gui_templates.h"
#include "string_pair_drag.h"
#include "RemoteZynAddSubFx.h"
#include "LocalZynAddSubFx.h"

#include "embed.cpp"
#include "moc_ZynAddSubFx.cxx"


static const char * __supportedExts[] =
{ "xiz", NULL };

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT ZynAddSubFx_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"ZynAddSubFX",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Embedded ZynAddSubFX" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	__supportedExts,
	NULL,
} ;

}



ZynAddSubFxInstrument::ZynAddSubFxInstrument(
									InstrumentTrack * _instrumentTrack ) :
	Instrument( _instrumentTrack, &ZynAddSubFx_plugin_descriptor ),
	m_hasGUI( false ),
	m_plugin( NULL ),
	m_remotePlugin( NULL )
{
	initPlugin();

	// now we need a play-handle which cares for calling play()
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
	engine::getMixer()->addPlayHandle( iph );

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( reloadPlugin() ) );
}




ZynAddSubFxInstrument::~ZynAddSubFxInstrument()
{
	engine::getMixer()->removePlayHandles( instrumentTrack() );

	m_pluginMutex.lock();
	delete m_remotePlugin;
	m_pluginMutex.unlock();
}




void ZynAddSubFxInstrument::saveSettings( QDomDocument & _doc,
	                             QDomElement & _this )
{
	QTemporaryFile tf;
	if( tf.open() )
	{
		const std::string fn = QSTR_TO_STDSTR(
									QDir::toNativeSeparators( tf.fileName() ) );
		m_pluginMutex.lock();
		if( m_remotePlugin )
		{
			m_remotePlugin->lock();
			m_remotePlugin->sendMessage(
				RemotePlugin::message( IdSaveSettingsToFile ).addString( fn ) );
			m_remotePlugin->waitForMessage( IdSaveSettingsToFile );
			m_remotePlugin->unlock();
		}
		else
		{
			m_plugin->saveXML( fn );
		}
		m_pluginMutex.unlock();
		QByteArray a = tf.readAll();
		// remove first blank line
		a.remove( 0,
#ifdef LMMS_BUILD_WIN32
				2
#else
				1
#endif
					);
		QDomDocument doc( "mydoc" );
		doc.setContent( a );
		_this.appendChild( doc.documentElement() );
	}
}




void ZynAddSubFxInstrument::loadSettings( const QDomElement & _this )
{
	if( !_this.hasChildNodes() )
	{
		return;
	}

	QDomDocument doc;
	doc.appendChild( doc.importNode( _this.firstChild(), true ) );
	QTemporaryFile tf;
	tf.setAutoRemove( false );
	if( tf.open() )
	{
		QByteArray a = doc.toString( 0 ).toUtf8();
		a.prepend( "<?xml version=\"1.0\"?>\n" );
		tf.write( a );

		const std::string fn = QSTR_TO_STDSTR(
									QDir::toNativeSeparators( tf.fileName() ) );
		m_pluginMutex.lock();
		if( m_remotePlugin )
		{
			m_remotePlugin->lock();
			m_remotePlugin->sendMessage(
				RemotePlugin::message( IdLoadSettingsFromFile ).
															addString( fn ) );
			m_remotePlugin->waitForMessage( IdLoadSettingsFromFile );
			m_remotePlugin->unlock();
		}
		else
		{
			m_plugin->loadXML( fn );
		}
		m_pluginMutex.unlock();

		emit settingsChanged();
	}
}




void ZynAddSubFxInstrument::loadResource( const ResourceItem * _item )
{
	ResourceFileMapper mapper( _item );
	const std::string fn = QSTR_TO_STDSTR( mapper.fileName() );
	if( m_remotePlugin )
	{
		m_remotePlugin->lock();
		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdLoadPresetFromFile ).
				addString( QSTR_TO_STDSTR( mapper.fileName() ) ) );
		m_remotePlugin->waitForMessage( IdLoadPresetFromFile );
		m_remotePlugin->unlock();
	}
	else
	{
		m_pluginMutex.lock();
		m_plugin->loadPreset( fn );
		m_pluginMutex.unlock();
	}

	emit settingsChanged();
}




QString ZynAddSubFxInstrument::nodeName() const
{
	return ZynAddSubFx_plugin_descriptor.name;
}




void ZynAddSubFxInstrument::play( sampleFrame * _buf )
{
	m_pluginMutex.lock();
	if( m_remotePlugin )
	{
		m_remotePlugin->process( NULL, _buf );
	}
	else
	{
		m_plugin->processAudio( _buf );
	}
	m_pluginMutex.unlock();
	instrumentTrack()->processAudioBuffer( _buf,
				engine::getMixer()->framesPerPeriod(), NULL );
}




bool ZynAddSubFxInstrument::handleMidiEvent( const midiEvent & _me,
                                                const midiTime & _time )
{
	m_pluginMutex.lock();
	if( m_remotePlugin )
	{
		m_remotePlugin->processMidiEvent( _me, 0 );
	}
	else
	{
		m_plugin->processMidiEvent( _me );
	}
	m_pluginMutex.unlock();

	return true;
}




void ZynAddSubFxInstrument::reloadPlugin()
{
	// save state of current plugin instance
	multimediaProject m( multimediaProject::InstrumentTrackSettings );
	saveSettings( m, m.content() );

	// init plugin (will delete current one and create a new instance)
	initPlugin();

	// and load the settings again
	loadSettings( m.content() );
}




void ZynAddSubFxInstrument::initPlugin()
{
	m_pluginMutex.lock();
	delete m_plugin;
	delete m_remotePlugin;
	m_plugin = NULL;
	m_remotePlugin = NULL;

	if( m_hasGUI )
	{
		m_remotePlugin = new RemotePlugin( "RemoteZynAddSubFx", false );
		m_remotePlugin->lock();
		m_remotePlugin->waitForInitDone( false );

		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdZasfPresetDirectory ).
				addString(
					QSTR_TO_STDSTR(
						QString( configManager::inst()->factoryPresetsDir() +
								QDir::separator() + "ZynAddSubFX" ) ) ) );
		m_remotePlugin->showUI();
		m_remotePlugin->unlock();
	}
	else
	{
		m_plugin = new LocalZynAddSubFx;
		m_plugin->setSampleRate( engine::getMixer()->processingSampleRate() );
		m_plugin->setBufferSize( engine::getMixer()->framesPerPeriod() );
	}
	m_pluginMutex.unlock();
}




PluginView * ZynAddSubFxInstrument::instantiateView( QWidget * _parent )
{
	return new ZynAddSubFxView( this, _parent );
}







ZynAddSubFxView::ZynAddSubFxView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	m_toggleUIButton = new QPushButton( tr( "Show GUI" ), this );
	m_toggleUIButton->setCheckable( true );
	m_toggleUIButton->setChecked( false );
	m_toggleUIButton->setGeometry( 45, 80, 160, 24 );
	m_toggleUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	m_toggleUIButton->setFont( pointSize<8>( m_toggleUIButton->font() ) );
	connect( m_toggleUIButton, SIGNAL( toggled( bool ) ), this,
							SLOT( toggleUI() ) );
	m_toggleUIButton->setWhatsThis(
		tr( "Click here to show or hide the graphical user interface "
			"(GUI) of ZynAddSubFX." ) );
}




ZynAddSubFxView::~ZynAddSubFxView()
{
}




void ZynAddSubFxView::modelChanged()
{
	toggleUI();
}




void ZynAddSubFxView::toggleUI()
{
	ZynAddSubFxInstrument * model = castModel<ZynAddSubFxInstrument>();
	model->m_hasGUI = m_toggleUIButton->isChecked();
	model->reloadPlugin();
}





extern "C"
{

// neccessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{

	return new ZynAddSubFxInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}


