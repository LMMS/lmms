/*
 * vestige.cpp - instrument-plugin for hosting VST-instruments
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "vestige.h"

#include <QtGui/QDropEvent>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QMdiArea>
#include <QtGui/QMenu>
#include <QtXml/QDomElement>

#include "engine.h"
#include "gui_templates.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "VstPlugin.h"
#include "MainWindow.h"
#include "pixmap_button.h"
#include "string_pair_drag.h"
#include "text_float.h"
#include "tooltip.h"
#include "FileDialog.h"

#include "embed.cpp"



extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT vestige_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"VeSTige",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"VST-host for using VST(i)-plugins within LMMS" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	"dll",
	NULL
} ;

}


QPixmap * VestigeInstrumentView::s_artwork = NULL;
QPixmap * manageVestigeInstrumentView::s_artwork = NULL;


vestigeInstrument::vestigeInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &vestige_plugin_descriptor ),
	m_plugin( NULL ),
	m_pluginMutex(),
	m_subWindow( NULL ),
	vstKnobs( NULL ),
	knobFModel( NULL ),
	p_subWindow( NULL )
{
	// now we need a play-handle which cares for calling play()
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( iph );
}




vestigeInstrument::~vestigeInstrument()
{
	if (p_subWindow != NULL) {
		delete p_subWindow;
		p_subWindow = NULL;
	}

	if (knobFModel != NULL) {
		delete []knobFModel;
		knobFModel = NULL;
	}

	engine::mixer()->removePlayHandles( instrumentTrack() );
	closePlugin();
}




void vestigeInstrument::loadSettings( const QDomElement & _this )
{
	loadFile( _this.attribute( "plugin" ) );
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		m_plugin->loadSettings( _this );

		const QMap<QString, QString> & dump = m_plugin->parameterDump();
		paramCount = dump.size();
		char paramStr[35];
		vstKnobs = new knob *[ paramCount ];
		knobFModel = new FloatModel *[ paramCount ];
		QStringList s_dumpValues;
		QWidget * widget = new QWidget();
		for( int i = 0; i < paramCount; i++ )
		{
			sprintf( paramStr, "param%d", i );
			s_dumpValues = dump[ paramStr ].split( ":" );

			vstKnobs[i] = new knob( knobBright_26, widget, s_dumpValues.at( 1 ) );
			vstKnobs[i]->setHintText( s_dumpValues.at( 1 ) + ":", "" );
			vstKnobs[i]->setLabel( s_dumpValues.at( 1 ).left( 15 ) );

			knobFModel[i] = new FloatModel( 0.0f, 0.0f, 1.0f, 0.01f, this, QString::number(i) );
			knobFModel[i]->loadSettings( _this, paramStr );

			if( !( knobFModel[ i ]->isAutomated() || knobFModel[ i ]->controllerConnection() ) )
			{
				knobFModel[ i ]->setValue( ( s_dumpValues.at( 2 )).toFloat() );
				knobFModel[ i ]->setInitValue( ( s_dumpValues.at( 2 )).toFloat() );
			}

			connect( knobFModel[i], SIGNAL( dataChanged() ), this, SLOT( setParameter() ) );

			vstKnobs[i]->setModel( knobFModel[i] );
		}
	}
	m_pluginMutex.unlock();
}




void vestigeInstrument::setParameter( void )
{

	Model *action = qobject_cast<Model *>(sender());
	int knobUNID = action->displayName().toInt();

	if ( m_plugin != NULL ) {
		m_plugin->setParam( knobUNID, knobFModel[knobUNID]->value() );
	}
}




void vestigeInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( QFileInfo( m_pluginDLL ).isAbsolute() )
	{
		QString f = QString( m_pluginDLL ).replace( QDir::separator(), '/' );
		QString vd = QString( configManager::inst()->vstDir() ).replace( QDir::separator(), '/' );
        	QString relativePath;
		if( !( relativePath = f.section( vd, 1, 1 ) ).isEmpty() )
		{
			m_pluginDLL = relativePath;
		}
	}

	_this.setAttribute( "plugin", m_pluginDLL );
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		m_plugin->saveSettings( _doc, _this );
		if (knobFModel != NULL) {
			const QMap<QString, QString> & dump = m_plugin->parameterDump();
			paramCount = dump.size();
			char paramStr[35];
			for( int i = 0; i < paramCount; i++ )
			{
				if (knobFModel[i]->isAutomated() || knobFModel[i]->controllerConnection()) {
					sprintf( paramStr, "param%d", i);
					knobFModel[i]->saveSettings( _doc, _this, paramStr );
				}

/*				QDomElement me = _doc.createElement( paramStr );
				me.setAttribute( "id", knobFModel[i]->id() );
				me.setAttribute( "value", knobFModel[i]->value() );
				_this.appendChild( me );

				ControllerConnection * m_controllerConnection = knobFModel[i]->controllerConnection();
				if (m_controllerConnection) {
					QDomElement controller_element;
					QDomNode node = _this.namedItem( "connection" );
					if( node.isElement() )
					{
						controller_element = node.toElement();
					}
					else
					{
						controller_element = _doc.createElement( "connection" );
						_this.appendChild( controller_element );
					}
					QDomElement element = _doc.createElement( paramStr );
					m_controllerConnection->saveSettings( _doc, element );
					controller_element.appendChild( element );
				}*/
			}
		}
	}
	m_pluginMutex.unlock();
}




