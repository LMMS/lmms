#ifndef SINGLE_SOURCE_COMPILE

/*
 * main_window.cpp - implementation of LMMS-main-window
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtCore/QUrl>
#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QSplitter>

#include "main_window.h"
#include "bb_editor.h"
#include "song_editor.h"
#include "song.h"
#include "piano_roll.h"
#include "embed.h"
#include "engine.h"
#include "fx_mixer_view.h" 
#include "about_dialog.h"
#include "controller_rack_view.h"
#include "file_browser.h"
#include "plugin_browser.h"
#include "side_bar.h"
#include "config_mgr.h"
#include "mixer.h"
#include "plugin_view.h"
#include "project_notes.h"
#include "setup_dialog.h"
#include "audio_dummy.h"
#include "tool.h"
#include "tool_button.h"
#include "project_journal.h"
#include "automation_editor.h"
#include "templates.h"




mainWindow::mainWindow( void ) :
	m_workspace( NULL ),
	m_templatesMenu( NULL ),
	m_recentlyOpenedProjectsMenu( NULL ),
	m_toolsMenu( NULL )
{
	setAttribute( Qt::WA_DeleteOnClose );

	QWidget * main_widget = new QWidget( this );
	QVBoxLayout * vbox = new QVBoxLayout( main_widget );
	vbox->setSpacing( 0 );
	vbox->setMargin( 0 );


	QWidget * w = new QWidget( main_widget );
	QHBoxLayout * hbox = new QHBoxLayout( w );
	hbox->setSpacing( 0 );
	hbox->setMargin( 0 );

	sideBar * side_bar = new sideBar( sideBar::Vertical, w );
	side_bar->setStyle( sideBar::VSNET/*KDEV3ICON*/ );
	side_bar->setPosition( sideBar::Left );

	QSplitter * splitter = new QSplitter( Qt::Horizontal, w );
	splitter->setChildrenCollapsible( FALSE );

	QString sample_filter;
	QList<QString> ext_keys = engine::sampleExtensions().keys();
	for( QList<QString>::iterator it = ext_keys.begin();
						it != ext_keys.end(); ++it )
	{
		sample_filter += " *." + *it;
	}

	int id = 0;
	QString wdir = configManager::inst()->workingDir();
	side_bar->appendTab( new pluginBrowser( splitter ), ++id );
	side_bar->appendTab( new fileBrowser(
				configManager::inst()->userProjectsDir() + "*" +
				configManager::inst()->factoryProjectsDir(),
					"*.mmp *.mmpz *.xml *.mid *.flp",
							tr( "My projects" ),
					embed::getIconPixmap( "project_file" ),
							splitter ), ++id );
	side_bar->appendTab( new fileBrowser(
				configManager::inst()->userSamplesDir() + "*" +
				configManager::inst()->factorySamplesDir(),
					sample_filter, tr( "My samples" ),
					embed::getIconPixmap( "sound_file" ),
							splitter ), ++id );
	side_bar->appendTab( new fileBrowser(
				configManager::inst()->userPresetsDir() + "*" +
				configManager::inst()->factoryPresetsDir(),
					"*.cs.xml *.xiz", tr( "My presets" ),
					embed::getIconPixmap( "preset_file" ),
							splitter ), ++id );
	side_bar->appendTab( new fileBrowser( QDir::homePath(), "*",
							tr( "My home" ),
					embed::getIconPixmap( "home" ),
							splitter ), ++id );
	side_bar->appendTab( new fileBrowser( QDir::rootPath(), "*",
							tr( "Root directory" ),
					embed::getIconPixmap( "root" ),
							splitter ), ++id );

	m_workspace = new QMdiArea( splitter );
	m_workspace->setBackground( Qt::NoBrush );
	m_workspace->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_workspace->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );

	hbox->addWidget( side_bar );
	hbox->addWidget( splitter );


	// create global-toolbar at the top of our window
	m_toolBar = new QWidget( main_widget );
	m_toolBar->setObjectName( "mainToolbar" );
	m_toolBar->setFixedHeight( 64 );
	m_toolBar->move( 0, 0 );

	// add layout for organizing quite complex toolbar-layouting
	m_toolBarLayout = new QGridLayout( m_toolBar/*, 2, 1*/ );
	m_toolBarLayout->setMargin( 0 );
	m_toolBarLayout->setSpacing( 0 );

	vbox->addWidget( m_toolBar );
	vbox->addWidget( w );
	setCentralWidget( main_widget );



}




