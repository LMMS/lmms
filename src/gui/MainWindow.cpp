/*
 * MainWindow.cpp - implementation of LMMS-main-window
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtXml/QDomElement>
#include <QtCore/QUrl>
#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QSplitter>
#include <QtGui/QWhatsThis>

#include "lmmsversion.h"
#include "MainWindow.h"
#include "bb_editor.h"
#include "SongEditor.h"
#include "song.h"
#include "PianoRoll.h"
#include "embed.h"
#include "engine.h"
#include "FxMixerView.h"
#include "InstrumentTrack.h"
#include "PianoView.h"
#include "about_dialog.h"
#include "ControllerRackView.h"
#include "FileBrowser.h"
#include "plugin_browser.h"
#include "SideBar.h"
#include "config_mgr.h"
#include "Mixer.h"
#include "PluginView.h"
#include "project_notes.h"
#include "setup_dialog.h"
#include "AudioDummy.h"
#include "ToolPlugin.h"
#include "tool_button.h"
#include "ProjectJournal.h"
#include "AutomationEditor.h"
#include "templates.h"
#include "FileDialog.h"
#include "VersionedSaveDialog.h"




MainWindow::MainWindow() :
	m_workspace( NULL ),
	m_templatesMenu( NULL ),
	m_recentlyOpenedProjectsMenu( NULL ),
	m_toolsMenu( NULL ),
	m_autoSaveTimer( this )
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

	SideBar * sideBar = new SideBar( Qt::Vertical, w );

	QSplitter * splitter = new QSplitter( Qt::Horizontal, w );
	splitter->setChildrenCollapsible( FALSE );

	QString wdir = configManager::inst()->workingDir();
	sideBar->appendTab( new PluginBrowser( splitter ) );
	sideBar->appendTab( new FileBrowser(
				configManager::inst()->userProjectsDir() + "*" +
				configManager::inst()->factoryProjectsDir(),
					"*.mmp *.mmpz *.xml *.mid *.flp",
							tr( "My projects" ),
					embed::getIconPixmap( "project_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter ) );
	sideBar->appendTab( new FileBrowser(
				configManager::inst()->userSamplesDir() + "*" +
				configManager::inst()->factorySamplesDir(),
					"*", tr( "My samples" ),
					embed::getIconPixmap( "sample_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter ) );
	sideBar->appendTab( new FileBrowser(
				configManager::inst()->userPresetsDir() + "*" +
				configManager::inst()->factoryPresetsDir(),
					"*.xpf *.cs.xml *.xiz",
					tr( "My presets" ),
					embed::getIconPixmap( "preset_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter ) );
	sideBar->appendTab( new FileBrowser( QDir::homePath(), "*",
							tr( "My home" ),
					embed::getIconPixmap( "home" ).transformed( QTransform().rotate( 90 ) ),
							splitter ) );

	QStringList root_paths;
#ifdef LMMS_BUILD_APPLE
	root_paths += "/Volumes";
#else
	QFileInfoList drives = QDir::drives();
	foreach( const QFileInfo & drive, drives )
	{
		root_paths += drive.absolutePath();
	}
#endif
	sideBar->appendTab( new FileBrowser( root_paths.join( "*" ), "*",
#ifdef LMMS_BUILD_WIN32
							tr( "My computer" ),
#elif defined(LMMS_BUILD_APPLE)
							tr( "Volumes" ),
#else
							tr( "Root directory" ),
#endif

					embed::getIconPixmap( "computer" ).transformed( QTransform().rotate( 90 ) ),
							splitter,
#ifdef LMMS_BUILD_WIN32
							true
#else
							false
#endif
								) );

	m_workspace = new QMdiArea( splitter );

	// Load background
	QString bgArtwork = configManager::inst()->backgroundArtwork();
	QImage bgImage;
	if( !bgArtwork.isEmpty() )
	{
		bgImage = QImage( bgArtwork );
	}
	if( !bgImage.isNull() )
	{
		m_workspace->setBackground( bgImage );
	}
	else
	{
		m_workspace->setBackground( Qt::NoBrush );
	}

	m_workspace->setOption( QMdiArea::DontMaximizeSubWindowOnActivation );
	m_workspace->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_workspace->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );

	hbox->addWidget( sideBar );
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


	m_updateTimer.start( 1000 / 20, this );	// 20 fps

	if( configManager::inst()->value( "ui", "enableautosave" ).toInt() )
	{
		// connect auto save
		connect(&m_autoSaveTimer, SIGNAL(timeout()), this, SLOT(autoSave()));
		m_autoSaveTimer.start(1000 * 60); // 1 minute
	}

	connect( engine::getSong(), SIGNAL( playbackStateChanged() ),
				this, SLOT( updatePlayPauseIcons() ) );
}




MainWindow::~MainWindow()
{
	for( QList<PluginView *>::iterator it = m_tools.begin();
						it != m_tools.end(); ++it )
	{
		Model * m = ( *it )->model();
		delete *it;
		delete m;
	}
	// TODO: Close tools
	// destroy engine which will do further cleanups etc.
	engine::destroy();
}




void MainWindow::finalize()
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

	project_menu->addAction( embed::getIconPixmap( "project_save" ),
					tr( "Save as new &version" ),
					this, SLOT( saveProjectAsNewVersion() ),
					Qt::CTRL + Qt::ALT + Qt::Key_S );
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
	project_menu->addAction( embed::getIconPixmap( "project_export" ),
					tr( "E&xport tracks..." ),
					engine::getSong(),
					SLOT( exportProjectTracks() ),
					Qt::CTRL + Qt::SHIFT + Qt::Key_E );

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
					Qt::CTRL + Qt::Key_Y );
	edit_menu->addSeparator();
	edit_menu->addAction( embed::getIconPixmap( "setup_general" ),
					tr( "Settings" ),
					this, SLOT( showSettingsDialog() ) );


	m_toolsMenu = new QMenu( this );
	Plugin::DescriptorList pluginDescriptors;
	Plugin::getDescriptorsOfAvailPlugins( pluginDescriptors );
	for( Plugin::DescriptorList::ConstIterator it = pluginDescriptors.begin();
										it != pluginDescriptors.end(); ++it )
	{
		if( it->type == Plugin::Tool )
		{
			m_toolsMenu->addAction( it->logo->pixmap(),
							it->displayName );
			m_tools.push_back( ToolPlugin::instantiate( it->name,
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

	toolButton * whatsthis = new toolButton(
				embed::getIconPixmap( "whatsthis" ),
					tr( "What's this?" ),
					this, SLOT( enterWhatsThisMode() ),
								m_toolBar );


	m_toolBarLayout->setColumnMinimumWidth( 0, 5 );
	m_toolBarLayout->addWidget( project_new, 0, 1 );
	m_toolBarLayout->addWidget( project_new_from_template, 0, 2 );
	m_toolBarLayout->addWidget( project_open, 0, 3 );
	m_toolBarLayout->addWidget( project_open_recent, 0, 4 );
	m_toolBarLayout->addWidget( project_save, 0, 5 );
	m_toolBarLayout->addWidget( project_export, 0, 6 );
	m_toolBarLayout->addWidget( whatsthis, 0, 7 );


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
			"rap samples) directly into the playlist." ) );


	toolButton * bb_editor_window = new toolButton(
					embed::getIconPixmap( "bb_track_btn" ),
					tr( "Show/hide Beat+Bassline Editor" ) +
									" (F6)",
					this, SLOT( toggleBBEditorWin() ),
								m_toolBar );
	bb_editor_window->setShortcut( Qt::Key_F6 );
	bb_editor_window->setWhatsThis(
		tr( "By pressing this button, you can show or hide the "
			"Beat+Bassline Editor. The Beat+Bassline Editor is "
			"needed for creating beats, and for opening, adding, and "
			"removing channels, and for cutting, copying and pasting "
			"beat and bassline-patterns, and for other things like "
			"that." ) );


	toolButton * piano_roll_window = new toolButton(
						embed::getIconPixmap( "piano" ),
						tr( "Show/hide Piano-Roll" ) +
									" (F7)",
					this, SLOT( togglePianoRollWin() ),
								m_toolBar );
	piano_roll_window->setShortcut( Qt::Key_F7 );
	piano_roll_window->setWhatsThis(
			tr( "Click here to show or hide the "
				"Piano-Roll. With the help of the Piano-Roll "
				"you can edit melodies in an easy way."
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
			tr( "Click here to show or hide the "
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
		tr( "Click here to show or hide the "
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
		tr( "Click here to show or hide the "
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
	m_toolBarLayout->setColumnStretch( 100, 1 );

	// setup-dialog opened before?
	if( !configManager::inst()->value( "app", "configured" ).toInt() )
	{
		configManager::inst()->setValue( "app", "configured", "1" );
		// no, so show it that user can setup everything
		setupDialog sd;
		sd.exec();
	}
	// look whether mixer could use a audio-interface beside AudioDummy
	else if( engine::mixer()->audioDevName() == AudioDummy::name() )
	{
		// no, so we offer setup-dialog with audio-settings...
		setupDialog sd( setupDialog::AudioSettings );
		sd.exec();
	}
	// reset window title every time we change the state of a subwindow to show the correct title
	foreach( QMdiSubWindow * subWindow, workspace()->subWindowList() )
	{
		connect( subWindow, SIGNAL( windowStateChanged(Qt::WindowStates,Qt::WindowStates) ), this, SLOT( resetWindowTitle() ) );
	}
}




int MainWindow::addWidgetToToolBar( QWidget * _w, int _row, int _col )
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




void MainWindow::addSpacingToToolBar( int _size )
{
	m_toolBarLayout->setColumnMinimumWidth( m_toolBarLayout->columnCount() +
								7, _size );
}




void MainWindow::resetWindowTitle()
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




bool MainWindow::mayChangeProject()
{
	engine::getSong()->stop();

	if( !engine::getSong()->isModified() )
	{
		return( TRUE );
	}

	QMessageBox mb( tr( "Project not saved" ),
				tr( "The current project was modified since "
					"last saving. Do you want to save it "
								"now?" ),
				QMessageBox::Question,
				QMessageBox::Save,
				QMessageBox::Discard,
				QMessageBox::Cancel,
				this );
	int answer = mb.exec();

	if( answer == QMessageBox::Save )
	{
		return( saveProject() );
	}
	else if( answer == QMessageBox::Discard )
	{
		return( TRUE );
	}

	return( FALSE );
}




void MainWindow::clearKeyModifiers()
{
	m_keyMods.m_ctrl = FALSE;
	m_keyMods.m_shift = FALSE;
	m_keyMods.m_alt = FALSE;
}




void MainWindow::saveWidgetState( QWidget * _w, QDomElement & _de )
{
	if( _w->parentWidget() != NULL &&
			_w->parentWidget()->inherits( "QMdiSubWindow" ) )
	{
		_w = _w->parentWidget();
	}

	_de.setAttribute( "x", _w->x() );
	_de.setAttribute( "y", _w->y() );
	_de.setAttribute( "visible", _w->isVisible() );
	_de.setAttribute( "minimized", _w->isMinimized() );
	_de.setAttribute( "maximized", _w->isMaximized() );

	_de.setAttribute( "width", _w->width() );
	_de.setAttribute( "height", _w->height() );
}




void MainWindow::restoreWidgetState( QWidget * _w, const QDomElement & _de )
{
	QRect r( qMax( 0, _de.attribute( "x" ).toInt() ),
			qMax( 0, _de.attribute( "y" ).toInt() ),
			qMax( 100, _de.attribute( "width" ).toInt() ),
			qMax( 100, _de.attribute( "height" ).toInt() ) );
	if( _de.hasAttribute( "visible" ) && !r.isNull() )
	{
		if ( _w->parentWidget() != NULL &&
			_w->parentWidget()->inherits( "QMdiSubWindow" ) )
		{
			_w = _w->parentWidget();
		}

		_w->resize( r.size() );
		_w->move( r.topLeft() );
		_w->setVisible( _de.attribute( "visible" ).toInt() );
		_w->setWindowState( _de.attribute( "minimized" ).toInt() ?
				( _w->windowState() | Qt::WindowMinimized ) :
				( _w->windowState() & ~Qt::WindowMinimized ) );
		_w->setWindowState( _de.attribute( "maximized" ).toInt() ?
				( _w->windowState() | Qt::WindowMaximized ) :
				( _w->windowState() & ~Qt::WindowMaximized ) );
	}
}



void MainWindow::emptySlot()
{
}



void MainWindow::enterWhatsThisMode()
{
	QWhatsThis::enterWhatsThisMode();
}



void MainWindow::createNewProject()
{
	if( mayChangeProject() )
	{
		engine::getSong()->createNewProject();
	}
}




void MainWindow::createNewProjectFromTemplate( QAction * _idx )
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




void MainWindow::openProject()
{
	if( mayChangeProject() )
	{
		FileDialog ofd( this, tr( "Open project" ), "", tr( "LMMS (*.mmp *.mmpz)" ) );

		ofd.setDirectory( configManager::inst()->userProjectsDir() );
		ofd.setFileMode( FileDialog::ExistingFiles );
		if( ofd.exec () == QDialog::Accepted &&
						!ofd.selectedFiles().isEmpty() )
		{
			setCursor( Qt::WaitCursor );
			engine::getSong()->loadProject(
						ofd.selectedFiles()[0] );
			setCursor( Qt::ArrowCursor );
		}
	}
}




void MainWindow::updateRecentlyOpenedProjectsMenu()
{
	m_recentlyOpenedProjectsMenu->clear();
	QStringList rup = configManager::inst()->recentlyOpenedProjects();
	for( QStringList::iterator it = rup.begin(); it != rup.end(); ++it )
	{
		m_recentlyOpenedProjectsMenu->addAction(
				embed::getIconPixmap( "project_file" ), *it );
	}
}




void MainWindow::openRecentlyOpenedProject( QAction * _action )
{
	if ( mayChangeProject() )
	{
		const QString & f = _action->text();
		setCursor( Qt::WaitCursor );
		engine::getSong()->loadProject( f );
		configManager::inst()->addRecentlyOpenedProject( f );
		setCursor( Qt::ArrowCursor );
	}
}




bool MainWindow::saveProject()
{
	if( engine::getSong()->projectFileName() == "" )
	{
		return( saveProjectAs() );
	}
	else
	{
		engine::getSong()->guiSaveProject();
	}
	return( TRUE );
}




bool MainWindow::saveProjectAs()
{
	VersionedSaveDialog sfd( this, tr( "Save project" ), "",
			tr( "LMMS Project (*.mmpz *.mmp);;"
				"LMMS Project Template (*.mpt)" ) );
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

	if( sfd.exec () == FileDialog::Accepted &&
		!sfd.selectedFiles().isEmpty() && sfd.selectedFiles()[0] != "" )
	{
		engine::getSong()->guiSaveProjectAs(
						sfd.selectedFiles()[0] );
		return( TRUE );
	}
	return( FALSE );
}




bool MainWindow::saveProjectAsNewVersion()
{
	QString fileName = engine::getSong()->projectFileName();
	if( fileName == "" )
	{
		return saveProjectAs();
	}
	else
	{
		do 		VersionedSaveDialog::changeFileNameVersion( fileName, true );
		while 	( QFile( fileName ).exists() );

		engine::getSong()->guiSaveProjectAs( fileName );
		return true;
	}
}




void MainWindow::showSettingsDialog()
{
	setupDialog sd;
	sd.exec();
}




void MainWindow::aboutLMMS()
{
	aboutDialog().exec();
}




void MainWindow::help()
{
	QMessageBox::information( this, tr( "Help not available" ),
				  tr( "Currently there's no help "
						  "available in LMMS.\n"
						  "Please visit "
						  "http://lmms.sf.net/wiki "
						  "for documentation on LMMS." ),
				  QMessageBox::Ok );
}




void MainWindow::toggleWindow( QWidget *window, bool forceShow )
{
	QWidget *parent = window->parentWidget();

	if( forceShow ||
		m_workspace->activeSubWindow() != parent ||
		parent->isHidden() )
	{
		parent->show();
		window->show();
		window->setFocus();
	}
	else
	{
		parent->hide();
	}

	// Workaround for Qt Bug #260116
	m_workspace->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_workspace->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_workspace->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_workspace->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
}




void MainWindow::toggleBBEditorWin( bool forceShow )
{
	toggleWindow( engine::getBBEditor(), forceShow );
}




void MainWindow::toggleSongEditorWin()
{
	toggleWindow( engine::songEditor() );
}




void MainWindow::toggleProjectNotesWin()
{
	toggleWindow( engine::getProjectNotes() );
}




void MainWindow::togglePianoRollWin()
{
	toggleWindow( engine::pianoRoll() );
}




void MainWindow::toggleAutomationEditorWin()
{
	toggleWindow( engine::automationEditor() );
}




void MainWindow::toggleFxMixerWin()
{
	toggleWindow( engine::fxMixerView() );
}




void MainWindow::toggleControllerRack()
{
	toggleWindow( engine::getControllerRackView() );
}




void MainWindow::updatePlayPauseIcons()
{
	engine::songEditor()->setPauseIcon( false );
	engine::automationEditor()->setPauseIcon( false );
	engine::getBBEditor()->setPauseIcon( false );
	engine::pianoRoll()->setPauseIcon( false );

	if( engine::getSong()->isPlaying() )
	{
		switch( engine::getSong()->playMode() )
		{
			case song::Mode_PlaySong:
				engine::songEditor()->setPauseIcon( true );
				break;

			case song::Mode_PlayAutomationPattern:
				engine::automationEditor()->setPauseIcon( true );
				break;

			case song::Mode_PlayBB:
				engine::getBBEditor()->setPauseIcon( true );
				break;

			case song::Mode_PlayPattern:
				engine::pianoRoll()->setPauseIcon( true );
				break;

			default:
				break;
		}
	}
}




void MainWindow::undo()
{
	engine::projectJournal()->undo();
}




void MainWindow::redo()
{
	engine::projectJournal()->redo();
}




void MainWindow::closeEvent( QCloseEvent * _ce )
{
	if( mayChangeProject() )
	{
		// delete recovery file
		QDir working(configManager::inst()->workingDir());
		working.remove("recover.mmp");
		_ce->accept();
	}
	else
	{
		_ce->ignore();
	}
}




void MainWindow::focusOutEvent( QFocusEvent * _fe )
{
	// when loosing focus we do not receive key-(release!)-events anymore,
	// so we might miss release-events of one the modifiers we're watching!
	clearKeyModifiers();
	QMainWindow::leaveEvent( _fe );
}




void MainWindow::keyPressEvent( QKeyEvent * _ke )
{
	switch( _ke->key() )
	{
		case Qt::Key_Control: m_keyMods.m_ctrl = TRUE; break;
		case Qt::Key_Shift: m_keyMods.m_shift = TRUE; break;
		case Qt::Key_Alt: m_keyMods.m_alt = TRUE; break;
		default:
		{
			InstrumentTrackWindow * w =
						InstrumentTrackView::topLevelInstrumentTrackWindow();
			if( w )
			{
				w->pianoView()->keyPressEvent( _ke );
			}
			if( !_ke->isAccepted() )
			{
				QMainWindow::keyPressEvent( _ke );
			}
		}
	}
}




void MainWindow::keyReleaseEvent( QKeyEvent * _ke )
{
	switch( _ke->key() )
	{
		case Qt::Key_Control: m_keyMods.m_ctrl = FALSE; break;
		case Qt::Key_Shift: m_keyMods.m_shift = FALSE; break;
		case Qt::Key_Alt: m_keyMods.m_alt = FALSE; break;
		default:
			if( InstrumentTrackView::topLevelInstrumentTrackWindow() )
			{
				InstrumentTrackView::topLevelInstrumentTrackWindow()->
					pianoView()->keyReleaseEvent( _ke );
			}
			if( !_ke->isAccepted() )
			{
				QMainWindow::keyReleaseEvent( _ke );
			}
	}
}




void MainWindow::timerEvent( QTimerEvent * _te)
{
	emit periodicUpdate();
}




void MainWindow::fillTemplatesMenu()
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




void MainWindow::showTool( QAction * _idx )
{
	PluginView * p = m_tools[m_toolsMenu->actions().indexOf( _idx )];
	p->show();
	p->parentWidget()->show();
	p->setFocus();
}




void MainWindow::browseHelp()
{
	// file:// alternative for offline help
	QString url = "http://lmms.sf.net/wiki/index.php?title=Main_Page";
	QDesktopServices::openUrl( url );
	// TODO: Handle error
}




void MainWindow::autoSave()
{
	if( !( engine::getSong()->isPlaying() ||
			engine::getSong()->isExporting() ) )
	{
		QDir work(configManager::inst()->workingDir());
		engine::getSong()->saveProjectFile(work.absoluteFilePath("recover.mmp"));
	}
	else
	{
		// try again in 10 seconds
		QTimer::singleShot( 10*1000, this, SLOT( autoSave() ) );
	}
}


#include "moc_MainWindow.cxx"

