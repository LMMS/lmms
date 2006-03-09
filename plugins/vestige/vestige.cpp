/*
 * vestige.cpp - instrument-plugin for hosting VST-plugins
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QPushButton>
#include <QCursor>

#else

#include <qdom.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qpushbutton.h>
#include <qcursor.h>
#include <qwhatsthis.h>

#endif


#include "channel_track.h"
#include "note_play_handle.h"
#include "buffer_allocator.h"
#include "mixer.h"
#include "instrument_play_handle.h"
#include "pixmap_button.h"
#include "tooltip.h"
#include "spc_bg_hndl_widget.h"
#include "vestige.h"
#include "text_float.h"
#include "song_editor.h"
#include "lvsl_client.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor vestige_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"VeSTige",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"VST-host for using VST(i)-plugins within LMMS" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::INSTRUMENT,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) )
} ;

}


QPixmap * vestigeInstrument::s_artwork = NULL;


vestigeInstrument::vestigeInstrument( channelTrack * _channel_track ) :
	instrument( _channel_track, &vestige_plugin_descriptor ),
	specialBgHandlingWidget( PLUGIN_NAME::getIconPixmap( "artwork" ) ),
	m_plugin( NULL ),
	m_pluginMutex()
{
	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}

#ifdef QT4
	QPalette pal;
	pal.setBrush( backgroundRole(), *s_artwork);
	setPalette( pal );
#else
	setErasePixmap( *s_artwork );
#endif

	connect( eng()->getSongEditor(), SIGNAL( tempoChanged( bpm_t ) ),
					 this, SLOT( changeTempo( bpm_t ) ) );

	m_openPluginButton = new pixmapButton( this );
	m_openPluginButton->setCheckable( FALSE );
	m_openPluginButton->setCursor( Qt::PointingHandCursor );
	m_openPluginButton->move( 200, 70 );
	m_openPluginButton->setActiveGraphic( embed::getIconPixmap(
							"project_open_down" ) );
	m_openPluginButton->setInactiveGraphic( embed::getIconPixmap(
							"project_open" ) );
	m_openPluginButton->setBgGraphic( getBackground(
						m_openPluginButton ) );
	connect( m_openPluginButton, SIGNAL( clicked() ), this,
						SLOT( openPlugin() ) );
	toolTip::add( m_openPluginButton, tr( "Open other VST-plugin" ) );

#ifdef QT4
	m_openPluginButton->setWhatsThis(
#else
	QWhatsThis::add( m_openPluginButton,
#endif
		tr( "Click here, if you want to open another VST-plugin. After "
			"clicking on this button, a file-open-dialog appears "
			"and you can select your file." ) );

	m_toggleGUIButton = new QPushButton( tr( "Show/hide VST-GUI" ), this );
	m_toggleGUIButton->setGeometry( 20, 120, 160, 24 );
	m_toggleGUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	m_toggleGUIButton->setFont( pointSize<8>( m_toggleGUIButton->font() ) );
	connect( m_toggleGUIButton, SIGNAL( clicked() ), this,
							SLOT( toggleGUI() ) );
#ifdef QT4
	m_toggleGUIButton->setWhatsThis(
#else
	QWhatsThis::add( m_toggleGUIButton,
#endif
		tr( "Click here to show or hide the graphical user interface "
			"(GUI) of your VST-plugin." ) );

	QPushButton * note_off_all_btn = new QPushButton( tr( "Turn off all "
							"notes" ), this );
	note_off_all_btn->setGeometry( 20, 150, 160, 24 );
	note_off_all_btn->setIcon( embed::getIconPixmap( "state_stop" ) );
	note_off_all_btn->setFont( pointSize<8>( note_off_all_btn->font() ) );
	connect( note_off_all_btn, SIGNAL( clicked() ), this,
							SLOT( noteOffAll() ) );

	// now we need a play-handle which cares for calling play()
	instrumentPlayHandle * iph = new instrumentPlayHandle( this );
	eng()->getMixer()->addPlayHandle( iph );
}




vestigeInstrument::~vestigeInstrument()
{
	// this single call automates the rest of cleanup like trashing our
	// play-handle and so on
	invalidate();
	closePlugin();
}




void vestigeInstrument::loadSettings( const QDomElement & _this )
{
	setParameter( "plugin", _this.attribute( "plugin" ) );
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		if( m_plugin->pluginWidget() != NULL )
		{
			if( _this.attribute( "guivisible" ).toInt() )
			{
				m_plugin->pluginWidget()->show();
			}
			else
			{
				m_plugin->pluginWidget()->hide();
			}
		}
		const Sint32 num_params = _this.attribute(
							"numparams" ).toInt();
		if( num_params > 0 )
		{
			QMap<QString, QString> dump;
			for( Sint32 i = 0; i < num_params; ++i )
			{
				const QString key = "param" +
							QString::number( i );
				dump[key] = _this.attribute( key );
			}
			m_plugin->setParameterDump( dump );
		}
	}
	m_pluginMutex.unlock();
}




void vestigeInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement vst_de = _doc.createElement( nodeName() );
	vst_de.setAttribute( "plugin", m_pluginDLL );
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		if( m_plugin->pluginWidget() != NULL )
		{
			vst_de.setAttribute( "guivisible",
					m_plugin->pluginWidget()->isVisible() );
		}
		const QMap<QString, QString> & dump = m_plugin->parameterDump();
		vst_de.setAttribute( "numparams", dump.size() );
		for( QMap<QString, QString>::const_iterator it = dump.begin();
							it != dump.end(); ++it )
		{
#ifdef QT4
			vst_de.setAttribute( it.key(), it.value() );
#else
			vst_de.setAttribute( it.key(), it.data() );
#endif
		}
	}
	m_pluginMutex.unlock();
	_parent.appendChild( vst_de );
}




QString vestigeInstrument::nodeName( void ) const
{
	return( vestige_plugin_descriptor.name );
}




void vestigeInstrument::setParameter( const QString & _param,
							const QString & _value )
{
	if( _param == "plugin" && _value != "" )
	{
		m_pluginMutex.lock();
		bool set_ch_name = ( m_plugin != NULL &&
			getChannelTrack()->name() == m_plugin->name() ) ||
				getChannelTrack()->name() ==
						channelTrack::tr( "Default" );
		m_pluginMutex.unlock();

		closePlugin();

		m_pluginDLL = _value;
		textFloat * tf = textFloat::displayMessage(
				tr( "Loading plugin" ),
				tr( "Please wait while loading VST-plugin..." ),
				PLUGIN_NAME::getIconPixmap( "logo", 24, 24 ),
			0 );
		m_pluginMutex.lock();
		m_plugin = new remoteVSTPlugin( m_pluginDLL, eng() );
		if( m_plugin->failed() )
		{
			m_pluginMutex.unlock();
			closePlugin();
			delete tf;
			QMessageBox::information( this,
					tr( "Failed loading VST-plugin" ),
					tr( "The VST-plugin %1 could not "
						"be loaded for some reason.\n"
						"If it runs with other VST-"
						"software under Linux, please "
						"contact an LMMS-developer!"
						).arg( m_pluginDLL ),
							QMessageBox::Ok );
			return;
		}
/*		if( m_plugin->vstVersion() < 2000 )
		{
			QMessageBox::information( this,
					tr( "VST-plugin too old" ),
					tr( "The version of VST-plugin %1 "
						"is smaller than 2, which "
						"isn't supported." ).arg(
								m_pluginDLL ),
							QMessageBox::Ok );
			closePlugin();
			return;
		}*/
		m_plugin->showEditor();
		m_plugin->setTempo( eng()->getSongEditor()->getTempo() );
		if( set_ch_name == TRUE )
		{
			getChannelTrack()->setName( m_plugin->name() );
		}
		if( m_plugin->pluginWidget() != NULL )
		{
#ifdef QT4
			m_plugin->pluginWidget()->setWindowIcon(
					getChannelTrack()->windowIcon() );
#else
			m_plugin->pluginWidget()->setWindowIcon(
					*( getChannelTrack()->windowIcon() ) );
#endif
		}
		m_pluginMutex.unlock();
		update();
		delete tf;
	}
}




