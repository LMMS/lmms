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
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QLibrary>
#include <QSplitter>
#include <QUrl>

#include "AboutDialog.h"
#include "AudioDummy.h"
#include "AutomationEditor.h"
#include "BBEditor.h"
#include "ControllerRackView.h"
#include "embed.h"
#include "Engine.h"
#include "ExportProjectDialog.h"
#include "FileBrowser.h"
#include "FileDialog.h"
#include "FxMixerView.h"
#include "GuiApplication.h"
#include "ImportFilter.h"
#include "InstrumentTrack.h"
#include "PianoRoll.h"
#include "PluginBrowser.h"
#include "PluginFactory.h"
#include "PluginView.h"
#include "ProjectJournal.h"
#include "ProjectNotes.h"
#include "ProjectRenderer.h"
#include "RecentProjectsMenu.h"
#include "RemotePlugin.h"
#include "SetupDialog.h"
#include "SideBar.h"
#include "SongEditor.h"
#include "TemplatesMenu.h"
#include "TextFloat.h"
#include "TimeLineWidget.h"
#include "ToolButton.h"
#include "ToolPlugin.h"
#include "VersionedSaveDialog.h"

#include "lmmsversion.h"


#if !defined(LMMS_BUILD_WIN32) && !defined(LMMS_BUILD_APPLE) && !defined(LMMS_BUILD_HAIKU)
//Work around an issue on KDE5 as per https://bugs.kde.org/show_bug.cgi?id=337491#c21
void disableAutoKeyAccelerators(QWidget* mainWindow)
{
	using DisablerFunc = void(*)(QWidget*);
	QLibrary kf5WidgetsAddon("KF5WidgetsAddons", 5);
	DisablerFunc setNoAccelerators =
			reinterpret_cast<DisablerFunc>(kf5WidgetsAddon.resolve("_ZN19KAcceleratorManager10setNoAccelEP7QWidget"));
	if(setNoAccelerators)
	{
		setNoAccelerators(mainWindow);
	}
	kf5WidgetsAddon.unload();
}
#endif