mainWindow::~mainWindow()
{
	for( QList<pluginView *>::iterator it = m_tools.begin();
						it != m_tools.end(); ++it )
	{
		model * m = ( *it )->getModel();
		delete *it;
		delete m;
	}
	// TODO: Close tools
	// destroy engine which will do further cleanups etc.
	engine::destroy();
}




void mainWindow::finalize( void )
{
	resetWindowTitle();
	setWindowIcon( embed::getIconPixmap( "icon" ) );


	// project-popup-menu
	QMenu * project_menu = new QMenu( this );
	menuBar()->addMenu( project_menu )->setText( tr( "&Project" ) );
	project_menu->addAction( embed::getIconPixmap( "project_new" ),
					tr( "&New" ),
					this, SLOT( createNewProject() ),
					Qt::CTRL + Qt::Key_N );

	project_menu->addAction( embed::getIconPixmap( "project_open" ),
					tr( "&Open..." ),
					this, SLOT( openProject() ),
					Qt::CTRL + Qt::Key_O );	

	m_recentlyOpenedProjectsMenu = project_menu->addMenu(
				embed::getIconPixmap( "project_open_recent" ),
					tr( "Recently opened projects" ) );
	connect( m_recentlyOpenedProjectsMenu, SIGNAL( aboutToShow() ),
			this, SLOT( updateRecentlyOpenedProjectsMenu() ) );
	connect( m_recentlyOpenedProjectsMenu, SIGNAL( triggered( QAction * ) ),
			this, SLOT( openRecentlyOpenedProject( QAction * ) ) );

	project_menu->addAction( embed::getIconPixmap( "project_save" ),
					tr( "&Save" ),
					this, SLOT( saveProject() ),
					Qt::CTRL + Qt::Key_S );

	project_menu->addAction( embed::getIconPixmap( "project_saveas" ),
					tr( "Save &As..." ),
					this, SLOT( saveProjectAs() ),
					Qt::CTRL + Qt::SHIFT + Qt::Key_S );
	project_menu->addSeparator();
	project_menu->addAction( embed::getIconPixmap( "project_import" ),
					tr( "Import..." ),
					engine::getSong(),
					SLOT( importProject() ) );
	project_menu->addAction( embed::getIconPixmap( "project_export" ),
					tr( "E&xport..." ),
					engine::getSong(),
					SLOT( exportProject() ),
					Qt::CTRL + Qt::Key_E );
	project_menu->addSeparator();
	project_menu->addAction( embed::getIconPixmap( "exit" ), tr( "&Quit" ),
					qApp, SLOT( closeAllWindows() ),
					Qt::CTRL + Qt::Key_Q );


	QMenu * edit_menu = new QMenu( this );
	menuBar()->addMenu( edit_menu )->setText( tr( "&Edit" ) );
	edit_menu->addAction( embed::getIconPixmap( "edit_undo" ),
					tr( "Undo" ),
					this, SLOT( undo() ),
					Qt::CTRL + Qt::Key_Z );
	edit_menu->addAction( embed::getIconPixmap( "edit_redo" ),
					tr( "Redo" ),
					this, SLOT( redo() ),
					Qt::CTRL + Qt::Key_R );
	edit_menu->addSeparator();
	edit_menu->addAction( embed::getIconPixmap( "setup_general" ),
					tr( "Settings" ),
					this, SLOT( showSettingsDialog() ) );


	m_toolsMenu = new QMenu( this );
	QVector<plugin::descriptor> pluginDescriptors;
	plugin::getDescriptorsOfAvailPlugins( pluginDescriptors );
	for( QVector<plugin::descriptor>::iterator it =
						pluginDescriptors.begin();
					it != pluginDescriptors.end(); ++it )
	{
		if( it->type == plugin::Tool )
		{
			m_toolsMenu->addAction( it->logo->pixmap(),
							it->displayName );
			m_tools.push_back( tool::instantiate( it->name,
					/*this*/NULL )->createView( this ) );
		}
	}
	if( !m_toolsMenu->isEmpty() )
	{
		menuBar()->addMenu( m_toolsMenu )->setText( tr( "&Tools" ) );
		connect( m_toolsMenu, SIGNAL( triggered( QAction * ) ),
					this, SLOT( showTool( QAction * ) ) );
	}


	// help-popup-menu
	QMenu * help_menu = new QMenu( this );
	menuBar()->addMenu( help_menu )->setText( tr( "&Help" ) );
	// May use offline help
	if( TRUE )
	{
		help_menu->addAction( embed::getIconPixmap( "help" ),
						tr( "Online help" ),
						this, SLOT( browseHelp() ) );
	}
	else
	{
		help_menu->addAction( embed::getIconPixmap( "help" ),
							tr( "Help" ),
							this, SLOT( help() ) );
	}
	help_menu->addAction( embed::getIconPixmap( "whatsthis" ),
					tr( "What's this?" ),
					this, SLOT( enterWhatsThisMode() ) );

	help_menu->addSeparator();
	help_menu->addAction( embed::getIconPixmap( "icon" ), tr( "About" ),
			      this, SLOT( aboutLMMS() ) );

	// create tool-buttons
	toolButton * project_new = new toolButton(
					embed::getIconPixmap( "project_new" ),
					tr( "Create new project" ),
					this, SLOT( createNewProject() ),
							m_toolBar );

	toolButton * project_new_from_template = new toolButton(
			embed::getIconPixmap( "project_new_from_template" ),
				tr( "Create new project from template" ),
					this, SLOT( emptySlot() ),
							m_toolBar );

	m_templatesMenu = new QMenu( project_new_from_template );
	connect( m_templatesMenu, SIGNAL( aboutToShow() ),
					this, SLOT( fillTemplatesMenu() ) );
	connect( m_templatesMenu, SIGNAL( triggered( QAction * ) ),
		this, SLOT( createNewProjectFromTemplate( QAction * ) ) );
	project_new_from_template->setMenu( m_templatesMenu );
	project_new_from_template->setPopupMode( toolButton::InstantPopup );

	toolButton * project_open = new toolButton( 
					embed::getIconPixmap( "project_open" ),
					tr( "Open existing project" ),
					this, SLOT( openProject() ),
								m_toolBar );


	toolButton * project_open_recent = new toolButton( 
				embed::getIconPixmap( "project_open_recent" ),
					tr( "Recently opened project" ),
					this, SLOT( emptySlot() ), m_toolBar );
	project_open_recent->setMenu( m_recentlyOpenedProjectsMenu );
	project_open_recent->setPopupMode( toolButton::InstantPopup );

	toolButton * project_save = new toolButton( 
					embed::getIconPixmap( "project_save" ),
					tr( "Save current project" ),
					this, SLOT( saveProject() ),
								m_toolBar );


	toolButton * project_export = new toolButton( 
				embed::getIconPixmap( "project_export" ),
					tr( "Export current project" ),
					engine::getSong(),
							SLOT( exportProject() ),
								m_toolBar );


	m_toolBarLayout->setColumnMinimumWidth( 0, 5 );
	m_toolBarLayout->addWidget( project_new, 0, 1 );
	m_toolBarLayout->addWidget( project_new_from_template, 0, 2 );
	m_toolBarLayout->addWidget( project_open, 0, 3 );
	m_toolBarLayout->addWidget( project_open_recent, 0, 4 );
	m_toolBarLayout->addWidget( project_save, 0, 5 );
	m_toolBarLayout->addWidget( project_export, 0, 6 );



	// window-toolbar
	toolButton * song_editor_window = new toolButton(
					embed::getIconPixmap( "songeditor" ),
					tr( "Show/hide Song-Editor" ) + " (F5)",
					this, SLOT( toggleSongEditorWin() ),
								m_toolBar );
	song_editor_window->setShortcut( Qt::Key_F5 );
	song_editor_window->setWhatsThis(
		tr( "By pressing this button, you can show or hide the "
			"Song-Editor. With the help of the Song-Editor you can "
			"edit song-playlist and specify when which track "
			"should be played. "
			"You can also insert and move samples (e.g. "
			"rap-samples) directly into the playlist." ) );


	toolButton * bb_editor_window = new toolButton(
					embed::getIconPixmap( "bb_track" ),
					tr( "Show/hide Beat+Bassline Editor" ) +
									" (F6)",
					this, SLOT( toggleBBEditorWin() ),
								m_toolBar );
	bb_editor_window->setShortcut( Qt::Key_F6 );
	bb_editor_window->setWhatsThis(
		tr( "By pressing this button, you can show or hide the "
			"Beat+Bassline Editor. The Beat+Bassline Editor is "
			"needed for creating beats, opening, adding and "
			"removing channels, cutting, copying and pasting "
			"beat- and bassline-patterns and other things like "
			"that." ) );


	toolButton * piano_roll_window = new toolButton(
						embed::getIconPixmap( "piano" ),
						tr( "Show/hide Piano-Roll" ) +
									" (F7)",
					this, SLOT( togglePianoRollWin() ),
								m_toolBar );
	piano_roll_window->setShortcut( Qt::Key_F7 );
	piano_roll_window->setWhatsThis(
			tr( "By pressing this button, you can show or hide the "
				"Piano-Roll. With the help of the Piano-Roll "
				"you can edit melody-patterns in an easy way."
				) );

	toolButton * automation_editor_window = new toolButton(
					embed::getIconPixmap( "automation" ),
					tr( "Show/hide Automation Editor" ) +
									" (F8)",
					this,
					SLOT( toggleAutomationEditorWin() ),
					m_toolBar );
	automation_editor_window->setShortcut( Qt::Key_F8 );
	automation_editor_window->setWhatsThis(
			tr( "By pressing this button, you can show or hide the "
				"Automation Editor. With the help of the "
				"Automation Editor you can edit dynamic values "
				"in an easy way."
				) );

	toolButton * fx_mixer_window = new toolButton(
					embed::getIconPixmap( "fx_mixer" ),
					tr( "Show/hide FX Mixer" ) + " (F9)",
					this, SLOT( toggleFxMixerWin() ),
					m_toolBar );
	fx_mixer_window->setShortcut( Qt::Key_F9 );
	fx_mixer_window->setWhatsThis( 
		tr( "By pressing this button, you can show or hide the "
			"FX Mixer. The FX Mixer is a very powerful tool "
			"for managing effects for your song. You can insert "
			"effects into different effect-channels." ) );

	toolButton * project_notes_window = new toolButton(
					embed::getIconPixmap( "project_notes" ),
					tr( "Show/hide project notes" ) +
								" (F10)",
					this, SLOT( toggleProjectNotesWin() ),
								m_toolBar );
	project_notes_window->setShortcut( Qt::Key_F10 );
	project_notes_window->setWhatsThis(
		tr( "By pressing this button, you can show or hide the "
			"project notes window. In this window you can put "
			"down your project notes.") );

	toolButton * controllers_window = new toolButton(
					embed::getIconPixmap( "controller" ),
					tr( "Show/hide controller rack" ) +
								" (F11)",
					this, SLOT( toggleControllerRack() ),
								m_toolBar );
	controllers_window->setShortcut( Qt::Key_F11 );

	m_toolBarLayout->addWidget( song_editor_window, 1, 1 );
	m_toolBarLayout->addWidget( bb_editor_window, 1, 2 );
	m_toolBarLayout->addWidget( piano_roll_window, 1, 3 );
	m_toolBarLayout->addWidget( automation_editor_window, 1, 4 );
	m_toolBarLayout->addWidget( fx_mixer_window, 1, 5 );
	m_toolBarLayout->addWidget( project_notes_window, 1, 6 );
	m_toolBarLayout->addWidget( controllers_window, 1, 7 );
	m_toolBarLayout->addWidget( controllers_window, 1, 7 );
	m_toolBarLayout->setColumnStretch( 100, 1 );


	// setup-dialog opened before?
	if( !configManager::inst()->value( "app", "configured" ).toInt() )
	{
		configManager::inst()->setValue( "app", "configured", "1" );
		// no, so show it that user can setup everything
		setupDialog sd;
		sd.exec();
	}
	// look whether mixer could use a audio-interface beside audioDummy
	else if( engine::getMixer()->audioDevName() == audioDummy::name() )
	{
		// no, so we offer setup-dialog with audio-settings...
		setupDialog sd( setupDialog::AudioSettings );
		sd.exec();
	}
}