QString vestigeInstrument::nodeName( void ) const
{
	return( vestige_plugin_descriptor.name );
}




void vestigeInstrument::loadFile( const QString & _file )
{
	m_pluginMutex.lock();
	const bool set_ch_name = ( m_plugin != NULL &&
        	instrumentTrack()->name() == m_plugin->name() ) ||
            	instrumentTrack()->name() == InstrumentTrack::tr( "Default preset" ) ||
            	instrumentTrack()->name() == displayName();

	m_pluginMutex.unlock();

	if ( m_plugin != NULL )
	{
		closePlugin();
	}

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

	m_plugin->showEditor( NULL, false );

	if( set_ch_name )
	{
		instrumentTrack()->setName( m_plugin->name() );
	}

	m_pluginMutex.unlock();

	emit dataChanged();

	delete tf;
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

	const fpp_t frames = engine::mixer()->framesPerPeriod();

	instrumentTrack()->processAudioBuffer( _buf, frames, NULL );

	m_pluginMutex.unlock();
}




bool vestigeInstrument::handleMidiEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset )
{
	m_pluginMutex.lock();
	if( m_plugin != NULL )
	{
		m_plugin->processMidiEvent( event, offset );
	}
	m_pluginMutex.unlock();

	return true;
}




void vestigeInstrument::closePlugin( void )
{
	// disconnect all signals
	if( knobFModel != NULL )
	{
		for( int i = 0; i < paramCount; i++ )
		{
			delete knobFModel[ i ];
			delete vstKnobs[ i ];
		}
	}

	if( vstKnobs != NULL )
	{
		delete [] vstKnobs;
		vstKnobs = NULL;
	}

	if( knobFModel != NULL )
	{
		delete [] knobFModel;
		knobFModel = NULL;
	}

	if( m_scrollArea != NULL )
	{
//		delete m_scrollArea;
		m_scrollArea = NULL;
	}

	if( m_subWindow != NULL )
	{
		m_subWindow->setAttribute( Qt::WA_DeleteOnClose );
		m_subWindow->close();

		if( m_subWindow != NULL )
		{
			delete m_subWindow;
		}
		m_subWindow = NULL;
	}

	if( p_subWindow != NULL )
	{
		p_subWindow = NULL;
	}

	m_pluginMutex.lock();
	if( m_plugin )
	{
		delete m_plugin->pluginWidget();
	}
	delete m_plugin;
	m_plugin = NULL;
	m_pluginMutex.unlock();
}



PluginView * vestigeInstrument::instantiateView( QWidget * _parent )
{
	return new VestigeInstrumentView( this, _parent );
}