void vestigeInstrument::play( void )
{
	QMutexLocker ml( &m_pluginMutex );
	if( m_plugin == NULL )
	{
		return;
	}

	const fpab_t frames = eng()->getMixer()->framesPerAudioBuffer();
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );

	m_plugin->process( NULL, buf );
	
	getChannelTrack()->processAudioBuffer( buf, frames, NULL );

	bufferAllocator::free( buf );
}






void vestigeInstrument::playNote( notePlayHandle * _n )
{
	m_pluginMutex.lock();
	if( _n->totalFramesPlayed() == 0 && m_plugin != NULL )
	{
		m_plugin->enqueueMidiEvent( midiEvent( NOTE_ON, 0,
					getChannelTrack()->masterKey( _n ),
					_n->getVolume() ), _n->framesAhead() );
	}
	m_pluginMutex.unlock();
}




void vestigeInstrument::deleteNotePluginData( notePlayHandle * _n )
{
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		m_plugin->enqueueMidiEvent( midiEvent( NOTE_OFF, 0,
					getChannelTrack()->masterKey( _n ),
								0 ), 0 );
	}
	m_pluginMutex.unlock();
}




void vestigeInstrument::openPlugin( void )
{
#ifdef QT4
	QFileDialog ofd( NULL, tr( "Open VST-plugin" ) );
#else
	QFileDialog ofd( QString::null, QString::null, NULL, "", TRUE );
	ofd.setWindowTitle( tr( "Open VST-plugin" ) );
#endif

	QString dir;
	if( m_pluginDLL != "" )
	{
#ifdef QT4
		dir = QFileInfo( m_pluginDLL ).absolutePath();
#else
		dir = QFileInfo( m_pluginDLL ).dirPath( TRUE );
#endif
	}
	else
	{
		dir = configManager::inst()->vstDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	ofd.setFileMode( QFileDialog::ExistingFiles );

	// set filters
#ifdef QT4
	QStringList types;
	types << tr( "DLL-files (*.dll)" )
		<< tr( "EXE-files (*.exe)" )
		;
	ofd.setFilters( types );
#else
	ofd.addFilter( tr( "DLL-files (*.dll)" ) );
	ofd.addFilter( tr( "EXE-files (*.exe)" ) );
	ofd.setSelectedFilter( tr( "DLL-files (*.dll)" ) );
#endif
	if( m_pluginDLL != "" )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_pluginDLL ).fileName() );
	}

	if ( ofd.exec () == QDialog::Accepted )
	{
		if( ofd.selectedFiles().isEmpty() )
		{
			return;
		}
		setParameter( "plugin", ofd.selectedFiles()[0] );
	}
}




