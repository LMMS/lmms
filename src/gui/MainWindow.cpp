/*
 * MainWindow.cpp - implementation of LMMS' main window
 *
 * Copyright (c) 2004-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QLabel>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QSplitter>
#include <QtGui/QCheckBox>
#include <QtGui/QRadioButton>
#include <QtGui/QShortcut>
#include <QtGui/QGraphicsView>
#include <QtGui/QPixmapCache>

#include "lmmsversion.h"
#include "MainWindow.h"
#include "bb_editor.h"
#include "song_editor.h"
#include "AutomationRecorder.h"
#include "song.h"
#include "piano_roll.h"
#include "embed.h"
#include "engine.h"
#include "FxMixerView.h"
#include "InstrumentTrack.h"
#include "PianoView.h"
#include "AboutDialog.h"
#include "PreferencesDialog.h"
#include "ControllerRackView.h"
#include "plugin_browser.h"
#include "SideBar.h"
#include "config_mgr.h"
#include "Mixer.h"
#include "project_notes.h"
#include "setup_dialog.h"
#include "AudioDummy.h"
#include "ToolPlugin.h"
#include "ToolPluginView.h"
#include "tool_button.h"
#include "ProjectJournal.h"
#include "AutomationEditor.h"
#include "templates.h"
#include "lcd_spinbox.h"
#include "tooltip.h"
#include "MeterDialog.h"
#include "automatable_slider.h"
#include "text_float.h"
#include "cpuload_widget.h"
#include "visualization_widget.h"
#include "ResourceBrowser.h"
#include "QuickLoadDialog.h"
#include "WelcomeScreen.h"

#include "gui/tracks/track_container_scene.h"


MainWindow::MainWindow() :
	m_workspace( NULL ),
	m_templatesMenu( NULL ),
	m_recentlyOpenedProjectsMenu( NULL ),
	m_toolsMenu( NULL ),
	m_autoSaveTimer( this )
{
	setAttribute( Qt::WA_DeleteOnClose );

	m_mainWidget = new QWidget( this );
	QVBoxLayout * vbox = new QVBoxLayout( m_mainWidget );
	vbox->setSpacing( 0 );
	vbox->setMargin( 0 );

	QWidget * w = new QWidget( m_mainWidget );
	QHBoxLayout * hbox = new QHBoxLayout( w );
	hbox->setSpacing( 0 );
	hbox->setMargin( 0 );

	SideBar * sideBar = new SideBar( Qt::Vertical, w );

	QSplitter * splitter = new QSplitter( Qt::Horizontal, w );
	splitter->setChildrenCollapsible( false );

	QString wdir = configManager::inst()->workingDir();
	sideBar->appendTab( new pluginBrowser( splitter ) );

	// add a resource browser to sidebar
	m_resourceBrowser = new ResourceBrowser( splitter );
	sideBar->appendTab( m_resourceBrowser );


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
	m_toolBar = new QWidget( m_mainWidget );
	m_toolBar->setObjectName( "mainToolbar" );
	m_toolBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_toolBar->setFixedHeight( 64 );
	m_toolBar->move( 0, 0 );

	// add layout for organizing quite complex toolbar-layouting
	m_toolBarLayout = new QHBoxLayout( m_toolBar );
	m_toolBarLayout->setMargin( 0 );
	m_toolBarLayout->setSpacing( 0 );

	vbox->addWidget( m_toolBar );
	vbox->addWidget( w );

	m_updateTimer.start( 1000 / 20, this );	// 20 fps

	// connect auto save
	connect(&m_autoSaveTimer, SIGNAL(timeout()), this, SLOT(autoSave()));
	m_autoSaveTimer.start(1000 * 60); // 1 minute

	m_welcomeScreen = new WelcomeScreen( this );
	m_welcomeScreen->setVisible( false );
}



MainWindow::~MainWindow()
{
	for( QList<PluginView *>::Iterator it = m_tools.begin();
						it != m_tools.end(); ++it )
	{
		Model * m = ( *it )->model();
		delete *it;
		delete m;
	}

	delete m_resourceBrowser;

	// TODO: Close tools
	// destroy engine which will do further cleanups etc.
	engine::destroy();
}



void MainWindow::showWelcomeScreen(bool _visible)
{
	m_welcomeScreen->setVisible( _visible );
	setCentralWidget( _visible ? m_welcomeScreen : m_mainWidget );
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
	edit_menu->addAction( embed::getIconPixmap( "---TODO---" ),
					tr( "Load resource into current context" ),
					this, SLOT( loadResource() ),
					Qt::CTRL + Qt::Key_L );
	edit_menu->addSeparator();
	edit_menu->addAction( embed::getIconPixmap( "setup_general" ),
					tr( "Settings" ),
					this, SLOT( showSettingsDialog() ) );
	edit_menu->addSeparator();
	edit_menu->addAction( embed::getIconPixmap( "setup_general" ),
					tr( "Preferences (premature dialog)" ),
					this, SLOT( showPreferencesDialog() ) );


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
	if( true )
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

	// create the grid layout for the first buttons area
	QWidget * gridButtons_w = new QWidget( m_toolBar );
	QGridLayout * gridButtons_layout = new QGridLayout( gridButtons_w );

	// create tool-buttons
	toolButton * project_new = new toolButton(
					embed::getIconPixmap( "project_new" ),
					tr( "Create new project" ),
					this, SLOT( createNewProject() ),
							gridButtons_w );

	toolButton * project_new_from_template = new toolButton(
			embed::getIconPixmap( "project_new_from_template" ),
				tr( "Create new project from template" ),
					this, SLOT( emptySlot() ),
							gridButtons_w );

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
								gridButtons_w );


	toolButton * project_open_recent = new toolButton(
				embed::getIconPixmap( "project_open_recent" ),
					tr( "Recently opened project" ),
					this, SLOT( emptySlot() ), gridButtons_w );
	project_open_recent->setMenu( m_recentlyOpenedProjectsMenu );
	project_open_recent->setPopupMode( toolButton::InstantPopup );

	toolButton * project_save = new toolButton(
					embed::getIconPixmap( "project_save" ),
					tr( "Save current project" ),
					this, SLOT( saveProject() ),
								gridButtons_w );


	toolButton * project_export = new toolButton(
				embed::getIconPixmap( "project_export" ),
					tr( "Export current project" ),
					engine::getSong(),
							SLOT( exportProject() ),
								gridButtons_w );

	gridButtons_layout->setMargin( 0 );
	gridButtons_layout->setSpacing( 0 );
	gridButtons_layout->setColumnMinimumWidth( 0, 5 );
	gridButtons_layout->addWidget( project_new, 0, 1 );
	gridButtons_layout->addWidget( project_new_from_template, 0, 2 );
	gridButtons_layout->addWidget( project_open, 0, 3 );
	gridButtons_layout->addWidget( project_open_recent, 0, 4 );
	gridButtons_layout->addWidget( project_save, 0, 5 );
	gridButtons_layout->addWidget( project_export, 0, 6 );



	// window-toolbar
	toolButton * song_editor_window = new toolButton(
					embed::getIconPixmap( "songeditor" ),
					tr( "Show/hide Song-Editor" ) + " (F5)",
					this, SLOT( toggleSongEditorWin() ),
								gridButtons_w );
	song_editor_window->setShortcut( Qt::Key_F5 );
	song_editor_window->setWhatsThis(
		tr( "By pressing this button, you can show or hide the "
			"Song-Editor. With the help of the Song-Editor you can "
			"edit song-playlist and specify when which track "
			"should be played. "
			"You can also insert and move samples (e.g. "
			"rap samples) directly into the playlist." ) );


	toolButton * bb_editor_window = new toolButton(
					embed::getIconPixmap( "bb_track" ),
					tr( "Show/hide Beat+Bassline Editor" ) +
									" (F6)",
					this, SLOT( toggleBBEditorWin() ),
								gridButtons_w );
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
								gridButtons_w );
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
					gridButtons_w );
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
					gridButtons_w );
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
								gridButtons_w );
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
								gridButtons_w );
	controllers_window->setShortcut( Qt::Key_F11 );

	gridButtons_layout->addWidget( song_editor_window, 1, 1 );
	gridButtons_layout->addWidget( bb_editor_window, 1, 2 );
	gridButtons_layout->addWidget( piano_roll_window, 1, 3 );
	gridButtons_layout->addWidget( automation_editor_window, 1, 4 );
	gridButtons_layout->addWidget( fx_mixer_window, 1, 5 );
	gridButtons_layout->addWidget( project_notes_window, 1, 6 );
	gridButtons_layout->addWidget( controllers_window, 1, 7 );
	gridButtons_layout->setColumnStretch( 100, 1 );


	m_toolBarLayout->addWidget( gridButtons_w, 0, Qt::AlignLeft );

	m_toolBarLayout->insertSpacing( -1, 10 );

	// container for tempo/bpm and high quality
	QWidget * tempo_hq_w = new QWidget( m_toolBar );
	QVBoxLayout * tempo_hq_layout = new QVBoxLayout( tempo_hq_w );
	tempo_hq_layout->setMargin( 0 );
	tempo_hq_layout->setSpacing( 0 );
	tempo_hq_layout->addSpacing( 22 );

	// tempo spin box
	m_tempoSpinBox = new lcdSpinBox( 3, tempo_hq_w, tr( "Tempo" ) );
	m_tempoSpinBox->setModel( &( engine::getSong()->m_tempoModel) );
	m_tempoSpinBox->setLabel( tr( "TEMPO/BPM" ) );
	toolTip::add( m_tempoSpinBox, tr( "tempo of song" ) );

	m_tempoSpinBox->setWhatsThis(
		tr( "The tempo of a song is specified in beats per minute "
			"(BPM). If you want to change the tempo of your "
			"song, change this value. Every measure has four beats, "
			"so the tempo in BPM specifies, how many measures / 4 "
			"should be played within a minute (or how many measures "
			"should be played within four minutes)." ) );

	tempo_hq_layout->addWidget( m_tempoSpinBox );

#if 0
	// high quality button
	toolButton * hq_btn = new toolButton( embed::getIconPixmap( "hq_mode" ),
						tr( "High quality mode" ),
						NULL, NULL, tempo_hq_w );
	hq_btn->setCheckable( true );
	connect( hq_btn, SIGNAL( toggled( bool ) ),
			this, SLOT( setHighQuality( bool ) ) );
	hq_btn->setFixedWidth( 42 );

	tempo_hq_layout->addWidget( hq_btn );
#endif

	// add container to main toolbar
	m_toolBarLayout->addWidget( tempo_hq_w );


	m_toolBarLayout->insertSpacing( -1, 10 );

	// time signature spin boxes
	QWidget * timeSigWidget = new QWidget( m_toolBar );
	QVBoxLayout * timeSigLayout = new QVBoxLayout( timeSigWidget );
	timeSigLayout->setMargin( 0 );
	timeSigLayout->setSpacing( 0 );
	timeSigLayout->addSpacing( 3 );

	m_timeSigDisplay = new MeterDialog( this, true );
	m_timeSigDisplay->setModel( &( engine::getSong()->m_timeSigModel ) );
	timeSigLayout->addWidget( m_timeSigDisplay );

	m_toolBarLayout->addWidget( timeSigWidget );

	m_toolBarLayout->insertSpacing( -1, 10 );

	// master volume slider
	QLabel * master_vol_lbl = new QLabel( m_toolBar );
	master_vol_lbl->setPixmap( embed::getIconPixmap( "master_volume" ) );

	m_masterVolumeSlider = new automatableSlider( m_toolBar,
							tr( "Master volume" ) );
	m_masterVolumeSlider->setModel( &( engine::getSong()->m_masterVolumeModel ) );
	m_masterVolumeSlider->setOrientation( Qt::Vertical );
	m_masterVolumeSlider->setPageStep( 1 );
	m_masterVolumeSlider->setTickPosition( QSlider::TicksLeft );
	m_masterVolumeSlider->setFixedSize( 26, 60 );
	m_masterVolumeSlider->setTickInterval( 50 );
	toolTip::add( m_masterVolumeSlider, tr( "master volume" ) );

	connect( m_masterVolumeSlider, SIGNAL( logicValueChanged( int ) ), this,
			SLOT( masterVolumeChanged( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderPressed() ), this,
			SLOT( masterVolumePressed() ) );
	connect( m_masterVolumeSlider, SIGNAL( logicSliderMoved( int ) ), this,
			SLOT( masterVolumeMoved( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderReleased() ), this,
			SLOT( masterVolumeReleased() ) );

	m_mvsStatus = new textFloat;
	m_mvsStatus->setTitle( tr( "Master volume" ) );
	m_mvsStatus->setPixmap( embed::getIconPixmap( "master_volume" ) );

	m_toolBarLayout->addWidget( master_vol_lbl, 0, Qt::AlignLeft );
	m_toolBarLayout->addWidget( m_masterVolumeSlider, 0, Qt::AlignLeft );

	m_toolBarLayout->insertSpacing( -1, 10 );

	// master pitch slider
	QLabel * master_pitch_lbl = new QLabel( m_toolBar );
	master_pitch_lbl->setPixmap( embed::getIconPixmap( "master_pitch" ) );
	master_pitch_lbl->setFixedHeight( 64 );

	m_masterPitchSlider = new automatableSlider( m_toolBar, tr( "Master pitch" ) );
	m_masterPitchSlider->setModel( &( engine::getSong()->m_masterPitchModel ) );
	m_masterPitchSlider->setOrientation( Qt::Vertical );
	m_masterPitchSlider->setPageStep( 1 );
	m_masterPitchSlider->setTickPosition( QSlider::TicksLeft );
	m_masterPitchSlider->setFixedSize( 26, 60 );
	m_masterPitchSlider->setTickInterval( 12 );
	toolTip::add( m_masterPitchSlider, tr( "master pitch" ) );
	connect( m_masterPitchSlider, SIGNAL( logicValueChanged( int ) ), this,
			SLOT( masterPitchChanged( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderPressed() ), this,
			SLOT( masterPitchPressed() ) );
	connect( m_masterPitchSlider, SIGNAL( logicSliderMoved( int ) ), this,
			SLOT( masterPitchMoved( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderReleased() ), this,
			SLOT( masterPitchReleased() ) );

	m_mpsStatus = new textFloat;
	m_mpsStatus->setTitle( tr( "Master pitch" ) );
	m_mpsStatus->setPixmap( embed::getIconPixmap( "master_pitch" ) );

	m_toolBarLayout->addWidget( master_pitch_lbl, 0, Qt::AlignLeft );
	m_toolBarLayout->addWidget( m_masterPitchSlider, 0, Qt::AlignLeft );

	m_toolBarLayout->insertSpacing( -1, 10 );

	// box to hold all new controls we're adding to main toolbar
	//QWidget * add_w = new QWidget( tb );
	//QHBoxLayout * add_layout = new QHBoxLayout( add_w );

	// create widget for visualization- and cpu-load-widget
	QWidget * vc_w = new QWidget( m_toolBar );
	QVBoxLayout * vcw_layout = new QVBoxLayout( vc_w );
	vcw_layout->setMargin( 0 );
	vcw_layout->setSpacing( 0 );

	//vcw_layout->addStretch();
	vcw_layout->addWidget( new visualizationWidget(
			embed::getIconPixmap( "output_graph" ), vc_w ) );

	vcw_layout->addWidget( new cpuloadWidget( vc_w ) );
	vcw_layout->addStretch();

	m_toolBarLayout->addWidget( vc_w, 0, Qt::AlignLeft );
	m_toolBarLayout->insertSpacing( -1, 10 );


	// global playback and record controls
	// main box
	QWidget * gpbr_w = new QWidget( m_toolBar );
	QHBoxLayout * gpbrw_layout = new QHBoxLayout( gpbr_w );

	// playback half
	QWidget * gpb_w = new QWidget( gpbr_w );
	QVBoxLayout * gpbw_layout = new QVBoxLayout( gpb_w );
	gpbw_layout->setMargin( 0 );
	gpbw_layout->setSpacing( 0 );
	gpbw_layout->addWidget( new QLabel( tr( "PLAYBACK" ), gpb_w ) );

	QWidget * btns = new QWidget( gpb_w );
	QHBoxLayout * btns_layout = new QHBoxLayout( btns );
	btns_layout->setMargin(0);
	btns_layout->setSpacing(0);


	toolButton * m_playButton =
		new toolButton(
					   embed::getIconPixmap( "play" ),
					   tr( "Play/pause the project (Space)" ),
					   this,
					   SLOT(play()),
					   btns );

	toolButton * m_recordButton =
		new toolButton(
						embed::getIconPixmap( "record" ),
						tr( "Record from the checked items to the right" ),
						this,
						SLOT(record()),
						btns );

	toolButton * m_recordAccompanyButton =
		new toolButton(
					   embed::getIconPixmap( "record_accompany" ),
					   tr( "Record from the checked items to "
						   "the right while playing" ),
					   this,
					   SLOT(playAndRecord()),
					   btns );

	toolButton * m_stopButton =
		new toolButton(
					   embed::getIconPixmap( "stop" ),
					   tr( "Stop playing whatever is playing" ),
					   this,
					   SLOT( stop() ),
					   btns );

	btns_layout->addWidget( m_playButton );
	btns_layout->addWidget( m_recordButton );
	btns_layout->addWidget( m_recordAccompanyButton );
	btns_layout->addWidget( m_stopButton );

	gpbw_layout->addWidget( btns );

	// choose between song or BB to playback
	QWidget * pbopt_w = new QWidget( gpb_w );
	QHBoxLayout * pbopt_layout = new QHBoxLayout( pbopt_w );
	pbopt_layout->setSpacing(0);
	pbopt_layout->setMargin(0);

	m_radpSong = new QRadioButton( tr( "Song" ), pbopt_w );
	m_radpSong->setToolTip( tr("Playback: song mode") );
	connect(m_radpSong, SIGNAL(clicked(bool)), SLOT(playbackSongClicked(bool)));
	m_radpBB = new QRadioButton( tr( "B+B" ), pbopt_w );
	m_radpBB->setToolTip( tr("Playback: beat+bassline mode") );
	connect(m_radpBB, SIGNAL(clicked(bool)), SLOT(playbackBBClicked(bool)));
	m_radpPianoRoll = new QRadioButton( tr( "Piano Roll" ), pbopt_w );
	m_radpPianoRoll->setToolTip( tr("Playback: piano roll mode") );
	connect(m_radpPianoRoll, SIGNAL(clicked(bool)), SLOT(playbackPianoRollClicked(bool)));


	pbopt_layout->setMargin(0);
	pbopt_layout->setSpacing(0);
	pbopt_layout->addWidget( m_radpSong );
	pbopt_layout->addWidget( m_radpBB );
	pbopt_layout->addWidget( m_radpPianoRoll );

	gpbw_layout->addWidget( pbopt_w );

	gpbw_layout->addStretch();

	QWidget * gr_w = new QWidget( gpbr_w );
	QVBoxLayout * grw_layout = new QVBoxLayout( gr_w );
	grw_layout->setMargin( 0 );
	grw_layout->setSpacing( 0 );


	m_chkrAudio = new QCheckBox( tr( "Audio-device" ), gr_w );
	m_chkrAutomation = new QCheckBox( tr( "Automation" ), gr_w );
	m_chkrMidi = new QCheckBox( tr( "MIDI" ), gr_w );

	connect(m_chkrAutomation, SIGNAL(toggled(bool)), SLOT(toggleRecordAutomation(bool)));

	grw_layout->addWidget( new QLabel( tr( "RECORD" ), gr_w ) );
	grw_layout->addWidget( m_chkrAudio );
	grw_layout->addWidget( m_chkrAutomation );
	grw_layout->addWidget( m_chkrMidi );


	gpbrw_layout->setMargin( 0 );
	gpbrw_layout->setSpacing( 0 );
	gpbrw_layout->addWidget( gpb_w );
	gpbrw_layout->addSpacing( 20 );
	gpbrw_layout->addWidget( gr_w );
	gpbrw_layout->addStretch();


	m_toolBarLayout->addWidget( gpbr_w );

	// initial toolbar settings
	m_chkrAutomation->click();
	m_chkrMidi->click();
	m_radpSong->click();

	// global keyboard shortcuts
	QShortcut * qs_space = new QShortcut(QKeySequence(Qt::Key_Space), this);
	connect(qs_space, SIGNAL(activated()), SLOT(shortcutSpacePressed()));
	QShortcut * qs_L = new QShortcut(QKeySequence(Qt::Key_L), this);
	connect(qs_L, SIGNAL(activated()), SLOT(shortcutLPressed()));


	// setup-dialog opened before?
	if( !configManager::inst()->value( "app", "configured" ).toInt() )
	{
		configManager::inst()->setValue( "app", "configured", "1" );
		// no, so show it that user can setup everything
		setupDialog sd;
		sd.exec();
	}
	// look whether mixer could use a audio-interface beside AudioDummy
	else if( engine::getMixer()->audioDevName() == AudioDummy::name() )
	{
		// no, so we offer setup-dialog with audio-settings...
		setupDialog sd( setupDialog::AudioSettings );
		sd.exec();
	}
}



/*
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
	return col;
}




void MainWindow::addSpacingToToolBar( int _size )
{
	m_toolBarLayout->setColumnMinimumWidth( m_toolBarLayout->columnCount() +
								7, _size );
}
*/



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
		return true;
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
		return saveProject();
	}
	else if( answer == QMessageBox::Discard )
	{
		return true;
	}

	return false;
}