VestigeInstrumentView::VestigeInstrumentView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent ),
	lastPosInMenu (0)
{
	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}

	m_openPluginButton = new pixmapButton( this, "" );
	m_openPluginButton->setCheckable( false );
	m_openPluginButton->setCursor( Qt::PointingHandCursor );
	m_openPluginButton->move( 216, 81 );
	m_openPluginButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file_active" ) );
	m_openPluginButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	connect( m_openPluginButton, SIGNAL( clicked() ), this,
						SLOT( openPlugin() ) );
	toolTip::add( m_openPluginButton, tr( "Open other VST-plugin" ) );

	m_openPluginButton->setWhatsThis(
		tr( "Click here, if you want to open another VST-plugin. After "
			"clicking on this button, a file-open-dialog appears "
			"and you can select your file." ) );

	m_managePluginButton = new pixmapButton( this, "" );
	m_managePluginButton->setCheckable( false );
	m_managePluginButton->setCursor( Qt::PointingHandCursor );
	m_managePluginButton->move( 216, 101 );
	m_managePluginButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"track_op_menu_active" ) );
	m_managePluginButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"track_op_menu" ) );
	connect( m_managePluginButton, SIGNAL( clicked() ), this,
						SLOT( managePlugin() ) );
	toolTip::add( m_managePluginButton, tr( "Control VST-plugin from LMMS host" ) );

	m_managePluginButton->setWhatsThis(
		tr( "Click here, if you want to control VST-plugin from host." ) );


	m_openPresetButton = new pixmapButton( this, "" );
	m_openPresetButton->setCheckable( false );
	m_openPresetButton->setCursor( Qt::PointingHandCursor );
	m_openPresetButton->move( 200, 224 );
	m_openPresetButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"project_open", 20, 20 ) );
	m_openPresetButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"project_open", 20, 20 ) );
	connect( m_openPresetButton, SIGNAL( clicked() ), this,
						SLOT( openPreset() ) );
	toolTip::add( m_openPresetButton, tr( "Open VST-plugin preset" ) );

	m_openPresetButton->setWhatsThis(
		tr( "Click here, if you want to open another *.fxp, *.fxb VST-plugin preset." ) );


	m_rolLPresetButton = new pixmapButton( this, "" );
	m_rolLPresetButton->setCheckable( false );
	m_rolLPresetButton->setCursor( Qt::PointingHandCursor );
	m_rolLPresetButton->move( 190, 201 );
	m_rolLPresetButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-left-press" ) );
	m_rolLPresetButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-left" ) );
	connect( m_rolLPresetButton, SIGNAL( clicked() ), this,
						SLOT( previousProgram() ) );
	toolTip::add( m_rolLPresetButton, tr( "Previous (-)" ) );

	m_rolLPresetButton->setShortcut( Qt::Key_Minus );

	m_rolLPresetButton->setWhatsThis(
		tr( "Click here, if you want to switch to another VST-plugin preset program." ) );


	m_savePresetButton = new pixmapButton( this, "" );
	m_savePresetButton->setCheckable( false );
	m_savePresetButton->setCursor( Qt::PointingHandCursor );
	m_savePresetButton->move( 224, 224 );
	m_savePresetButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"project_save", 20, 20  ) );
	m_savePresetButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"project_save", 20, 20  ) );
	connect( m_savePresetButton, SIGNAL( clicked() ), this,
						SLOT( savePreset() ) );
	toolTip::add( m_savePresetButton, tr( "Save preset" ) );

	m_savePresetButton->setWhatsThis(
		tr( "Click here, if you want to save current VST-plugin preset program." ) );


	m_rolRPresetButton = new pixmapButton( this, "" );
	m_rolRPresetButton->setCheckable( false );
	m_rolRPresetButton->setCursor( Qt::PointingHandCursor );
	m_rolRPresetButton->move( 209, 201 );
	m_rolRPresetButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-right-press" ) );
	m_rolRPresetButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-right" ) );
	connect( m_rolRPresetButton, SIGNAL( clicked() ), this,
						SLOT( nextProgram() ) );
	toolTip::add( m_rolRPresetButton, tr( "Next (+)" ) );

	m_rolRPresetButton->setShortcut( Qt::Key_Plus );

	m_rolRPresetButton->setWhatsThis(
		tr( "Click here, if you want to switch to another VST-plugin preset program." ) );



	m_selPresetButton = new QPushButton( tr( "" ), this );
	m_selPresetButton->setGeometry( 228, 201, 16, 16 );

	QMenu *menu = new QMenu;

	connect( menu, SIGNAL( aboutToShow() ), this, SLOT( updateMenu() ) );


	m_selPresetButton->setIcon( PLUGIN_NAME::getIconPixmap( "stepper-down" ) );
	m_selPresetButton->setWhatsThis(
		tr( "Click here to select presets that are currently loaded in VST." ) );

	m_selPresetButton->setMenu(menu);


	m_toggleGUIButton = new QPushButton( tr( "Show/hide GUI" ), this );
	m_toggleGUIButton->setGeometry( 20, 130, 200, 24 );
	m_toggleGUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	m_toggleGUIButton->setFont( pointSize<8>( m_toggleGUIButton->font() ) );
	connect( m_toggleGUIButton, SIGNAL( clicked() ), this,
							SLOT( toggleGUI() ) );
	m_toggleGUIButton->setWhatsThis(
		tr( "Click here to show or hide the graphical user interface "
			"(GUI) of your VST-plugin." ) );

	QPushButton * note_off_all_btn = new QPushButton( tr( "Turn off all "
							"notes" ), this );
	note_off_all_btn->setGeometry( 20, 160, 200, 24 );
	note_off_all_btn->setIcon( embed::getIconPixmap( "state_stop" ) );
	note_off_all_btn->setFont( pointSize<8>( note_off_all_btn->font() ) );
	connect( note_off_all_btn, SIGNAL( clicked() ), this,
							SLOT( noteOffAll() ) );

	setAcceptDrops( true );
	_instrument2 = _instrument;
	_parent2 = _parent;
}