int mainWindow::addWidgetToToolBar( QWidget * _w, int _row, int _col )
{
	int col = ( _col == -1 ) ? m_toolBarLayout->columnCount() + 7 : _col;
	if( _w->height() > 32 || _row == -1 )
	{
		m_toolBarLayout->addWidget( _w, 0, col, 2, 1 );
	}
	else
	{
		m_toolBarLayout->addWidget( _w, _row, col );
	}
	return( col );
}




void mainWindow::addSpacingToToolBar( int _size )
{
	m_toolBarLayout->setColumnMinimumWidth( m_toolBarLayout->columnCount() +
								7, _size );
}




void mainWindow::resetWindowTitle( void )
{
	QString title = "";
	if( engine::getSong()->projectFileName() != "" )
	{
		title = QFileInfo( engine::getSong()->projectFileName()
							).completeBaseName();
	}
	if( title == "" )
	{
		title = tr( "Untitled" );
	}
	if( engine::getSong()->isModified() )
	{
		title += '*';
	}
	setWindowTitle( title + " - " + tr( "LMMS %1" ).arg( LMMS_VERSION ) );
}




bool mainWindow::mayChangeProject( void )
{
	if( !engine::getSong()->isModified() )
	{
		return( TRUE );
	}

	QMessageBox mb ( tr( "Project not saved" ),
				tr( "The current project was modified since "
					"last saving. Do you want to save it "
								"now?" ),
				QMessageBox::Question,
				QMessageBox::Yes,
				QMessageBox::No,
				QMessageBox::Cancel,
				this );
	int answer = mb.exec();

	if( answer == QMessageBox::Yes )
	{
		return( saveProject() );
	}
	else if( answer == QMessageBox::No )
	{
		return( TRUE );
	}

	return( FALSE );
}




