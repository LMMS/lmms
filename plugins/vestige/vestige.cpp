/*
 * vestige.cpp - instrument-plugin for hosting VST-instruments
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "vestige.h"

#include <QtGui/QDropEvent>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtXml/QDomElement>

#include "ResourceFileMapper.h"
#include "engine.h"
#include "gui_templates.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "VstPlugin.h"
#include "pixmap_button.h"
#include "string_pair_drag.h"
#include "text_float.h"
#include "tooltip.h"

#include "embed.cpp"


static const char * __supportedExts[] =
{ "dll", NULL };

extern "C"
{

Plugin::Descriptor vestige_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"VeSTige",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"VST-host for using VST(i)-plugins within LMMS" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	__supportedExts,
	NULL
} ;

}


QPixmap * VestigeInstrumentView::s_artwork = NULL;


vestigeInstrument::vestigeInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &vestige_plugin_descriptor ),
	m_plugin( NULL ),
	m_pluginMutex()
{
	// now we need a play-handle which cares for calling play()
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
	engine::getMixer()->addPlayHandle( iph );
}




vestigeInstrument::~vestigeInstrument()
{
	engine::getMixer()->removePlayHandles( instrumentTrack() );
	closePlugin();
}




void vestigeInstrument::loadSettings( const QDomElement & _this )
{
	loadFile( _this.attribute( "plugin" ) );
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		m_plugin->loadSettings( _this );
	}
	m_pluginMutex.unlock();
}




void vestigeInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "plugin", m_pluginDLL );
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		m_plugin->saveSettings( _doc, _this );
	}
	m_pluginMutex.unlock();
}




QString vestigeInstrument::nodeName() const
{
	return vestige_plugin_descriptor.name;
}




void vestigeInstrument::loadResource( const ResourceItem * _item )
{
	ResourceFileMapper mapper( _item );
	loadFile( mapper.fileName() );
}




void vestigeInstrument::play( sampleFrame * _buf )
{
	m_pluginMutex.lock();
	if( m_plugin == NULL )
	{
		m_pluginMutex.unlock();
		return;
	}

	m_plugin->process( NULL, _buf );

	const fpp_t frames = engine::getMixer()->framesPerPeriod();

	instrumentTrack()->processAudioBuffer( _buf, frames, NULL );

	m_pluginMutex.unlock();
}




bool vestigeInstrument::handleMidiEvent( const midiEvent & _me,
						const midiTime & _time )
{
	if( !isMuted() )
	{
		m_pluginMutex.lock();
		if( m_plugin != NULL )
		{
			m_plugin->processMidiEvent( _me, _time );
		}
		m_pluginMutex.unlock();
	}
	return true;
}




void vestigeInstrument::closePlugin()
{
	m_pluginMutex.lock();
	if( m_plugin )
	{
		delete m_plugin->pluginWidget();
	}
	delete m_plugin;
	m_plugin = NULL;
	m_pluginMutex.unlock();
}




void vestigeInstrument::loadFile( const QString & _file )
{
	m_pluginMutex.lock();
	const bool set_ch_name = ( m_plugin != NULL &&
		instrumentTrack()->name() == m_plugin->name() ) ||
			instrumentTrack()->name() ==
				InstrumentTrack::tr( "Default preset" );
	m_pluginMutex.unlock();

	closePlugin();

	m_pluginDLL = _file;
	textFloat * tf = textFloat::displayMessage(
			tr( "Loading plugin" ),
			tr( "Please wait while loading VST-plugin..." ),
			PLUGIN_NAME::getIconPixmap( "logo", 24, 24 ), 0 );

	m_pluginMutex.lock();
	m_plugin = new VstPlugin( m_pluginDLL );
	if( m_plugin->failed() )
	{
		m_pluginMutex.unlock();
		closePlugin();
		delete tf;
		QMessageBox::information( 0,
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

	m_plugin->showEditor();

	if( set_ch_name )
	{
		instrumentTrack()->setName( m_plugin->name() );
	}

	m_pluginMutex.unlock();

	emit dataChanged();

	delete tf;
}




PluginView * vestigeInstrument::instantiateView( QWidget * _parent )
{
	return new VestigeInstrumentView( this, _parent );
}









VestigeInstrumentView::VestigeInstrumentView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}

	m_openPluginButton = new pixmapButton( this, "" );
	m_openPluginButton->setCheckable( false );
	m_openPluginButton->setCursor( Qt::PointingHandCursor );
	m_openPluginButton->move( 218, 79 );
	m_openPluginButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	m_openPluginButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	connect( m_openPluginButton, SIGNAL( clicked() ), this,
						SLOT( openPlugin() ) );
	toolTip::add( m_openPluginButton, tr( "Open other VST-plugin" ) );

	m_openPluginButton->setWhatsThis(
		tr( "Click here, if you want to open another VST-plugin. After "
			"clicking on this button, a file-open-dialog appears "
			"and you can select your file." ) );

	m_toggleGUIButton = new QPushButton( tr( "Show/hide GUI" ), this );
	m_toggleGUIButton->setGeometry( 20, 150, 200, 24 );
	m_toggleGUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	m_toggleGUIButton->setFont( pointSize<8>( m_toggleGUIButton->font() ) );
	connect( m_toggleGUIButton, SIGNAL( clicked() ), this,
							SLOT( toggleGUI() ) );
	m_toggleGUIButton->setWhatsThis(
		tr( "Click here to show or hide the graphical user interface "
			"(GUI) of your VST-plugin." ) );

	QPushButton * note_off_all_btn = new QPushButton( tr( "Turn off all "
							"notes" ), this );
	note_off_all_btn->setGeometry( 20, 180, 200, 24 );
	note_off_all_btn->setIcon( embed::getIconPixmap( "state_stop" ) );
	note_off_all_btn->setFont( pointSize<8>( note_off_all_btn->font() ) );
	connect( note_off_all_btn, SIGNAL( clicked() ), this,
							SLOT( noteOffAll() ) );
}




VestigeInstrumentView::~VestigeInstrumentView()
{
}




void VestigeInstrumentView::modelChanged()
{
	m_vi = castModel<vestigeInstrument>();
}




void VestigeInstrumentView::openPlugin()
{
	QFileDialog ofd( NULL, tr( "Open VST-plugin" ) );

	QString dir;
	if( m_vi->m_pluginDLL != "" )
	{
		dir = QFileInfo( m_vi->m_pluginDLL ).absolutePath();
	}
	else
	{
		dir = configManager::inst()->vstDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	ofd.setFileMode( QFileDialog::ExistingFiles );

	// set filters
	QStringList types;
	types << tr( "DLL-files (*.dll)" )
		<< tr( "EXE-files (*.exe)" )
		;
	ofd.setFilters( types );
	if( m_vi->m_pluginDLL != "" )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_vi->m_pluginDLL ).fileName() );
	}

	if ( ofd.exec () == QDialog::Accepted )
	{
		if( ofd.selectedFiles().isEmpty() )
		{
			return;
		}
		engine::getMixer()->lock();
		m_vi->loadFile( ofd.selectedFiles()[0] );
		engine::getMixer()->unlock();
		if( m_vi->m_plugin && m_vi->m_plugin->pluginWidget() )
		{
			m_vi->m_plugin->pluginWidget()->setWindowIcon(
									PLUGIN_NAME::getIconPixmap( "logo" ) );
		}
	}
}




void VestigeInstrumentView::toggleGUI()
{
	QMutexLocker ml( &m_vi->m_pluginMutex );
	if( m_vi->m_plugin == NULL )
	{
		return;
	}
	QWidget * w = m_vi->m_plugin->pluginWidget();
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




void VestigeInstrumentView::noteOffAll()
{
	m_vi->m_pluginMutex.lock();
	if( m_vi->m_plugin != NULL )
	{
		for( int key = 0; key < NumKeys; ++key )
		{
			m_vi->m_plugin->processMidiEvent(
				midiEvent( MidiNoteOff, 0, key, 0 ), 0 );
		}
	}
	m_vi->m_pluginMutex.unlock();
}




void VestigeInstrumentView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, *s_artwork );

	QString plugin_name = ( m_vi->m_plugin != NULL ) ? 
				m_vi->m_plugin->name()/* + QString::number(
						m_plugin->version() )*/
					:
				tr( "No VST-plugin loaded" );
	QFont f = p.font();
	f.setBold( true );
	p.setFont( pointSize<10>( f ) );
	p.setPen( QColor( 32, 160, 54 ) );

	p.drawText( 10, 100, plugin_name );

//	m_pluginMutex.lock();
	if( m_vi->m_plugin != NULL )
	{
		p.setPen( QColor( 251, 41, 8 ) );
		f.setBold( false );
		p.setFont( pointSize<8>( f ) );
		p.drawText( 10, 114, tr( "by" ) + " " +
					m_vi->m_plugin->vendorString() );
	}
//	m_pluginMutex.unlock();
}








extern "C"
{

// neccessary for getting instance out of shared lib
Plugin * lmms_plugin_main( Model *, void * _data )
{
	return new vestigeInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}


#include "moc_vestige.cxx"