MainWindow::MainWindow() :
	m_workspace( NULL ),
	m_toolsMenu( NULL ),
	m_autoSaveTimer( this ),
	m_viewMenu( NULL ),
	m_metronomeToggle( 0 ),
	m_session( Normal )
{
#if !defined(LMMS_BUILD_WIN32) && !defined(LMMS_BUILD_APPLE) && !defined(LMMS_BUILD_HAIKU)
	disableAutoKeyAccelerators(this);
#endif
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
	bool sideBarOnRight = confMgr->value("ui", "sidebaronright").toInt();

	emit initProgress(tr("Preparing plugin browser"));
	sideBar->appendTab( new PluginBrowser( splitter ) );
	emit initProgress(tr("Preparing file browsers"));
	sideBar->appendTab( new FileBrowser(
				confMgr->userProjectsDir() + "*" +
				confMgr->factoryProjectsDir(),
					"*.mmp *.mmpz *.xml *.mid",
							tr( "My Projects" ),
					embed::getIconPixmap( "project_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter, false, true,
				confMgr->userProjectsDir(),
				confMgr->factoryProjectsDir()));
	sideBar->appendTab( new FileBrowser(
				confMgr->userSamplesDir() + "*" +
				confMgr->factorySamplesDir(),
					"*", tr( "My Samples" ),
					embed::getIconPixmap( "sample_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter, false, true,
					confMgr->userSamplesDir(),
					confMgr->factorySamplesDir()));
	sideBar->appendTab( new FileBrowser(
				confMgr->userPresetsDir() + "*" +
				confMgr->factoryPresetsDir(),
					"*.xpf *.cs.xml *.xiz *.lv2",
					tr( "My Presets" ),
					embed::getIconPixmap( "preset_file" ).transformed( QTransform().rotate( 90 ) ),
							splitter , false, true,
				confMgr->userPresetsDir(),
				confMgr->factoryPresetsDir()));
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

	m_workspace = new QMdiArea(splitter);

	// Load background
	emit initProgress(tr("Loading background picture"));
	QString backgroundPicFile = ConfigManager::inst()->backgroundPicFile();
	QImage backgroundPic;
	if( !backgroundPicFile.isEmpty() )
	{
		backgroundPic = QImage( backgroundPicFile );
	}
	if( !backgroundPicFile.isNull() )
	{
		m_workspace->setBackground( backgroundPic );
	}
	else
	{
		m_workspace->setBackground( Qt::NoBrush );
	}

	m_workspace->setOption( QMdiArea::DontMaximizeSubWindowOnActivation );
	m_workspace->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_workspace->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );

	hbox->addWidget(sideBar);
	hbox->addWidget(splitter);
	// If the user wants the sidebar on the right, we move the workspace and
	// the splitter to the "left" side, or the first widgets in their list
	if (sideBarOnRight)
	{
		splitter->insertWidget(0, m_workspace);
		hbox->insertWidget(0, splitter);
	}

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

	m_updateTimer.start( 1000 / 60, this );  // 60 fps

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

	connect(Engine::getSong(), SIGNAL(stopped()), SLOT(onSongStopped()));

	connect(Engine::getSong(), SIGNAL(modified()), SLOT(onSongModified()));
	connect(Engine::getSong(), SIGNAL(projectFileNameChanged()), SLOT(onProjectFileNameChanged()));

	maximized = isMaximized();
	new QShortcut(QKeySequence(Qt::Key_F11), this, SLOT(toggleFullscreen()));
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
	setWindowIcon( embed::getIconPixmap( "icon_small" ) );


	// project-popup-menu
	QMenu * project_menu = new QMenu( this );
	menuBar()->addMenu( project_menu )->setText( tr( "&File" ) );
	project_menu->addAction( embed::getIconPixmap( "project_new" ),
					tr( "&New" ),
					this, SLOT( createNewProject() ),
					QKeySequence::New );

	auto templates_menu = new TemplatesMenu( this );
	project_menu->addMenu(templates_menu);

	project_menu->addAction( embed::getIconPixmap( "project_open" ),
					tr( "&Open..." ),
					this, SLOT( openProject() ),
					QKeySequence::Open );

	project_menu->addMenu(new RecentProjectsMenu(this));

	project_menu->addAction( embed::getIconPixmap( "project_save" ),
					tr( "&Save" ),
					this, SLOT( saveProject() ),
					QKeySequence::Save );
	project_menu->addAction( embed::getIconPixmap( "project_save" ),
					tr( "Save &As..." ),
					this, SLOT( saveProjectAs() ),
					Qt::CTRL + Qt::SHIFT + Qt::Key_S );
	project_menu->addAction( embed::getIconPixmap( "project_save" ),
					tr( "Save as New &Version" ),
					this, SLOT( saveProjectAsNewVersion() ),
					Qt::CTRL + Qt::ALT + Qt::Key_S );

	project_menu->addAction( embed::getIconPixmap( "project_save" ),
					tr( "Save as default template" ),
					this, SLOT( saveProjectAsDefaultTemplate() ) );

	project_menu->addSeparator();
	project_menu->addAction( embed::getIconPixmap( "project_import" ),
					tr( "Import..." ),
					this,
					SLOT( onImportProject() ) );
	project_menu->addAction( embed::getIconPixmap( "project_export" ),
					tr( "E&xport..." ),
					this,
					SLOT( onExportProject() ),
					Qt::CTRL + Qt::Key_E );
	project_menu->addAction( embed::getIconPixmap( "project_export" ),
					tr( "E&xport Tracks..." ),
					this,
					SLOT( onExportProjectTracks() ),
					Qt::CTRL + Qt::SHIFT + Qt::Key_E );

	project_menu->addAction( embed::getIconPixmap( "midi_file" ),
					tr( "Export &MIDI..." ),
					this,
					SLOT( onExportProjectMidi() ),
					Qt::CTRL + Qt::Key_M );

// Prevent dangling separator at end of menu per https://bugreports.qt.io/browse/QTBUG-40071
#if !(defined(LMMS_BUILD_APPLE) && (QT_VERSION < 0x050600))
	project_menu->addSeparator();
#endif
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

// Prevent dangling separator at end of menu per https://bugreports.qt.io/browse/QTBUG-40071
#if !(defined(LMMS_BUILD_APPLE) && (QT_VERSION < 0x050600))
	help_menu->addSeparator();
#endif
	help_menu->addAction( embed::getIconPixmap( "icon_small" ), tr( "About" ),
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
	project_new_from_template->setMenu( templates_menu );
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
	project_open_recent->setMenu( new RecentProjectsMenu(this) );
	project_open_recent->setPopupMode( ToolButton::InstantPopup );

	ToolButton * project_save = new ToolButton(
					embed::getIconPixmap( "project_save" ),
					tr( "Save current project" ),
					this, SLOT( saveProject() ),
								m_toolBar );


	ToolButton * project_export = new ToolButton(
				embed::getIconPixmap( "project_export" ),
					tr( "Export current project" ),
					this,
							SLOT( onExportProject() ),
								m_toolBar );

	m_metronomeToggle = new ToolButton(
				embed::getIconPixmap( "metronome" ),
				tr( "Metronome" ),
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
	m_toolBarLayout->addWidget( m_metronomeToggle, 0, 7 );


	// window-toolbar
	ToolButton * song_editor_window = new ToolButton(
					embed::getIconPixmap( "songeditor" ),
					tr( "Song Editor" ) + " (Ctrl+1)",
					this, SLOT( toggleSongEditorWin() ),
								m_toolBar );
	song_editor_window->setShortcut( Qt::CTRL + Qt::Key_1 );


	ToolButton * bb_editor_window = new ToolButton(
					embed::getIconPixmap( "bb_track_btn" ),
					tr( "Beat+Bassline Editor" ) +
									" (Ctrl+2)",
					this, SLOT( toggleBBEditorWin() ),
								m_toolBar );
	bb_editor_window->setShortcut( Qt::CTRL + Qt::Key_2 );


	ToolButton * piano_roll_window = new ToolButton(
						embed::getIconPixmap( "piano" ),
						tr( "Piano Roll" ) +
									" (Ctrl+3)",
					this, SLOT( togglePianoRollWin() ),
								m_toolBar );
	piano_roll_window->setShortcut( Qt::CTRL + Qt::Key_3 );

	ToolButton * automation_editor_window = new ToolButton(
					embed::getIconPixmap( "automation" ),
					tr( "Automation Editor" ) +
									" (Ctrl+4)",
					this,
					SLOT( toggleAutomationEditorWin() ),
					m_toolBar );
	automation_editor_window->setShortcut( Qt::CTRL + Qt::Key_4 );

	ToolButton * fx_mixer_window = new ToolButton(
					embed::getIconPixmap( "fx_mixer" ),
					tr( "FX Mixer" ) + " (Ctrl+5)",
					this, SLOT( toggleFxMixerWin() ),
					m_toolBar );
	fx_mixer_window->setShortcut( Qt::CTRL + Qt::Key_5 );

	ToolButton * controllers_window = new ToolButton(
					embed::getIconPixmap( "controller" ),
					tr( "Show/hide controller rack" ) +
								" (Ctrl+6)",
					this, SLOT( toggleControllerRack() ),
								m_toolBar );
	controllers_window->setShortcut( Qt::CTRL + Qt::Key_6 );

	ToolButton * project_notes_window = new ToolButton(
					embed::getIconPixmap( "project_notes" ),
					tr( "Show/hide project notes" ) +
								" (Ctrl+7)",
					this, SLOT( toggleProjectNotesWin() ),
								m_toolBar );
	project_notes_window->setShortcut( Qt::CTRL + Qt::Key_7 );

	m_toolBarLayout->addWidget( song_editor_window, 1, 1 );
	m_toolBarLayout->addWidget( bb_editor_window, 1, 2 );
	m_toolBarLayout->addWidget( piano_roll_window, 1, 3 );
	m_toolBarLayout->addWidget( automation_editor_window, 1, 4 );
	m_toolBarLayout->addWidget( fx_mixer_window, 1, 5 );
	m_toolBarLayout->addWidget( controllers_window, 1, 6 );
	m_toolBarLayout->addWidget( project_notes_window, 1, 7 );
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
	// or the audio device is set to invalid one
	else if( Engine::mixer()->audioDevStartFailed() || !Mixer::isAudioDevNameValid(
		ConfigManager::inst()->value( "mixer", "audiodev" ) ) )
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
	if (w && w->sizeHint().isValid()) {win->resize(w->sizeHint());}
	m_workspace->addSubWindow(win);
	return win;
}


void MainWindow::resetWindowTitle()
{
	QString title(tr( "Untitled" ));

	if( Engine::getSong()->projectFileName() != "" )
	{
		title = QFileInfo( Engine::getSong()->projectFileName()
							).completeBaseName();
	}

	if( Engine::getSong()->isModified() )
	{
		title += '*';
	}

	if( getSession() == Recover )
	{
		title += " - " + tr( "Recover session. Please save your work!" );
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




void MainWindow::saveWidgetState( QWidget * _w, QDomElement & _de )
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

	QSize sizeToStore = normalGeom.size();
	_de.setAttribute( "width", sizeToStore.width() );
	_de.setAttribute( "height", sizeToStore.height() );
}




void MainWindow::restoreWidgetState( QWidget * _w, const QDomElement & _de )
{
	QRect r( qMax( 1, _de.attribute( "x" ).toInt() ),
			qMax( 1, _de.attribute( "y" ).toInt() ),
			qMax( _w->sizeHint().width(), _de.attribute( "width" ).toInt() ),
			qMax( _w->minimumHeight(), _de.attribute( "height" ).toInt() ) );
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

		// Check isEmpty() to work around corrupt project files with empty size
		if ( ! r.size().isEmpty() ) {
			_w->resize( r.size() );
		}
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



void MainWindow::createNewProject()
{
	if( mayChangeProject(true) )
	{
		Engine::getSong()->createNewProject();
	}
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
}




bool MainWindow::saveProject()
{
	if( Engine::getSong()->projectFileName() == "" )
	{
		return( saveProjectAs() );
	}
	else if( this->guiSaveProject() )
	{
		if( getSession() == Recover )
		{
			sessionCleanup();
		}
		return true;
	}
	return false;
}




bool MainWindow::saveProjectAs()
{
	auto optionsWidget = new SaveOptionsWidget(Engine::getSong()->getSaveOptions());
	VersionedSaveDialog sfd( this, optionsWidget, tr( "Save Project" ), "",
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
		if( this->guiSaveProjectAs( fname ) )
		{
			if( getSession() == Recover )
			{
				sessionCleanup();
			}
			return true;
		}
	}
	return false;
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

		return this->guiSaveProjectAs( fileName );
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



void MainWindow::toggleFullscreen()
{
	if ( !isFullScreen() )
	{
		maximized = isMaximized();
		showFullScreen();
	}
	else
	{
		maximized ? showMaximized() : showNormal();
	}
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
			      tr( "Song Editor" ) + "\tCtrl+1",
			      this, SLOT( toggleSongEditorWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "bb_track" ),
					tr( "Beat+Bassline Editor" ) + "\tCtrl+2",
					this, SLOT( toggleBBEditorWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "piano" ),
			      tr( "Piano Roll" ) + "\tCtrl+3",
			      this, SLOT( togglePianoRollWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "automation" ),
			      tr( "Automation Editor" ) + "\tCtrl+4",
			      this,
			      SLOT( toggleAutomationEditorWin())
		);
	m_viewMenu->addAction(embed::getIconPixmap( "fx_mixer" ),
			      tr( "FX Mixer" ) + "\tCtrl+5",
			      this, SLOT( toggleFxMixerWin() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "controller" ),
			      tr( "Controller Rack" ) + "\tCtrl+6",
			      this, SLOT( toggleControllerRack() )
		);
	m_viewMenu->addAction(embed::getIconPixmap( "project_notes" ),
			      tr( "Project Notes" ) + "\tCtrl+7",
			      this, SLOT( toggleProjectNotesWin() )
		);

	m_viewMenu->addSeparator();
	
	m_viewMenu->addAction(embed::getIconPixmap( "fullscreen" ),
				tr( "Fullscreen" ) + "\tF11",
				this, SLOT( toggleFullscreen() ) 
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
		if( ConfigManager::inst()->
				value( "ui", "enableautosave" ).toInt() )
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
	// TODO Remove this function, since it is apparently never actually called!
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
		!Engine::getSong()->isLoadingProject() &&
		!RemotePluginBase::isMainThreadWaiting() &&
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

void MainWindow::onExportProjectMidi()
{
	FileDialog efd( this );

	efd.setFileMode( FileDialog::AnyFile );

	QStringList types;
	types << tr("MIDI File (*.mid)");
	efd.setNameFilters( types );
	QString base_filename;
	QString const & projectFileName = Engine::getSong()->projectFileName();
	if( !projectFileName.isEmpty() )
	{
		efd.setDirectory( QFileInfo( projectFileName ).absolutePath() );
		base_filename = QFileInfo( projectFileName ).completeBaseName();
	}
	else
	{
		efd.setDirectory( ConfigManager::inst()->userProjectsDir() );
		base_filename = tr( "untitled" );
	}
	efd.selectFile( base_filename + ".mid" );
	efd.setDefaultSuffix( "mid");
	efd.setWindowTitle( tr( "Select file for project-export..." ) );

	efd.setAcceptMode( FileDialog::AcceptSave );


	if( efd.exec() == QDialog::Accepted && !efd.selectedFiles().isEmpty() && !efd.selectedFiles()[0].isEmpty() )
	{
		const QString suffix = ".mid";

		QString export_filename = efd.selectedFiles()[0];
		if (!export_filename.endsWith(suffix)) export_filename += suffix;

		Engine::getSong()->exportProjectMidi(export_filename);
	}
}

void MainWindow::exportProject(bool multiExport)
{
	QString const & projectFileName = Engine::getSong()->projectFileName();

	FileDialog efd( gui->mainWindow() );

	if ( multiExport )
	{
		efd.setFileMode( FileDialog::Directory);
		efd.setWindowTitle( tr( "Select directory for writing exported tracks..." ) );
		if( !projectFileName.isEmpty() )
		{
			efd.setDirectory( QFileInfo( projectFileName ).absolutePath() );
		}
	}
	else
	{
		efd.setFileMode( FileDialog::AnyFile );
		int idx = 0;
		QStringList types;
		while( ProjectRenderer::fileEncodeDevices[idx].m_fileFormat != ProjectRenderer::NumFileFormats)
		{
			if(ProjectRenderer::fileEncodeDevices[idx].isAvailable()) {
				types << tr(ProjectRenderer::fileEncodeDevices[idx].m_description);
			}
			++idx;
		}
		efd.setNameFilters( types );
		QString baseFilename;
		if( !projectFileName.isEmpty() )
		{
			efd.setDirectory( QFileInfo( projectFileName ).absolutePath() );
			baseFilename = QFileInfo( projectFileName ).completeBaseName();
		}
		else
		{
			efd.setDirectory( ConfigManager::inst()->userProjectsDir() );
			baseFilename = tr( "untitled" );
		}
		efd.selectFile( baseFilename + ProjectRenderer::fileEncodeDevices[0].m_extension );
		efd.setWindowTitle( tr( "Select file for project-export..." ) );
	}

	QString suffix = "wav";
	efd.setDefaultSuffix( suffix );
	efd.setAcceptMode( FileDialog::AcceptSave );

	if( efd.exec() == QDialog::Accepted && !efd.selectedFiles().isEmpty() &&
					 !efd.selectedFiles()[0].isEmpty() )
	{

		QString exportFileName = efd.selectedFiles()[0];
		if ( !multiExport )
		{
			int stx = efd.selectedNameFilter().indexOf( "(*." );
			int etx = efd.selectedNameFilter().indexOf( ")" );

			if ( stx > 0 && etx > stx )
			{
				// Get first extension from selected dropdown.
				// i.e. ".wav" from "WAV-File (*.wav), Dummy-File (*.dum)"
				suffix = efd.selectedNameFilter().mid( stx + 2, etx - stx - 2 ).split( " " )[0].trimmed();

				Qt::CaseSensitivity cs = Qt::CaseSensitive;
#if defined(LMMS_BUILD_APPLE) || defined(LMMS_BUILD_WIN32)
				cs = Qt::CaseInsensitive;
#endif
				exportFileName.remove( "." + suffix, cs );
				if ( efd.selectedFiles()[0].endsWith( suffix ) )
				{
					if( VersionedSaveDialog::fileExistsQuery( exportFileName + suffix,
							tr( "Save project" ) ) )
					{
						exportFileName += suffix;
					}
				}
			}
		}

		ExportProjectDialog epd( exportFileName, gui->mainWindow(), multiExport );
		epd.exec();
	}
}

void MainWindow::handleSaveResult(QString const & filename, bool songSavedSuccessfully)
{
	if (songSavedSuccessfully)
	{
		TextFloat::displayMessage( tr( "Project saved" ), tr( "The project %1 is now saved.").arg( filename ),
				embed::getIconPixmap( "project_save", 24, 24 ), 2000 );
		ConfigManager::inst()->addRecentlyOpenedProject(filename);
		resetWindowTitle();
	}
	else
	{
		TextFloat::displayMessage( tr( "Project NOT saved." ), tr( "The project %1 was not saved!" ).arg(filename),
				embed::getIconPixmap( "error" ), 4000 );
	}
}

bool MainWindow::guiSaveProject()
{
	Song * song = Engine::getSong();
	bool const songSaveResult = song->guiSaveProject();
	handleSaveResult(song->projectFileName(), songSaveResult);

	return songSaveResult;
}

bool MainWindow::guiSaveProjectAs( const QString & filename )
{
	Song * song = Engine::getSong();
	bool const songSaveResult = song->guiSaveProjectAs(filename);
	handleSaveResult(filename, songSaveResult);

	return songSaveResult;
}

void MainWindow::onExportProject()
{
	this->exportProject();
}

void MainWindow::onExportProjectTracks()
{
	this->exportProject(true);
}

void MainWindow::onImportProject()
{
	Song * song = Engine::getSong();

	if (song)
	{
		FileDialog ofd( nullptr, tr( "Import file" ),
				ConfigManager::inst()->userProjectsDir(),
				tr("MIDI sequences") +
				" (*.mid *.midi *.rmi);;" +
				tr("Hydrogen projects") +
				" (*.h2song);;" +
				tr("All file types") +
				" (*.*)");

		ofd.setFileMode( FileDialog::ExistingFiles );
		if( ofd.exec () == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
		{
			ImportFilter::import( ofd.selectedFiles()[0], song );
		}

		song->setLoadOnLauch(false);
	}
}

void MainWindow::onSongStopped()
{
	Song * song = Engine::getSong();
	Song::PlayPos const & playPos = song->getPlayPos();

	TimeLineWidget * tl = playPos.m_timeLine;

	if( tl )
	{
		SongEditorWindow* songEditor = gui->songEditor();
		switch( tl->behaviourAtStop() )
		{
			case TimeLineWidget::BackToZero:
				if( songEditor && ( tl->autoScroll() == TimeLineWidget::AutoScrollEnabled ) )
				{
					songEditor->m_editor->updatePosition(0);
				}
				break;

			case TimeLineWidget::BackToStart:
				if( tl->savedPos() >= 0 )
				{
					if(songEditor && ( tl->autoScroll() == TimeLineWidget::AutoScrollEnabled ) )
					{
						songEditor->m_editor->updatePosition( TimePos(tl->savedPos().getTicks() ) );
					}
					tl->savePos( -1 );
				}
				break;

			case TimeLineWidget::KeepStopPosition:
				break;
		}
	}
}

void MainWindow::onSongModified()
{
	// Only update the window title if the code is executed from the GUI main thread.
	// The assumption seems to be that the Song can also be set as modified from other
	// threads. This is not a good design! Copied from the original implementation of
	// Song::setModified.
	if(QThread::currentThread() == this->thread())
	{
		this->resetWindowTitle();
	}
}

void MainWindow::onProjectFileNameChanged()
{
	this->resetWindowTitle();
}
