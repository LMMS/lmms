/*
 * zynaddsubfx.cpp - ZynAddSubFX-embedding plugin
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
#include <QtGui/QPushButton>

#include "zynaddsubfx.h"
#include "engine.h"
#include "instrument_play_handle.h"
#include "instrument_track.h"
#include "gui_templates.h"
#include "remote_zynaddsubfx.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"
#include "moc_zynaddsubfx.cxx"


extern "C"
{

plugin::descriptor PLUGIN_EXPORT zynaddsubfx_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"ZynAddSubFX",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Embedded ZynAddSubFX" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::Instrument,
	new pluginPixmapLoader( "logo" ),
	"xiz",
	NULL,
} ;

}



zynAddSubFx::zynAddSubFx( instrumentTrack * _instrumentTrack ) :
	instrument( _instrumentTrack, &zynaddsubfx_plugin_descriptor ),
	remotePlugin( "remote_zynaddsubfx" )
{
	// now we need a play-handle which cares for calling play()
	instrumentPlayHandle * iph = new instrumentPlayHandle( this );
	engine::getMixer()->addPlayHandle( iph );

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( updateSampleRate() ) );
	sendMessage( message( IdZasfPresetDirectory ).
				addString(
		( configManager::inst()->factoryPresetsDir() +
			QDir::separator() + "ZynAddSubFX" ).
						toStdString() ) );
}




zynAddSubFx::~zynAddSubFx()
{
	engine::getMixer()->removePlayHandles( getInstrumentTrack() );
}




void zynAddSubFx::saveSettings( QDomDocument & _doc,
	                             QDomElement & _this )
{
	QTemporaryFile tf;
	if( tf.open() )
	{
		lock();
		sendMessage( message( IdSaveSettingsToFile ).
				addString( QDir::toNativeSeparators(
					tf.fileName() ).toStdString() ) );
		waitForMessage( IdSaveSettingsToFile );
		unlock();
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




void zynAddSubFx::loadSettings( const QDomElement & _this )
{
	if( !_this.hasChildNodes() )
	{
		return;
	}

	QDomDocument doc;
	doc.appendChild( doc.importNode( _this.firstChild(), TRUE ) );
	QTemporaryFile tf;
	tf.setAutoRemove( false );
	if( tf.open() )
	{
		QByteArray a = doc.toString( 0 ).toUtf8();
		a.prepend( "<?xml version=\"1.0\"?>\n" );
		tf.write( a );
		lock();
		sendMessage( message( IdLoadSettingsFromFile ).
				addString( tf.fileName().toStdString() ) );
		waitForMessage( IdLoadSettingsFromFile );
		unlock();

		emit settingsChanged();
	}
}




void zynAddSubFx::loadFile( const QString & _file )
{
	lock();
	sendMessage( message( IdLoadPresetFromFile ).
				addString( _file.toStdString() ) );
	waitForMessage( IdLoadPresetFromFile );
	unlock();
	emit settingsChanged();
}




QString zynAddSubFx::nodeName( void ) const
{
	return( zynaddsubfx_plugin_descriptor.name );
}




void zynAddSubFx::play( sampleFrame * _buf )
{
	process( NULL, _buf );
	getInstrumentTrack()->processAudioBuffer( _buf,
				engine::getMixer()->framesPerPeriod(), NULL );
}




bool zynAddSubFx::handleMidiEvent( const midiEvent & _me,
                                                const midiTime & _time )
{
	processMidiEvent( _me, 0 );
	return true;
}




void zynAddSubFx::updateSampleRate( void )
{
	remotePlugin::updateSampleRate(
				engine::getMixer()->processingSampleRate() );
}




pluginView * zynAddSubFx::instantiateView( QWidget * _parent )
{
	return( new zynAddSubFxView( this, _parent ) );
}







zynAddSubFxView::zynAddSubFxView( instrument * _instrument, QWidget * _parent ) :
	instrumentView( _instrument, _parent ),
	m_exit( 0 )
{
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	m_toggleUIButton = new QPushButton( tr( "Show GUI" ), this );
	m_toggleUIButton->setCheckable( TRUE );
	m_toggleUIButton->setChecked( FALSE );
	m_toggleUIButton->setGeometry( 45, 80, 160, 24 );
	m_toggleUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	m_toggleUIButton->setFont( pointSize<8>( m_toggleUIButton->font() ) );
	connect( m_toggleUIButton, SIGNAL( toggled( bool ) ), this,
							SLOT( toggleUI() ) );
	m_toggleUIButton->setWhatsThis(
		tr( "Click here to show or hide the graphical user interface "
			"(GUI) of ZynAddSubFX." ) );
}




zynAddSubFxView::~zynAddSubFxView()
{
}




void zynAddSubFxView::modelChanged( void )
{
	zynAddSubFx * z = castModel<zynAddSubFx>();
	connect( z, SIGNAL( settingsChanged() ),
			this, SLOT( updateUI() ) );
	toggleUI();
}




void zynAddSubFxView::updateUI( void )
{
/*	zynAddSubFxManager::instance()->m_uiMutex.lock();
	m_ui->refresh_master_ui();
	zynAddSubFxManager::instance()->m_uiMutex.unlock();*/
}




void zynAddSubFxView::toggleUI( void )
{
	if( m_toggleUIButton->isChecked() )
	{
		castModel<zynAddSubFx>()->showUI();
	}
	else
	{
		castModel<zynAddSubFx>()->hideUI();
	}

}





extern "C"
{

// neccessary for getting instance out of shared lib
plugin * PLUGIN_EXPORT lmms_plugin_main( model *, void * _data )
{

	return( new zynAddSubFx(
	        static_cast<instrumentTrack *>( _data ) ) );
}


}