void VestigeInstrumentView::managePlugin( void )
{
	if ( m_vi->m_plugin != NULL && m_vi->m_subWindow == NULL ) {
		m_vi->p_subWindow = new manageVestigeInstrumentView( _instrument2, _parent2, m_vi);
	} else if (m_vi->m_subWindow != NULL) {
		if (m_vi->m_subWindow->widget()->isVisible() == FALSE) {
			m_vi->m_scrollArea->show();
			m_vi->m_subWindow->show();
		} else {
			m_vi->m_scrollArea->hide();
			m_vi->m_subWindow->hide();
		}
	}
}


void VestigeInstrumentView::updateMenu( void )
{

	// get all presets -
	if ( m_vi->m_plugin != NULL )
	{
		m_vi->m_plugin->loadProgramNames();
		QWidget::update();

     		QString str = m_vi->m_plugin->allProgramNames();

    		QStringList list1 = str.split("|");

     		QMenu * to_menu = m_selPresetButton->menu();
    		to_menu->clear();

    		QAction *presetActions[list1.size()];

     		for (int i = 0; i < list1.size(); i++) {
			presetActions[i] = new QAction(this);
			connect(presetActions[i], SIGNAL(triggered()), this, SLOT(selPreset()));

        		presetActions[i]->setText(QString("%1. %2").arg(QString::number(i+1), list1.at(i)));
        		presetActions[i]->setData(i);
			if (i == lastPosInMenu) {
        			presetActions[i]->setIcon(embed::getIconPixmap( "sample_file", 16, 16 ));
			} else  presetActions[i]->setIcon(embed::getIconPixmap( "edit_copy", 16, 16 ));
			to_menu->addAction( presetActions[i] );
     		}

	}
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
	FileDialog ofd( NULL, tr( "Open VST-plugin" ) );

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
	ofd.setFileMode( FileDialog::ExistingFiles );

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
		engine::mixer()->lock();

		if (m_vi->p_subWindow != NULL) {
			delete m_vi->p_subWindow;
			m_vi->p_subWindow = NULL;
		}

		m_vi->loadFile( ofd.selectedFiles()[0] );
		engine::mixer()->unlock();
		if( m_vi->m_plugin && m_vi->m_plugin->pluginWidget() )
		{
			m_vi->m_plugin->pluginWidget()->setWindowIcon(
									PLUGIN_NAME::getIconPixmap( "logo" ) );
		}
	}
}




void VestigeInstrumentView::openPreset()
{

	if ( m_vi->m_plugin != NULL ) {
		m_vi->m_plugin->openPreset( );
    		bool converted;
    		QString str = m_vi->m_plugin->currentProgramName().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		QWidget::update();
	}

}