void mainWindow::clearKeyModifiers( void )
{
	m_keyMods.m_ctrl = FALSE;
	m_keyMods.m_shift = FALSE;
	m_keyMods.m_alt = FALSE;
}




void mainWindow::saveWidgetState( QWidget * _w, QDomElement & _de )
{
	if( _w->parentWidget() != NULL &&
			_w->parentWidget()->inherits( "QMdiSubWindow" ) )
	{
		_w = _w->parentWidget();
	}
	
	_de.setAttribute( "x", _w->x() );
	_de.setAttribute( "y", _w->y() );
	_de.setAttribute( "visible", _w->isVisible() );

	_de.setAttribute( "width", _w->width() );
	_de.setAttribute( "height", _w->height() );
}




void mainWindow::restoreWidgetState( QWidget * _w, const QDomElement & _de )
{
	QRect r( tMax( 0, _de.attribute( "x" ).toInt() ),
			tMax( 0, _de.attribute( "y" ).toInt() ),
			tMax( 100, _de.attribute( "width" ).toInt() ),
			tMax( 100, _de.attribute( "height" ).toInt() ) );
	if( !r.isNull())
	{
		if ( _w->parentWidget() != NULL &&
			_w->parentWidget()->inherits( "QMdiSubWindow" ) )
		{
			_w = _w->parentWidget();
		}

		_w->resize( r.size() );
		_w->move( r.topLeft() );
		_w->setVisible( _de.attribute( "visible" ).toInt() );
	}
}




