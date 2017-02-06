/*
 * MainWindow.cpp - implementation of LMMS-main-window
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "MainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDomElement>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QSplitter>
#include <QUrl>
#include <QWhatsThis>

#include "AboutDialog.h"
#include "AudioDummy.h"
#include "AutomationEditor.h"
#include "BBEditor.h"
#include "ConfigManager.h"
#include "ControllerRackView.h"
#include "embed.h"
#include "Engine.h"
#include "FileBrowser.h"
#include "FileDialog.h"
#include "FxMixerView.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "lmmsversion.h"
#include "Mixer.h"
#include "PianoRoll.h"
#include "PianoView.h"
#include "PluginBrowser.h"
#include "PluginFactory.h"
#include "PluginView.h"
#include "ProjectJournal.h"
#include "ProjectNotes.h"
#include "SetupDialog.h"
#include "SideBar.h"
#include "Song.h"
#include "SongEditor.h"
#include "SubWindow.h"
#include "templates.h"
#include "ToolButton.h"
#include "ToolPlugin.h"
#include "VersionedSaveDialog.h"




MainWindow::MainWindow() :
	m_workspace( NULL ),
	m_templatesMenu( NULL ),
	m_recentlyOpenedProjectsMenu( NULL ),
	m_toolsMenu( NULL ),
	m_autoSaveTimer( this ),
	m_viewMenu( NULL ),
	m_metronomeToggle( 0 ),
	m_session( Normal )
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
	splitter->setChildrenCollapsible( false );

	ConfigManager* confMgr = ConfigManager::inst();

	emit initProgress(tr("Preparing plugin browser"));
	sideBar->appendTab( new PluginBrowser( splitter ) );
	emit initProgress(tr("Preparing file browsers"));
	sideBar->appendTab( new FileBrowser(
				confMgr->userProjectsDir() + "*" +
				confMgr->factoryProjectsDir(),
					"*.mmp *.mmpz *.xml *.mid",
							tr( "My Projects" ),
					embed::getIconPixmap( "project_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter, false, true ) );
	sideBar->appendTab( new FileBrowser(
				confMgr->userSamplesDir() + "*" +
				confMgr->factorySamplesDir(),
					"*", tr( "My Samples" ),
					embed::getIconPixmap( "sample_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter, false, true ) );
	sideBar->appendTab( new FileBrowser(
				confMgr->userPresetsDir() + "*" +
				confMgr->factoryPresetsDir(),
					"*.xpf *.cs.xml *.xiz",
					tr( "My Presets" ),
					embed::getIconPixmap( "preset_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter , false, true  ) );
	sideBar->appendTab( new FileBrowser( QDir::homePath(), "*",
							tr( "My Home" ),
					embed::getIconPixmap( "home" ).transformed( QTransform().rotate( 90 ) ),
							splitter, false, false ) );


	QStringList root_paths;
	QString title = tr( "Root directory" );
	bool dirs_as_items = false;

#ifdef LMMS_BUILD_APPLE
	title = tr( "Volumes" );
	root_paths += "/Volumes";
#elif defined(LMMS_BUILD_WIN32)
	title = tr( "My Computer" );
	dirs_as_items = true;
#endif

#if ! defined(LMMS_BUILD_APPLE)
	QFileInfoList drives = QDir::drives();
	for( const QFileInfo & drive : drives )
	{
		root_paths += drive.absolutePath();
	}
#endif

	sideBar->appendTab( new FileBrowser( root_paths.join( "*" ), "*", title,
					embed::getIconPixmap( "computer" ).transformed( QTransform().rotate( 90 ) ),
							splitter, dirs_as_items) );

	m_workspace = new QMdiArea( splitter );

	// Load background
	emit initProgress(tr("Loading background artwork"));
	QString bgArtwork = ConfigManager::inst()->backgroundArtwork();
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

	m_updateTimer.start( 1000 / 20, this );  // 20 fps

	if( ConfigManager::inst()->value( "ui", "enableautosave" ).toInt() )
	{
		// connect auto save
		connect(&m_autoSaveTimer, SIGNAL(timeout()), this, SLOT(autoSave()));
		m_autoSaveInterval = ConfigManager::inst()->value(
					"ui", "saveinterval" ).toInt() < 1 ?
						DEFAULT_AUTO_SAVE_INTERVAL :
				ConfigManager::inst()->value(
					"ui", "saveinterval" ).toInt();

		// The auto save function mustn't run until there is a project
		// to save or it will run over recover.mmp if you hesitate at the
		// recover messagebox for a minute. It is now started in main.
		// See autoSaveTimerReset() in MainWindow.h
	}

	connect( Engine::getSong(), SIGNAL( playbackStateChanged() ),
				this, SLOT( updatePlayPauseIcons() ) );
}




MainWindow::~MainWindow()
{
	for( PluginView *view : m_tools )
	{
		delete view->model();
		delete view;
	}
	// TODO: Close tools
	// dependencies are such that the editors must be destroyed BEFORE Song is deletect in Engine::destroy
	//   see issue #2015 on github
	delete gui->automationEditor();
	delete gui->pianoRoll();
	delete gui->songEditor();
	// destroy engine which will do further cleanups etc.
	Engine::destroy();
}




void MainWindow::finalize()
{
	resetWindowTitle();
	setWindowIcon( embed::getIconPixmap( "icon" ) );


	// project-popup-menu
	QMenu * project_menu = new QMenu( this );
	menuBar()->addMenu( project_menu )->setText( tr( "&File" ) );
	project_menu->addAction( embed::getIconPixmap( "project_new" ),
					tr( "&New" ),
					this, SLOT( createNewProject() ),
					QKeySequence::New );

	m_templatesMenu = new QMenu( tr("New from template"), this );
	connect( m_templatesMenu, SIGNAL( aboutToShow() ), SLOT( fillTemplatesMenu() ) );
	connect( m_templatesMenu, SIGNAL( triggered( QAction * ) ),
		 SLOT( createNewProjectFromTemplate( QAction * ) ) );

	project_menu->addMenu(m_templatesMenu);

	project_menu->addAction( embed::getIconPixmap( "project_open" ),
					tr( "&Open..." ),
					this, SLOT( openProject() ),
					QKeySequence::Open );

	m_recentlyOpenedProjectsMenu = project_menu->addMenu(
				embed::getIconPixmap( "project_open_recent" ),
					tr( "&Recently Opened Projects" ) );
	connect( m_recentlyOpenedProjectsMenu, SIGNAL( aboutToShow() ),
			this, SLOT( updateRecentlyOpenedProjectsMenu() ) );
	connect( m_recentlyOpenedProjectsMenu, SIGNAL( triggered( QAction * ) ),
			this, SLOT( openRecentlyOpenedProject( QAction * ) ) );

	project_menu->addAction( embed::getIconPixmap( "project_save" ),
					tr( "&Save" ),
					this, SLOT( saveProject() ),
					QKeySequence::Save );
	project_menu->addAction( embed::getIconPixmap( "project_saveas" ),
					tr( "Save &As..." ),
					this, SLOT( saveProjectAs() ),
					Qt::CTRL + Qt::SHIFT + Qt::Key_S );
	project_menu->addAction( embed::getIconPixmap( "project_save" ),
					tr( "Save as New &Version" ),
					this, SLOT( saveProjectAsNewVersion() ),
					Qt::CTRL + Qt::ALT + Qt::Key_S );

	project_menu->addAction( tr( "Save as default template" ),
				     this, SLOT( saveProjectAsDefaultTemplate() ) );

	project_menu->addSeparator();
	project_menu->addAction( embed::getIconPixmap( "project_import" ),
					tr( "Import..." ),
					Engine::getSong(),
					SLOT( importProject() ) );
	project_menu->addAction( embed::getIconPixmap( "project_export" ),
					tr( "E&xport..." ),
					Engine::getSong(),
					SLOT( exportProject() ),
					Qt::CTRL + Qt::Key_E );
	project_menu->addAction( embed::getIconPixmap( "project_export" ),
					tr( "E&xport Tracks..." ),
					Engine::getSong(),
					SLOT( exportProjectTracks() ),
					Qt::CTRL + Qt::SHIFT + Qt::Key_E );

	// temporarily disabled broken MIDI export				
	/*project_menu->addAction( embed::getIconPixmap( "midi_file" ),
					tr( "Export &MIDI..." ),
					Engine::getSong(),
					SLOT( exportProjectMidi() ),
					Qt::CTRL + Qt::Key_M );*/

	project_menu->addSeparator();
	project_menu->addAction( embed::getIconPixmap( "exit" ), tr( "&Quit" ),
					qApp, SLOT( closeAllWindows() ),
					Qt::CTRL + Qt::Key_Q );


	QMenu * edit_menu = new QMenu( this );
	menuBar()->addMenu( edit_menu )->setText( tr( "&Edit" ) );
	m_undoAction = edit_menu->addAction( embed::getIconPixmap( "edit_undo" ),
					tr( "Undo" ),
					this, SLOT( undo() ),
					QKeySequence::Undo );
	m_redoAction = edit_menu->addAction( embed::getIconPixmap( "edit_redo" ),
					tr( "Redo" ),
					this, SLOT( redo() ),
					QKeySequence::Redo );
	// Ensure that both (Ctrl+Y) and (Ctrl+Shift+Z) activate redo shortcut regardless of OS defaults
	if (QKeySequence(QKeySequence::Redo) != QKeySequence(Qt::CTRL + Qt::Key_Y))
	{
		new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_Y ), this, SLOT(redo()) );
	}
	if (QKeySequence(QKeySequence::Redo) != QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z ))
	{
		new QShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_Z ), this, SLOT(redo()) );
	}

	edit_menu->addSeparator();
	edit_menu->addAction( embed::getIconPixmap( "setup_general" ),
					tr( "Settings" ),
					this, SLOT( showSettingsDialog() ) );
	connect( edit_menu, SIGNAL(aboutToShow()), this, SLOT(updateUndoRedoButtons()) );

	m_viewMenu = new QMenu( this );
	menuBar()->addMenu( m_viewMenu )->setText( tr( "&View" ) );
	connect( m_viewMenu, SIGNAL( aboutToShow() ),
		 this, SLOT( updateViewMenu() ) );
	connect( m_viewMenu, SIGNAL(triggered(QAction*)), this,
		SLOT(updateConfig(QAction*)));


	m_toolsMenu = new QMenu( this );
	for( const Plugin::Descriptor* desc : pluginFactory->descriptors(Plugin::Tool) )
	{
		m_toolsMenu->addAction( desc->logo->pixmap(), desc->displayName );
		m_tools.push_back( ToolPlugin::instantiate( desc->name, /*this*/NULL )
						   ->createView(this) );
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
	if( true )
	{
		help_menu->addAction( embed::getIconPixmap( "help" ),
						tr( "Online Help" ),
						this, SLOT( browseHelp() ) );
	}
	else
	{
		help_menu->addAction( embed::getIconPixmap( "help" ),
							tr( "Help" ),
							this, SLOT( help() ) );
	}
	help_menu->addAction( embed::getIconPixmap( "whatsthis" ),
					tr( "What's This?" ),
					this, SLOT( enterWhatsThisMode() ) );

	help_menu->addSeparator();
	help_menu->addAction( embed::getIconPixmap( "icon" ), tr( "About" ),
				  this, SLOT( aboutLMMS() ) );

	// create tool-buttons
	ToolButton * project_new = new ToolButton(
					embed::getIconPixmap( "project_new" ),
					tr( "Create new project" ),
					this, SLOT( createNewProject() ),
							m_toolBar );

	ToolButton * project_new_from_template = new ToolButton(
			embed::getIconPixmap( "project_new_from_template" ),
				tr( "Create new project from template" ),
					this, SLOT( emptySlot() ),
							m_toolBar );
	project_new_from_template->setMenu( m_templatesMenu );
	project_new_from_template->setPopupMode( ToolButton::InstantPopup );

	ToolButton * project_open = new ToolButton(
					embed::getIconPixmap( "project_open" ),
					tr( "Open existing project" ),
					this, SLOT( openProject() ),
								m_toolBar );


	ToolButton * project_open_recent = new ToolButton(
				embed::getIconPixmap( "project_open_recent" ),
					tr( "Recently opened projects" ),
					this, SLOT( emptySlot() ), m_toolBar );
	project_open_recent->setMenu( m_recentlyOpenedProjectsMenu );
	project_open_recent->setPopupMode( ToolButton::InstantPopup );

	ToolButton * project_save = new ToolButton(
					embed::getIconPixmap( "project_save" ),
					tr( "Save current project" ),
					this, SLOT( saveProject() ),
								m_toolBar );


	ToolButton * project_export = new ToolButton(
				embed::getIconPixmap( "project_export" ),
					tr( "Export current project" ),
					Engine::getSong(),
							SLOT( exportProject() ),
								m_toolBar );

	ToolButton * whatsthis = new ToolButton(
				embed::getIconPixmap( "whatsthis" ),
					tr( "What's this?" ),
					this, SLOT( enterWhatsThisMode() ),
								m_toolBar );

	m_metronomeToggle = new ToolButton(
				embed::getIconPixmap( "metronome" ),
				tr( "Toggle metronome" ),
				this, SLOT( onToggleMetronome() ),
							m_toolBar );
	m_metronomeToggle->setCheckable(true);
	m_metronomeToggle->setChecked(Engine::mixer()->isMetronomeActive());

	m_toolBarLayout->setColumnMinimumWidth( 0, 5 );
	m_toolBarLayout->addWidget( project_new, 0, 1 );
	m_toolBarLayout->addWidget( project_new_from_template, 0, 2 );
	m_toolBarLayout->addWidget( project_open, 0, 3 );
	m_toolBarLayout->addWidget( project_open_recent, 0, 4 );
	m_toolBarLayout->addWidget( project_save, 0, 5 );
	m_toolBarLayout->addWidget( project_export, 0, 6 );
	m_toolBarLayout->addWidget( whatsthis, 0, 7 );
	m_toolBarLayout->addWidget( m_metronomeToggle, 0, 8 );


	// window-toolbar
	ToolButton * song_editor_window = new ToolButton(
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


	ToolButton * bb_editor_window = new ToolButton(
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


	ToolButton * piano_roll_window = new ToolButton(
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

	ToolButton * automation_editor_window = new ToolButton(
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

	ToolButton * fx_mixer_window = new ToolButton(
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

	ToolButton * project_notes_window = new ToolButton(
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

	ToolButton * controllers_window = new ToolButton(
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
	if( !ConfigManager::inst()->value( "app", "configured" ).toInt() )
	{
		ConfigManager::inst()->setValue( "app", "configured", "1" );
		// no, so show it that user can setup everything
		SetupDialog sd;
		sd.exec();
	}
	// look whether mixer failed to start the audio device selected by the
	// user and is using AudioDummy as a fallback
	else if( Engine::mixer()->audioDevStartFailed() )
	{
		// if so, offer the audio settings section of the setup dialog
		SetupDialog sd( SetupDialog::AudioSettings );
		sd.exec();
	}

	// Add editor subwindows
	for (QWidget* widget :  std::list<QWidget*>{
			gui->automationEditor(),
			gui->getBBEditor(),
			gui->pianoRoll(),
			gui->songEditor()
	})
	{
		QMdiSubWindow* window = addWindowedWidget(widget);
		window->setWindowIcon(widget->windowIcon());
		window->setAttribute(Qt::WA_DeleteOnClose, false);
		window->resize(widget->sizeHint());
	}

	gui->automationEditor()->parentWidget()->hide();
	gui->getBBEditor()->parentWidget()->move( 610, 5 );
	gui->getBBEditor()->parentWidget()->hide();
	gui->pianoRoll()->parentWidget()->move(5, 5);
	gui->pianoRoll()->parentWidget()->hide();
	gui->songEditor()->parentWidget()->move(5, 5);
	gui->songEditor()->parentWidget()->show();

	// reset window title every time we change the state of a subwindow to show the correct title
	for( const QMdiSubWindow * subWindow : workspace()->subWindowList() )
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

SubWindow* MainWindow::addWindowedWidget(QWidget *w, Qt::WindowFlags windowFlags)
{
	// wrap the widget in our own *custom* window that patches some errors in QMdiSubWindow
	SubWindow *win = new SubWindow(m_workspace->viewport(), windowFlags);
	win->setAttribute(Qt::WA_DeleteOnClose);
	win->setWidget(w);
	m_workspace->addSubWindow(win);
	return win;
}


void MainWindow::resetWindowTitle()
{
	QString title = "";
	if( Engine::getSong()->projectFileName() != "" )
	{
		title = QFileInfo( Engine::getSong()->projectFileName()
							).completeBaseName();
	}
	if( title == "" )
	{
		title = tr( "Untitled" );
	}
	if( Engine::getSong()->isModified() )
	{
		title += '*';
	}
	if( getSession() == Recover )
	{
		title += " - " + tr( "Recover session. Please save your work!" );
	}
	if( getSession() == Limited )
	{
		title += " - " + tr( "Automatic backup disabled. Remember to save your work!" );
	}
	setWindowTitle( title + " - " + tr( "LMMS %1" ).arg( LMMS_VERSION ) );
}




bool MainWindow::mayChangeProject(bool stopPlayback)
{
	if( stopPlayback )
	{
		Engine::getSong()->stop();
	}

	if( !Engine::getSong()->isModified() && getSession() != Recover )
	{
		return( true );
	}

	// Separate message strings for modified and recovered files
	QString messageTitleRecovered = tr( "Recovered project not saved" );
	QString messageRecovered = tr( "This project was recovered from the "
					"previous session. It is currently "
					"unsaved and will be lost if you don't "
					"save it. Do you want to save it now?" );

	QString messageTitleUnsaved = tr( "Project not saved" );
	QString messageUnsaved = tr( "The current project was modified since "
					"last saving. Do you want to save it "
								"now?" );

	QMessageBox mb( ( getSession() == Recover ?
				messageTitleRecovered : messageTitleUnsaved ),
			( getSession() == Recover ?
					messageRecovered : messageUnsaved ),
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
		if( getSession() == Recover )
		{
			sessionCleanup();
		}
		return( true );
	}

	return( false );
}




void MainWindow::clearKeyModifiers()
{
	m_keyMods.m_ctrl = false;
	m_keyMods.m_shift = false;
	m_keyMods.m_alt = false;
}




void MainWindow::saveWidgetState( QWidget * _w, QDomElement & _de, QSize const & sizeIfInvisible )
{
	// If our widget is the main content of a window (e.g. piano roll, FxMixer, etc), 
	// we really care about the position of the *window* - not the position of the widget within its window
	if( _w->parentWidget() != NULL &&
			_w->parentWidget()->inherits( "QMdiSubWindow" ) )
	{
		_w = _w->parentWidget();
	}

	// If the widget is a SubWindow, then we can make use of the getTrueNormalGeometry() method that 
	// performs the same as normalGeometry, but isn't broken on X11 ( see https://bugreports.qt.io/browse/QTBUG-256 )
	SubWindow *asSubWindow = qobject_cast<SubWindow*>(_w);
	QRect normalGeom = asSubWindow != nullptr ? asSubWindow->getTrueNormalGeometry() : _w->normalGeometry();

	bool visible = _w->isVisible();
	_de.setAttribute( "visible", visible );
	_de.setAttribute( "minimized", _w->isMinimized() );
	_de.setAttribute( "maximized", _w->isMaximized() );

	_de.setAttribute( "x", normalGeom.x() );
	_de.setAttribute( "y", normalGeom.y() );

	QSize sizeToStore = visible ? normalGeom.size() : sizeIfInvisible;
	_de.setAttribute( "width", sizeToStore.width() );
	_de.setAttribute( "height", sizeToStore.height() );
}




void MainWindow::restoreWidgetState( QWidget * _w, const QDomElement & _de )
{
	QRect r( qMax( 1, _de.attribute( "x" ).toInt() ),
			qMax( 1, _de.attribute( "y" ).toInt() ),
			qMax( 100, _de.attribute( "width" ).toInt() ),
			qMax( 100, _de.attribute( "height" ).toInt() ) );
	if( _de.hasAttribute( "visible" ) && !r.isNull() )
	{
		// If our widget is the main content of a window (e.g. piano roll, FxMixer, etc), 
		// we really care about the position of the *window* - not the position of the widget within its window
		if ( _w->parentWidget() != NULL &&
			_w->parentWidget()->inherits( "QMdiSubWindow" ) )
		{
			_w = _w->parentWidget();
		}
		// first restore the window, as attempting to resize a maximized window causes graphics glitching
		_w->setWindowState( _w->windowState() & ~(Qt::WindowMaximized | Qt::WindowMinimized) );

		_w->resize( r.size() );
		_w->move( r.topLeft() );

		// set the window to its correct minimized/maximized/restored state
		Qt::WindowStates flags = _w->windowState();
		flags = _de.attribute( "minimized" ).toInt() ?
				( flags | Qt::WindowMinimized ) :
				( flags & ~Qt::WindowMinimized );
		flags = _de.attribute( "maximized" ).toInt() ?
				( flags | Qt::WindowMaximized ) :
				( flags & ~Qt::WindowMaximized );
		_w->setWindowState( flags );

		_w->setVisible( _de.attribute( "visible" ).toInt() );
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
	if( mayChangeProject(true) )
	{
		Engine::getSong()->createNewProject();
	}
	runAutoSave();
}




void MainWindow::createNewProjectFromTemplate( QAction * _idx )
{
	if( m_templatesMenu && mayChangeProject(true) )
	{
		int indexOfTemplate = m_templatesMenu->actions().indexOf( _idx );
		bool isFactoryTemplate = indexOfTemplate >= m_custom_templates_count;
		QString dirBase =  isFactoryTemplate ?
				ConfigManager::inst()->factoryTemplatesDir() :
				ConfigManager::inst()->userTemplateDir();

		Engine::getSong()->createNewProjectFromTemplate(
			dirBase + _idx->text() + ".mpt" );
	}
	runAutoSave();
}




void MainWindow::openProject()
{
	if( mayChangeProject(false) )
	{
		FileDialog ofd( this, tr( "Open Project" ), "", tr( "LMMS (*.mmp *.mmpz)" ) );

		ofd.setDirectory( ConfigManager::inst()->userProjectsDir() );
		ofd.setFileMode( FileDialog::ExistingFiles );
		if( ofd.exec () == QDialog::Accepted &&
						!ofd.selectedFiles().isEmpty() )
		{
			Song *song = Engine::getSong();

			song->stop();
			setCursor( Qt::WaitCursor );
			song->loadProject( ofd.selectedFiles()[0] );
			setCursor( Qt::ArrowCursor );
		}
	}
	runAutoSave();
}




void MainWindow::updateRecentlyOpenedProjectsMenu()
{
	m_recentlyOpenedProjectsMenu->clear();
	QStringList rup = ConfigManager::inst()->recentlyOpenedProjects();

//	The file history goes 30 deep but we only show the 15
//	most recent ones that we can open.
	int shownInMenu = 0;
	for( QStringList::iterator it = rup.begin(); it != rup.end(); ++it )
	{
		QFileInfo recentFile( *it );
		if ( recentFile.exists() && 
				*it != ConfigManager::inst()->recoveryFile() )
		{
			m_recentlyOpenedProjectsMenu->addAction(
					embed::getIconPixmap( "project_file" ), *it );
			shownInMenu++;
			if( shownInMenu >= 15 )
			{
				return;
			}
		}
	}
}




void MainWindow::openRecentlyOpenedProject( QAction * _action )
{
	if ( mayChangeProject(true) )
	{
		const QString & f = _action->text();
		setCursor( Qt::WaitCursor );
		Engine::getSong()->loadProject( f );
		ConfigManager::inst()->addRecentlyOpenedProject( f );
		setCursor( Qt::ArrowCursor );
	}
	runAutoSave();
}




bool MainWindow::saveProject()
{
	if( Engine::getSong()->projectFileName() == "" )
	{
		return( saveProjectAs() );
	}
	else
	{
		Engine::getSong()->guiSaveProject();
		if( getSession() == Recover )
		{
			sessionCleanup();
		}
	}
	return( true );
}




bool MainWindow::saveProjectAs()
{
	VersionedSaveDialog sfd( this, tr( "Save Project" ), "",
			tr( "LMMS Project" ) + " (*.mmpz *.mmp);;" +
				tr( "LMMS Project Template" ) + " (*.mpt)" );
	QString f = Engine::getSong()->projectFileName();
	if( f != "" )
	{
		sfd.setDirectory( QFileInfo( f ).absolutePath() );
		sfd.selectFile( QFileInfo( f ).fileName() );
	}
	else
	{
		sfd.setDirectory( ConfigManager::inst()->userProjectsDir() );
	}

	// Don't write over file with suffix if no suffix is provided.
	QString suffix = ConfigManager::inst()->value( "app",
							"nommpz" ).toInt() == 0
						? "mmpz"
						: "mmp" ;
	sfd.setDefaultSuffix( suffix );

	if( sfd.exec () == FileDialog::Accepted &&
		!sfd.selectedFiles().isEmpty() && sfd.selectedFiles()[0] != "" )
	{
		QString fname = sfd.selectedFiles()[0] ;
		if( sfd.selectedNameFilter().contains( "(*.mpt)" ) )
		{
			// Remove the default suffix
			fname.remove( "." + suffix );
			if( !sfd.selectedFiles()[0].endsWith( ".mpt" ) )
			{
				if( VersionedSaveDialog::fileExistsQuery( fname + ".mpt",
						tr( "Save project template" ) ) )
				{
					fname += ".mpt";
				}
			}
		}
		Engine::getSong()->guiSaveProjectAs( fname );
		if( getSession() == Recover )
		{
			sessionCleanup();
		}
		return( true );
	}
	return( false );
}




bool MainWindow::saveProjectAsNewVersion()
{
	QString fileName = Engine::getSong()->projectFileName();
	if( fileName == "" )
	{
		return saveProjectAs();
	}
	else
	{
		do 		VersionedSaveDialog::changeFileNameVersion( fileName, true );
		while 	( QFile( fileName ).exists() );

		Engine::getSong()->guiSaveProjectAs( fileName );
		return true;
	}
}




void MainWindow::saveProjectAsDefaultTemplate()
{
	QString defaultTemplate = ConfigManager::inst()->userTemplateDir() + "default.mpt";

	QFileInfo fileInfo(defaultTemplate);
	if (fileInfo.exists())
	{
		if (QMessageBox::warning(this,
					 tr("Overwrite default template?"),
					 tr("This will overwrite your current default template."),
					 QMessageBox::Ok,
					 QMessageBox::Cancel) != QMessageBox::Ok)
		{
			return;
		}
	}

	Engine::getSong()->saveProjectFile( defaultTemplate );
}




void MainWindow::showSettingsDialog()
{
	SetupDialog sd;
	sd.exec();
}




void MainWindow::aboutLMMS()
{
	AboutDialog(this).exec();
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
		refocus();
	}

	// Workaround for Qt Bug #260116
	m_workspace->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_workspace->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_workspace->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_workspace->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
}




/*
 * When an editor window with focus is toggled off, attempt to set focus
 * to the next visible editor window, or if none are visible, set focus
 * to the parent window.
 */
void MainWindow::refocus()
{
	QList<QWidget*> editors;
	editors
		<< gui->songEditor()->parentWidget()
		<< gui->getBBEditor()->parentWidget()
		<< gui->pianoRoll()->parentWidget()
		<< gui->automationEditor()->parentWidget();

	bool found = false;
	QList<QWidget*>::Iterator editor;
	for( editor = editors.begin(); editor != editors.end(); ++editor )
	{
		if( ! (*editor)->isHidden() ) {
			(*editor)->setFocus();
			found = true;
			break;
		}
	}

	if( ! found )
		this->setFocus();
}




void MainWindow::toggleBBEditorWin( bool forceShow )
{
	toggleWindow( gui->getBBEditor(), forceShow );
}




void MainWindow::toggleSongEditorWin()
{
	toggleWindow( gui->songEditor() );
}




void MainWindow::toggleProjectNotesWin()
{
	toggleWindow( gui->getProjectNotes() );
}




void MainWindow::togglePianoRollWin()
{
	toggleWindow( gui->pianoRoll() );
}




void MainWindow::toggleAutomationEditorWin()
{
	toggleWindow( gui->automationEditor() );
}




void MainWindow::toggleFxMixerWin()
{
	toggleWindow( gui->fxMixerView() );
}


void MainWindow::updateViewMenu()
{
	m_viewMenu->clear();
	// TODO: get current visibility for these and indicate in menu?
	// Not that it's straight visible <-> invisible, more like
	// not on top -> top <-> invisible
	m_viewMenu->addAction(embed::getIconPixmap( "songeditor" ),
			      tr( "Song Editor" ) + " (F5)",
			      this, SLOT( toggleSongEditorWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "bb_track" ),
					tr( "Beat+Bassline Editor" ) + " (F6)",
					this, SLOT( toggleBBEditorWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "piano" ),
			      tr( "Piano Roll" ) + " (F7)",
			      this, SLOT( togglePianoRollWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "automation" ),
			      tr( "Automation Editor" ) + " (F8)",
			      this,
			      SLOT( toggleAutomationEditorWin())
		);
	m_viewMenu->addAction(embed::getIconPixmap( "fx_mixer" ),
			      tr( "FX Mixer" ) + " (F9)",
			      this, SLOT( toggleFxMixerWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "project_notes" ),
			      tr( "Project Notes" ) +	" (F10)",
			      this, SLOT( toggleProjectNotesWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "controller" ),
			      tr( "Controller Rack" ) +
			      " (F11)",
			      this, SLOT( toggleControllerRack() )
		);

	m_viewMenu->addSeparator();

	// Here we should put all look&feel -stuff from configmanager
	// that is safe to change on the fly. There is probably some
	// more elegant way to do this.
	QAction *qa;
	qa = new QAction(tr( "Volume as dBFS" ), this);
	qa->setData("displaydbfs");
	qa->setCheckable( true );
	qa->setChecked( ConfigManager::inst()->value( "app", "displaydbfs" ).toInt() );
	m_viewMenu->addAction(qa);

	// Maybe this is impossible?
	/* qa = new QAction(tr( "Tooltips" ), this);
	qa->setData("tooltips");
	qa->setCheckable( true );
	qa->setChecked( !ConfigManager::inst()->value( "tooltips", "disabled" ).toInt() );
	m_viewMenu->addAction(qa);
	*/

	qa = new QAction(tr( "Smooth scroll" ), this);
	qa->setData("smoothscroll");
	qa->setCheckable( true );
	qa->setChecked( ConfigManager::inst()->value( "ui", "smoothscroll" ).toInt() );
	m_viewMenu->addAction(qa);

	// Not yet.
	/* qa = new QAction(tr( "One instrument track window" ), this);
	qa->setData("oneinstrument");
	qa->setCheckable( true );
	qa->setChecked( ConfigManager::inst()->value( "ui", "oneinstrumenttrackwindow" ).toInt() );
	m_viewMenu->addAction(qa);
	*/

	qa = new QAction(tr( "Enable note labels in piano roll" ), this);
	qa->setData("printnotelabels");
	qa->setCheckable( true );
	qa->setChecked( ConfigManager::inst()->value( "ui", "printnotelabels" ).toInt() );
	m_viewMenu->addAction(qa);

}




void MainWindow::updateConfig( QAction * _who )
{
	QString tag = _who->data().toString();
	bool checked = _who->isChecked();

	if( tag == "displaydbfs" )
	{
		ConfigManager::inst()->setValue( "app", "displaydbfs",
						 QString::number(checked) );
	}
	else if ( tag == "tooltips" )
	{
		ConfigManager::inst()->setValue( "tooltips", "disabled",
						 QString::number(!checked) );
	}
	else if ( tag == "smoothscroll" )
	{
		ConfigManager::inst()->setValue( "ui", "smoothscroll",
						 QString::number(checked) );
	}
	else if ( tag == "oneinstrument" )
	{
		ConfigManager::inst()->setValue( "ui", "oneinstrumenttrackwindow",
						 QString::number(checked) );
	}
	else if ( tag == "printnotelabels" )
	{
		ConfigManager::inst()->setValue( "ui", "printnotelabels",
						 QString::number(checked) );
	}
}



void MainWindow::onToggleMetronome()
{
	Mixer * mixer = Engine::mixer();

	mixer->setMetronomeActive( m_metronomeToggle->isChecked() );
}




void MainWindow::toggleControllerRack()
{
	toggleWindow( gui->getControllerRackView() );
}




void MainWindow::updatePlayPauseIcons()
{
	gui->songEditor()->setPauseIcon( false );
	gui->automationEditor()->setPauseIcon( false );
	gui->getBBEditor()->setPauseIcon( false );
	gui->pianoRoll()->setPauseIcon( false );

	if( Engine::getSong()->isPlaying() )
	{
		switch( Engine::getSong()->playMode() )
		{
			case Song::Mode_PlaySong:
				gui->songEditor()->setPauseIcon( true );
				break;

			case Song::Mode_PlayAutomationPattern:
				gui->automationEditor()->setPauseIcon( true );
				break;

			case Song::Mode_PlayBB:
				gui->getBBEditor()->setPauseIcon( true );
				break;

			case Song::Mode_PlayPattern:
				gui->pianoRoll()->setPauseIcon( true );
				break;

			default:
				break;
		}
	}
}


void MainWindow::updateUndoRedoButtons()
{
	// when the edit menu is shown, grey out the undo/redo buttons if there's nothing to undo/redo
	// else, un-grey them
	m_undoAction->setEnabled(Engine::projectJournal()->canUndo());
	m_redoAction->setEnabled(Engine::projectJournal()->canRedo());
}



void MainWindow::undo()
{
	Engine::projectJournal()->undo();
}




void MainWindow::redo()
{
	Engine::projectJournal()->redo();
}




void MainWindow::closeEvent( QCloseEvent * _ce )
{
	if( mayChangeProject(true) )
	{
		// delete recovery file
		if( ConfigManager::inst()->value( "ui", "enableautosave" ).toInt()
			&& getSession() != Limited )
		{
			sessionCleanup();
			_ce->accept();
		}
	}
	else
	{
		_ce->ignore();
	}
}




void MainWindow::sessionCleanup()
{
	// delete recover session files
	QFile::remove( ConfigManager::inst()->recoveryFile() );
	setSession( Normal );
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
		case Qt::Key_Control: m_keyMods.m_ctrl = true; break;
		case Qt::Key_Shift: m_keyMods.m_shift = true; break;
		case Qt::Key_Alt: m_keyMods.m_alt = true; break;
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
		case Qt::Key_Control: m_keyMods.m_ctrl = false; break;
		case Qt::Key_Shift: m_keyMods.m_shift = false; break;
		case Qt::Key_Alt: m_keyMods.m_alt = false; break;
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

	QDir user_d( ConfigManager::inst()->userTemplateDir() );
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

	QDir d( ConfigManager::inst()->factoryProjectsDir() + "templates" );
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
	QString url = "https://lmms.io/documentation/";
	QDesktopServices::openUrl( url );
	// TODO: Handle error
}




void MainWindow::autoSave()
{
	if( !Engine::getSong()->isExporting() &&
		!QApplication::mouseButtons() &&
			( ConfigManager::inst()->value( "ui",
					"enablerunningautosave" ).toInt() ||
				! Engine::getSong()->isPlaying() ) )
	{
		Engine::getSong()->saveProjectFile(ConfigManager::inst()->recoveryFile());
		autoSaveTimerReset();  // Reset timer
	}
	else
	{
		// try again in 10 seconds
		if( getAutoSaveTimerInterval() != m_autoSaveShortTime )
		{
			autoSaveTimerReset( m_autoSaveShortTime );
		}
	}
}


// For the occasional auto save action that isn't run
// from the timer where we need to do extra tests.
void MainWindow::runAutoSave()
{
	if( ConfigManager::inst()->value( "ui", "enableautosave" ).toInt() &&
		getSession() != Limited )
	{
		autoSave();
		autoSaveTimerReset();  // Reset timer
	}
}