void VestigeInstrumentView::savePreset()
{

	if ( m_vi->m_plugin != NULL )
	{
		m_vi->m_plugin->savePreset( );
/*    		bool converted;
    		QString str = m_vi->m_plugin->presetString().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		QWidget::update();*/
	}

}




void VestigeInstrumentView::nextProgram()
{

	if ( m_vi->m_plugin != NULL ) {
		m_vi->m_plugin->rotateProgram( 1 );
    		bool converted;
    		QString str = m_vi->m_plugin->currentProgramName().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		QWidget::update();
	}
}




void VestigeInstrumentView::previousProgram()
{

	if ( m_vi->m_plugin != NULL ) {
		m_vi->m_plugin->rotateProgram( -1 );
    		bool converted;
    		QString str = m_vi->m_plugin->currentProgramName().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		QWidget::update();
	}
}




void VestigeInstrumentView::selPreset( void )
{

     QAction *action = qobject_cast<QAction *>(sender());
     if (action)
         if ( m_vi->m_plugin != NULL ) {
		lastPosInMenu = action->data().toInt();
		m_vi->m_plugin->setProgram( action->data().toInt() );
		QWidget::update();
	 }
}




void VestigeInstrumentView::toggleGUI( void )
{
	if( m_vi == NULL || m_vi->m_plugin == NULL )
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




void VestigeInstrumentView::noteOffAll( void )
{
	m_vi->m_pluginMutex.lock();
	if( m_vi->m_plugin != NULL )
	{
		for( int key = 0; key <= MidiMaxKey; ++key )
		{
			m_vi->m_plugin->processMidiEvent( MidiEvent( MidiNoteOff, 0, key, 0 ), 0 );
		}
	}
	m_vi->m_pluginMutex.unlock();
}




void VestigeInstrumentView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( stringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						stringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == "vstplugin" )
		{
			_dee->acceptProposedAction();
		}
		else
		{
			_dee->ignore();
		}
	}
	else
	{
		_dee->ignore();
	}
}




void VestigeInstrumentView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "vstplugin" )
	{
		m_vi->loadFile( value );
		_de->accept();
		return;
	}
	_de->ignore();
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
	p.setPen( QColor( 255, 255, 255 ) );
	p.drawText( 10, 100, plugin_name );

	p.setPen( QColor( 50, 50, 50 ) );
	p.drawText( 10, 211, tr( "Preset" ) );

//	m_pluginMutex.lock();
	if( m_vi->m_plugin != NULL )
	{
		p.setPen( QColor( 0, 0, 0 ) );
		f.setBold( false );
		p.setFont( pointSize<8>( f ) );
		p.drawText( 10, 114, tr( "by " ) +
					m_vi->m_plugin->vendorString() );
		p.setPen( QColor( 255, 255, 255 ) );
		p.drawText( 10, 225, m_vi->m_plugin->currentProgramName() );
	}

	if( m_vi->m_subWindow != NULL )
	{
		m_vi->m_subWindow->setWindowTitle( m_vi->instrumentTrack()->name()
								+ tr( " - VST plugin control" ) );
	}
//	m_pluginMutex.unlock();
}