void mainWindow::createNewProject( void )
{
	if( mayChangeProject() )
	{
		engine::getSong()->createNewProject();
	}
}




void mainWindow::createNewProjectFromTemplate( QAction * _idx )
{
	if( m_templatesMenu != NULL && mayChangeProject() )
	{
		QString dir_base = m_templatesMenu->actions().indexOf( _idx )
						>= m_custom_templates_count ?
				configManager::inst()->factoryProjectsDir() :
				configManager::inst()->userProjectsDir();
		engine::getSong()->createNewProjectFromTemplate(
			dir_base + "templates/" + _idx->text() + ".mpt" );
	}
}





void mainWindow::openProject( void )
{
	if( mayChangeProject() )
	{
		QFileDialog ofd( this, tr( "Open project" ), "",
			tr( "MultiMedia Project (*.mmp *.mmpz *.xml)" ) );
		ofd.setDirectory( configManager::inst()->userProjectsDir() );
		ofd.setFileMode( QFileDialog::ExistingFiles );
		if( ofd.exec () == QDialog::Accepted &&
						!ofd.selectedFiles().isEmpty() )
		{
			engine::getSong()->loadProject(
						ofd.selectedFiles()[0] );
		}
	}
}




void mainWindow::updateRecentlyOpenedProjectsMenu( void )
{
	m_recentlyOpenedProjectsMenu->clear();
	QStringList rup = configManager::inst()->recentlyOpenedProjects();
	for( QStringList::iterator it = rup.begin(); it != rup.end(); ++it )
	{
		m_recentlyOpenedProjectsMenu->addAction(
				embed::getIconPixmap( "project_file" ), *it );
	}
}