void MainWindow::clearKeyModifiers()
{
	m_keyMods.m_ctrl = false;
	m_keyMods.m_shift = false;
	m_keyMods.m_alt = false;
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
		QFileDialog ofd( this, tr( "Open project" ), "",
			tr( "MultiMedia Project (*.mmp *.mmpz *.xml)" ) );
		ofd.setDirectory( configManager::inst()->userProjectsDir() );
		ofd.setFileMode( QFileDialog::ExistingFiles );
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

void MainWindow::playbackSongClicked( bool checked )
{
	m_playbackMode = PPM_Song;
}


void MainWindow::playbackBBClicked( bool checked )
{
	m_playbackMode = PPM_BB;
}

void MainWindow::playbackPianoRollClicked( bool checked )
{
	m_playbackMode = PPM_PianoRoll;
}


void MainWindow::openRecentlyOpenedProject( QAction * _action )
{
	const QString & f = _action->text();
	setCursor( Qt::WaitCursor );
	engine::getSong()->loadProject( f );
	configManager::inst()->addRecentlyOpenedProject( f );
	setCursor( Qt::ArrowCursor );
}




bool MainWindow::saveProject()
{
	if( engine::getSong()->projectFileName() == "" )
	{
		return saveProjectAs();
	}
	else
	{
		engine::getSong()->guiSaveProject();
	}
	return true;
}




bool MainWindow::saveProjectAs()
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
		engine::getSong()->guiSaveProjectAs(
						sfd.selectedFiles()[0] );
		return true;
	}
	return false;
}