void vestigeInstrument::toggleGUI( void )
{
	QMutexLocker ml( &m_pluginMutex );
	if( m_plugin == NULL )
	{
		return;
	}
	QWidget * w = m_plugin->pluginWidget();
	if( w == NULL )
	{
		return;
	}
	if( w->isHidden() )
	{
		w->show();
	}
	else
	{
		w->hide();
	}
}




void vestigeInstrument::noteOffAll( void )
{
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		for( int key = 0; key < OCTAVES * NOTES_PER_OCTAVE; ++key )
		{
			m_plugin->enqueueMidiEvent( midiEvent( NOTE_OFF, 0,
								key, 0 ), 0 );
		}
	}
	m_pluginMutex.unlock();
}




void vestigeInstrument::changeTempo( bpm_t _new_tempo )
{
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		m_plugin->setTempo( _new_tempo );
	}
	m_pluginMutex.unlock();
}




void vestigeInstrument::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap pm( rect().size() );
	pm.fill( this, rect().topLeft() );

	QPainter p( &pm, this );
#endif

	p.drawPixmap( 0, 0, *s_artwork );

	QString plugin_name = ( m_plugin != NULL ) ? 
				m_plugin->name()/* + QString::number(
						m_plugin->version() )*/
					:
				tr( "No VST-plugin loaded" );
	QFont f = p.font();
	f.setBold( TRUE );
	p.setFont( pointSize<10>( f ) );
	p.setPen( QColor( 0, 0, 0 ) );

	p.drawText( 20, 80, plugin_name );

//	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		p.setPen( QColor( 64, 128, 64 ) );
		f.setBold( FALSE );
		p.setFont( pointSize<8>( f ) );
		p.drawText( 20, 94, tr( "by" ) + " " +
						m_plugin->vendorString() );
	}
//	m_pluginMutex.unlock();

#ifndef QT4
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void vestigeInstrument::closePlugin( void )
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
	return( new vestigeInstrument( static_cast<channelTrack *>( _data ) ) );
}


}


#include "vestige.moc"