void mainWindow::openRecentlyOpenedProject( QAction * _action )
{
	const QString & f = _action->text();
	engine::getSong()->loadProject( f );
	configManager::inst()->addRecentlyOpenedProject( f );
}




bool mainWindow::saveProject( void )
{
	if( engine::getSong()->projectFileName() == "" )
	{
		return( saveProjectAs() );
	}
	else
	{
		engine::getSong()->saveProject();
	}
	return( TRUE );
}




bool mainWindow::saveProjectAs( void )
{
	QFileDialog sfd( this, tr( "Save project" ), "",
			tr( "MultiMedia Project (*.mmp *.mmpz);;"
				"MultiMedia Project Template (*.mpt)" ) );
	sfd.setAcceptMode( QFileDialog::AcceptSave );
	sfd.setFileMode( QFileDialog::AnyFile );
	QString f = engine::getSong()->projectFileName();
	if( f != "" )
	{
		sfd.setDirectory( QFileInfo( f ).absolutePath() );
		sfd.selectFile( QFileInfo( f ).fileName() );
	}
	else
	{
		sfd.setDirectory( configManager::inst()->userProjectsDir() );
	}

	if( sfd.exec () == QFileDialog::Accepted &&
		!sfd.selectedFiles().isEmpty() && sfd.selectedFiles()[0] != "" )
	{
		engine::getSong()->saveProjectAs(
						sfd.selectedFiles()[0] );
		return( TRUE );
	}
	return( FALSE );
}




void mainWindow::showSettingsDialog( void )
{
	setupDialog sd;
	sd.exec();
}




void mainWindow::aboutLMMS( void )
{
	aboutDialog().exec();
}




void mainWindow::help( void )
{
	QMessageBox::information( this, tr( "Help not available" ),
				  tr( "Currently there's no help "
						  "available in LMMS.\n"
						  "Please visit "
						  "http://lmms.sf.net/wiki "
						  "for documentation on LMMS." ),
				  QMessageBox::Ok );
}




void mainWindow::toggleWindow( QWidget * _w )
{
	if( m_workspace->activeSubWindow() != _w->parentWidget()
				|| _w->parentWidget()->isHidden() )
	{
		_w->parentWidget()->show();
		_w->show();
		_w->setFocus();
	}
	else
	{
		_w->parentWidget()->hide();
	}
}