manageVestigeInstrumentView::manageVestigeInstrumentView( Instrument * _instrument,
							QWidget * _parent, vestigeInstrument * m_vi2 ) :
	InstrumentView( _instrument, _parent )
{
	m_vi = m_vi2;
	m_vi->m_scrollArea = new QScrollArea( this );
	widget = new QWidget(this);
	l = new QGridLayout( this );

	m_vi->m_subWindow = engine::mainWindow()->workspace()->addSubWindow(new QMdiSubWindow, Qt::SubWindow |
			Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
	m_vi->m_subWindow->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::MinimumExpanding );
	m_vi->m_subWindow->setFixedWidth( 960 );
	m_vi->m_subWindow->setMinimumHeight( 300 );
	m_vi->m_subWindow->setWidget(m_vi->m_scrollArea);
	m_vi->m_subWindow->setWindowTitle( m_vi->instrumentTrack()->name()
								+ tr( " - VST plugin control" ) );
	m_vi->m_subWindow->setWindowIcon( PLUGIN_NAME::getIconPixmap( "logo" ) );
	//m_vi->m_subWindow->setAttribute(Qt::WA_DeleteOnClose);


	l->setContentsMargins( 20, 10, 10, 10 );
	l->setVerticalSpacing( 10 );
	l->setHorizontalSpacing( 23 );

	m_syncButton = new QPushButton( tr( "VST Sync" ), this );
	connect( m_syncButton, SIGNAL( clicked() ), this,
							SLOT( syncPlugin() ) );
	m_syncButton->setWhatsThis(
		tr( "Click here if you want to synchronize all parameters with VST plugin." ) );

	l->addWidget( m_syncButton, 0, 0, 1, 2, Qt::AlignLeft );

	m_displayAutomatedOnly = new QPushButton( tr( "Automated" ), this );
	connect( m_displayAutomatedOnly, SIGNAL( clicked() ), this,
							SLOT( displayAutomatedOnly() ) );
	m_displayAutomatedOnly->setWhatsThis(
		tr( "Click here if you want to display automated parameters only." ) );

	l->addWidget( m_displayAutomatedOnly, 0, 1, 1, 2, Qt::AlignLeft );


	m_closeButton = new QPushButton( tr( "    Close    " ), widget );
	connect( m_closeButton, SIGNAL( clicked() ), this,
							SLOT( closeWindow() ) );
	m_closeButton->setWhatsThis(
		tr( "Close VST plugin knob-controller window." ) );

	l->addWidget( m_closeButton, 0, 2, 1, 7, Qt::AlignLeft );


	for( int i = 0; i < 10; i++ )
	{
		l->addItem( new QSpacerItem( 68, 45, QSizePolicy::Fixed, QSizePolicy::Fixed ), 0, i );
	}

	const QMap<QString, QString> & dump = m_vi->m_plugin->parameterDump();
	m_vi->paramCount = dump.size();

	bool isVstKnobs = true;

	if (m_vi->vstKnobs == NULL) {
		m_vi->vstKnobs = new knob *[ m_vi->paramCount ];
		isVstKnobs = false;
	}
	if (m_vi->knobFModel == NULL) {
		m_vi->knobFModel = new FloatModel *[ m_vi->paramCount ];
	}

	char paramStr[35];
	QStringList s_dumpValues;

	if (isVstKnobs == false) {
		for( int i = 0; i < m_vi->paramCount; i++ )
		{
			sprintf( paramStr, "param%d", i);
    			s_dumpValues = dump[ paramStr ].split( ":" );

			m_vi->vstKnobs[ i ] = new knob( knobBright_26, this, s_dumpValues.at( 1 ) );
			m_vi->vstKnobs[ i ]->setHintText( s_dumpValues.at( 1 ) + ":", "" );
			m_vi->vstKnobs[ i ]->setLabel( s_dumpValues.at( 1 ).left( 15 ) );

			sprintf( paramStr, "%d", i);
			m_vi->knobFModel[ i ] = new FloatModel( (s_dumpValues.at( 2 )).toFloat(),
				0.0f, 1.0f, 0.01f, castModel<vestigeInstrument>(), tr( paramStr ) );
			connect( m_vi->knobFModel[i], SIGNAL( dataChanged() ), this, SLOT( setParameter() ) );
			m_vi->vstKnobs[i] ->setModel( m_vi->knobFModel[i] );
		}
	}

	int i = 0;
	for( int lrow = 1; lrow < ( int( m_vi->paramCount / 10 ) + 1 ) + 1; lrow++ )
	{
		for( int lcolumn = 0; lcolumn < 10; lcolumn++ )
		{
			if( i < m_vi->paramCount )
			{
				l->addWidget( m_vi->vstKnobs[i], lrow, lcolumn, Qt::AlignCenter );
			}
			i++;
		}
	}

	l->setRowStretch( ( int( m_vi->paramCount / 10) + 1), 1 );
	l->setColumnStretch( 10, 1 );

	widget->setLayout(l);
	widget->setAutoFillBackground(true);

	m_vi->m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_vi->m_scrollArea->setPalette( QApplication::palette( m_vi->m_scrollArea ) );
	m_vi->m_scrollArea->setMinimumHeight( 64 );

	m_vi->m_scrollArea->setWidget( widget );

	m_vi->m_subWindow->show();
}