void MainWindow::showSettingsDialog()
{
	setupDialog sd;
	sd.exec();
}




void MainWindow::showPreferencesDialog()
{
	PreferencesDialog().exec();
}




void MainWindow::aboutLMMS()
{
	AboutDialog().exec();
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




void MainWindow::toggleWindow( QWidget * _w )
{
	QWidget * parent = _w->parentWidget();

	if( m_workspace->activeSubWindow() != parent
				|| parent->isHidden() )
	{
		parent->show();
		_w->show();
		_w->setFocus();
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




void MainWindow::toggleBBEditorWin()
{
	toggleWindow( engine::getBBEditor() );
}




void MainWindow::toggleSongEditorWin()
{
	toggleWindow( engine::getSongEditor() );
}




void MainWindow::toggleProjectNotesWin()
{
	toggleWindow( engine::getProjectNotes() );
}




void MainWindow::togglePianoRollWin()
{
	toggleWindow( engine::getPianoRoll() );
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




void MainWindow::undo()
{
	engine::projectJournal()->undo();
}




void MainWindow::redo()
{
	engine::projectJournal()->redo();
}




void MainWindow::loadResource()
{
	QuickLoadDialog( this ).exec();
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


void MainWindow::showEvent( QShowEvent * _se )
{
	//showWelcomeScreen( false ); // must explicitly ask for welcome screen
	_se->accept();
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




void MainWindow::shortcutSpacePressed()
{
	if( engine::getSong()->isPlaying() )
	{
		if(
			( engine::getSong()->playMode() == song::Mode_PlaySong &&
				m_playbackMode == PPM_Song ) ||
			( engine::getSong()->playMode() == song::Mode_PlayBB &&
				m_playbackMode == PPM_BB ) ||
			( engine::getSong()->playMode() == song::Mode_PlayPattern &&
				m_playbackMode == PPM_PianoRoll ) )
		{
			// we should stop playback because the user did not
			// change which mode to playback
			stop();
		}
		else
		{
			// stop playback and then play it again with the
			// current selected playback mode
			stop();
			play();
		}
	}
	else
	{
		play();
	}
}




void MainWindow::shortcutLPressed()
{
	// cycle through global playback mode
	if( m_playbackMode == PPM_BB )
	{
		m_radpPianoRoll->click();
	}
	else if( m_playbackMode == PPM_PianoRoll )
	{
		m_radpSong->click();
	}
	else
	{
		m_radpBB->click();
	}
}




void MainWindow::play()
{
	if( m_playbackMode == PPM_BB )
	{
		engine::getBBEditor()->play();
	}
	else if( m_playbackMode == PPM_PianoRoll )
	{
		engine::getPianoRoll()->play();
	}
	else
	{
		engine::getSongEditor()->play();
	}
}




void MainWindow::stop()
{
	if( m_playbackMode == PPM_BB )
	{
		engine::getBBEditor()->stop();
	}
	else if( m_playbackMode == PPM_PianoRoll )
	{
		engine::getPianoRoll()->stop();
	}
	else
	{
		engine::getSongEditor()->stop();
	}
}




void MainWindow::record()
{
	if( m_playbackMode == PPM_BB )
	{
		printf("beat+bassline does not support recording\n");
	}
	else if( m_playbackMode == PPM_PianoRoll )
	{
		engine::getPianoRoll()->record();
	}
	else
	{
		engine::getSongEditor()->record();
	}
}




void MainWindow::playAndRecord()
{
	if( m_playbackMode == PPM_BB )
	{
		printf("bb editor does not support record accompany\n");
	}
	else if( m_playbackMode == PPM_PianoRoll )
	{
		engine::getPianoRoll()->recordAccompany();
	}
	else
	{
		engine::getSongEditor()->recordAccompany();
	}
}




void MainWindow::setPlaybackMode( ProjectPlaybackMode _playbackMode )
{
	if( _playbackMode == PPM_BB )
	{
		m_radpBB->click();
	}
	else if( _playbackMode == PPM_Song )
	{
		m_radpSong->click();
	}
	else
	{
		m_radpPianoRoll->click();
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




void MainWindow::setHighQuality( bool _hq )
{
	/*engine::getMixer()->changeQuality( Mixer::qualitySettings(
			_hq ? Mixer::qualitySettings::Mode_HighQuality :
				Mixer::qualitySettings::Mode_Draft ) );*/
}



void MainWindow::masterVolumeChanged( int _new_val )
{
	masterVolumeMoved( _new_val );
	if( m_mvsStatus->isVisible() == false && engine::getSong()->m_loadingProject == false
					&& m_masterVolumeSlider->showStatus() )
	{
		m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
		m_mvsStatus->setVisibilityTimeOut( 1000 );
	}
	engine::getMixer()->setMasterGain( _new_val / 100.0f );
}




void MainWindow::masterVolumePressed()
{
	m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
	m_mvsStatus->show();
	masterVolumeMoved( engine::getSong()->m_masterVolumeModel.value() );
}




void MainWindow::masterVolumeMoved( int _new_val )
{
	m_mvsStatus->setText( tr( "Value: %1%" ).arg( _new_val ) );
}




void MainWindow::masterVolumeReleased()
{
	m_mvsStatus->hide();
}




void MainWindow::masterPitchChanged( int _new_val )
{
	masterPitchMoved( _new_val );
	if( m_mpsStatus->isVisible() == false &&
			engine::getSong()->m_loadingProject == false &&
			m_masterPitchSlider->showStatus() )
	{
		m_mpsStatus->moveGlobal( m_masterPitchSlider,
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
		m_mpsStatus->setVisibilityTimeOut( 1000 );
	}
}




void MainWindow::masterPitchPressed()
{
	m_mpsStatus->moveGlobal( m_masterPitchSlider,
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
	m_mpsStatus->show();
	masterPitchMoved( engine::getSong()->m_masterPitchModel.value() );
}




void MainWindow::masterPitchMoved( int _new_val )
{
	m_mpsStatus->setText( tr( "Value: %1 semitones" ).arg( _new_val ) );

}




void MainWindow::masterPitchReleased()
{
	m_mpsStatus->hide();
}



void MainWindow::toggleRecordAutomation( bool _recording )
{
	engine::automationRecorder()->setRecording( _recording );
}



void MainWindow::autoSave()
{
	QDir work(configManager::inst()->workingDir());
	engine::getSong()->saveProjectFile(work.absoluteFilePath("recover.mmp"));
}


#include "moc_MainWindow.cxx"

/* vim: set tw=0 noexpandtab: */