void mainWindow::toggleBBEditorWin( void )
{
	toggleWindow( engine::getBBEditor() );
}




void mainWindow::toggleSongEditorWin( void )
{
	toggleWindow( engine::getSongEditor() );
}




void mainWindow::toggleProjectNotesWin( void )
{
	toggleWindow( engine::getProjectNotes() );
}




void mainWindow::togglePianoRollWin( void )
{
	toggleWindow( engine::getPianoRoll() );
}




void mainWindow::toggleAutomationEditorWin( void )
{
	toggleWindow( engine::getAutomationEditor() );
}




void mainWindow::toggleFxMixerWin( void )
{
	toggleWindow( engine::getFxMixerView() );
}




void mainWindow::toggleControllerRack( void )
{
	toggleWindow( engine::getControllerRackView() );
}




void mainWindow::undo( void )
{
	engine::getProjectJournal()->undo();
}




void mainWindow::redo( void )
{
	engine::getProjectJournal()->redo();
}




void mainWindow::closeEvent( QCloseEvent * _ce )
{
	if( mayChangeProject() )
	{
		_ce->accept();
	}
	else
	{
		_ce->ignore();
	}
}




void mainWindow::focusOutEvent( QFocusEvent * _fe )
{
	// when loosing focus we do not receive key-(release!)-events anymore,
	// so we might miss release-events of one the modifiers we're watching!
	clearKeyModifiers();
	QMainWindow::leaveEvent( _fe );
}




void mainWindow::keyPressEvent( QKeyEvent * _ke )
{
	switch( _ke->key() )
	{
		case Qt::Key_Control: m_keyMods.m_ctrl = TRUE; break;
		case Qt::Key_Shift: m_keyMods.m_shift = TRUE; break;
		case Qt::Key_Alt: m_keyMods.m_alt = TRUE; break;
		default:
			QMainWindow::keyPressEvent( _ke );
	}
}




void mainWindow::keyReleaseEvent( QKeyEvent * _ke )
{
	switch( _ke->key() )
	{
		case Qt::Key_Control: m_keyMods.m_ctrl = FALSE; break;
		case Qt::Key_Shift: m_keyMods.m_shift = FALSE; break;
		case Qt::Key_Alt: m_keyMods.m_alt = FALSE; break;
		default:
			QMainWindow::keyReleaseEvent( _ke );
	}
}




void mainWindow::fillTemplatesMenu( void )
{
	m_templatesMenu->clear();

	QDir user_d( configManager::inst()->userProjectsDir() + "templates" );
	QStringList templates = user_d.entryList( QStringList( "*.mpt" ),
						QDir::Files | QDir::Readable );

	m_custom_templates_count = templates.count();
	for( QStringList::iterator it = templates.begin();
						it != templates.end(); ++it )
	{
		m_templatesMenu->addAction(
					embed::getIconPixmap( "project_file" ),
					( *it ).left( ( *it ).length() - 4 ) );
	}

	QDir d( configManager::inst()->factoryProjectsDir() + "templates" );
	templates = d.entryList( QStringList( "*.mpt" ),
						QDir::Files | QDir::Readable );


	if( m_custom_templates_count > 0 && !templates.isEmpty() )
	{
		m_templatesMenu->addSeparator();
	}
	for( QStringList::iterator it = templates.begin();
						it != templates.end(); ++it )
	{
		m_templatesMenu->addAction(
					embed::getIconPixmap( "project_file" ),
					( *it ).left( ( *it ).length() - 4 ) );
	}
}




void mainWindow::showTool( QAction * _idx )
{
	pluginView * p = m_tools[m_toolsMenu->actions().indexOf( _idx )];
	p->show();
	p->parentWidget()->show();
	p->setFocus();
}




void mainWindow::browseHelp( void )
{
	// file:// alternative for offline help
	QString url = "http://lmms.sf.net/wiki/index.php?title=Main_Page";
	QDesktopServices::openUrl( url );
	// TODO: Handle error
}




#include "moc_main_window.cxx"


#endif