void manageVestigeInstrumentView::closeWindow()
{
	m_vi->m_subWindow->hide();
}




void manageVestigeInstrumentView::syncPlugin( void )
{
	char paramStr[35];
	QStringList s_dumpValues;
	const QMap<QString, QString> & dump = m_vi->m_plugin->parameterDump();
	float f_value;

	for( int i = 0; i < m_vi->paramCount; i++ )
	{
		// only not automated knobs are synced from VST
		// those auto-setted values are not jurnaled, tracked for undo / redo
		if( !( m_vi->knobFModel[ i ]->isAutomated() || m_vi->knobFModel[ i ]->controllerConnection() ) )
		{
			sprintf( paramStr, "param%d", i );
    			s_dumpValues = dump[ paramStr ].split( ":" );
			f_value = ( s_dumpValues.at( 2 ) ).toFloat();
			m_vi->knobFModel[ i ]->setAutomatedValue( f_value );
			m_vi->knobFModel[ i ]->setInitValue( f_value );
		}
	}
}




void manageVestigeInstrumentView::displayAutomatedOnly( void )
{
	bool isAuto = QString::compare( m_displayAutomatedOnly->text(), tr( "Automated" ) ) == 0;

	for( int i = 0; i< m_vi->paramCount; i++ )
	{

		if( !( m_vi->knobFModel[ i ]->isAutomated() || m_vi->knobFModel[ i ]->controllerConnection() ) )
		{
			if( m_vi->vstKnobs[ i ]->isVisible() == true  && isAuto )
			{
				m_vi->vstKnobs[ i ]->hide();
				m_displayAutomatedOnly->setText( "All" );
			} else {
				m_vi->vstKnobs[ i ]->show();
				m_displayAutomatedOnly->setText( "Automated" );
			}
		}
	}
}


manageVestigeInstrumentView::~manageVestigeInstrumentView()
{
	if( m_vi->knobFModel != NULL )
	{
		for( int i = 0; i < m_vi->paramCount; i++ )
		{
			delete m_vi->knobFModel[ i ];
			delete m_vi->vstKnobs[ i ];
		}
	}

	if (m_vi->vstKnobs != NULL) {
		delete []m_vi->vstKnobs;
		m_vi->vstKnobs = NULL;
	}

	if( m_vi->knobFModel != NULL )
	{
		delete [] m_vi->knobFModel;
		m_vi->knobFModel = NULL;
	}

	if (m_vi->m_scrollArea != NULL) {
		delete m_vi->m_scrollArea;
		m_vi->m_scrollArea = NULL;
	}

	if ( m_vi->m_subWindow != NULL ) {
		m_vi->m_subWindow->setAttribute(Qt::WA_DeleteOnClose);
		m_vi->m_subWindow->close();

		if ( m_vi->m_subWindow != NULL )
			delete m_vi->m_subWindow;
		m_vi->m_subWindow = NULL;
	}

	m_vi->p_subWindow = NULL;
}




void manageVestigeInstrumentView::setParameter( void )
{

	Model *action = qobject_cast<Model *>(sender());
	int knobUNID = action->displayName().toInt();

	if ( m_vi->m_plugin != NULL ) {
		m_vi->m_plugin->setParam( knobUNID, m_vi->knobFModel[knobUNID]->value() );
	}
}



void manageVestigeInstrumentView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( stringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						stringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == "vstplugin" )
		{
			_dee->acceptProposedAction();
		}
		else
		{
			_dee->ignore();
		}
	}
	else
	{
		_dee->ignore();
	}
}




void manageVestigeInstrumentView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "vstplugin" )
	{
		m_vi->loadFile( value );
		_de->accept();
		return;
	}
	_de->ignore();
}




void manageVestigeInstrumentView::paintEvent( QPaintEvent * )
{
	m_vi->m_subWindow->setWindowTitle( m_vi->instrumentTrack()->name()
					+ tr( " - VST plugin control" ) );
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new vestigeInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}


#include "moc_vestige.cxx"

