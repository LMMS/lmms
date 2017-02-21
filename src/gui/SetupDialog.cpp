/*
 * SetupDialog.cpp - dialog for setting up LMMS
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QComboBox>
#include <QImageReader>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSlider>
#include <QWhatsThis>
#include <QScrollArea>

#include "SetupDialog.h"
#include "TabBar.h"
#include "TabButton.h"
#include "TabWidget.h"
#include "gui_templates.h"
#include "Mixer.h"
#include "MainWindow.h"
#include "ProjectJournal.h"
#include "ConfigManager.h"
#include "embed.h"
#include "Engine.h"
#include "debug.h"
#include "ToolTip.h"
#include "LcdSpinBox.h"
#include "FileDialog.h"


// platform-specific audio-interface-classes
#include "AudioAlsa.h"
#include "AudioAlsaSetupWidget.h"
#include "AudioJack.h"
#include "AudioOss.h"
#include "AudioSndio.h"
#include "AudioPortAudio.h"
#include "AudioSoundIo.h"
#include "AudioPulseAudio.h"
#include "AudioSdl.h"
#include "AudioDummy.h"

// platform-specific midi-interface-classes
#include "MidiAlsaRaw.h"
#include "MidiAlsaSeq.h"
#include "MidiJack.h"
#include "MidiOss.h"
#include "MidiSndio.h"
#include "MidiWinMM.h"
#include "MidiApple.h"
#include "MidiDummy.h"



inline void labelWidget( QWidget * _w, const QString & _txt )
{
	QLabel * title = new QLabel( _txt, _w );
	QFont f = title->font();
	f.setBold( true );
	title->setFont( pointSize<12>( f ) );


	assert( dynamic_cast<QBoxLayout *>( _w->layout() ) != NULL );

	dynamic_cast<QBoxLayout *>( _w->layout() )->addSpacing( 5 );
	dynamic_cast<QBoxLayout *>( _w->layout() )->addWidget( title );
	dynamic_cast<QBoxLayout *>( _w->layout() )->addSpacing( 10 );
}




SetupDialog::SetupDialog( ConfigTabs _tab_to_open ) :
	m_bufferSize( ConfigManager::inst()->value( "mixer",
					"framesperaudiobuffer" ).toInt() ),
	m_toolTips( !ConfigManager::inst()->value( "tooltips",
							"disabled" ).toInt() ),
	m_warnAfterSetup( !ConfigManager::inst()->value( "app",
						"nomsgaftersetup" ).toInt() ),
	m_displaydBFS( ConfigManager::inst()->value( "app", 
		      				"displaydbfs" ).toInt() ),
	m_MMPZ( !ConfigManager::inst()->value( "app", "nommpz" ).toInt() ),
	m_disableBackup( !ConfigManager::inst()->value( "app",
							"disablebackup" ).toInt() ),
	m_openLastProject( ConfigManager::inst()->value( "app",
							"openlastproject" ).toInt() ),
	m_hqAudioDev( ConfigManager::inst()->value( "mixer",
							"hqaudio" ).toInt() ),
	m_lang( ConfigManager::inst()->value( "app",
							"language" ) ),
	m_workingDir( QDir::toNativeSeparators( ConfigManager::inst()->workingDir() ) ),
	m_vstDir( QDir::toNativeSeparators( ConfigManager::inst()->vstDir() ) ),
	m_artworkDir( QDir::toNativeSeparators( ConfigManager::inst()->artworkDir() ) ),
	m_ladDir( QDir::toNativeSeparators( ConfigManager::inst()->ladspaDir() ) ),
	m_gigDir( QDir::toNativeSeparators( ConfigManager::inst()->gigDir() ) ),
	m_sf2Dir( QDir::toNativeSeparators( ConfigManager::inst()->sf2Dir() ) ),
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_defaultSoundfont( QDir::toNativeSeparators( ConfigManager::inst()->defaultSoundfont() ) ),
#endif
#ifdef LMMS_HAVE_STK
	m_stkDir( QDir::toNativeSeparators( ConfigManager::inst()->stkDir() ) ),
#endif
	m_backgroundArtwork( QDir::toNativeSeparators( ConfigManager::inst()->backgroundArtwork() ) ),
	m_smoothScroll( ConfigManager::inst()->value( "ui", "smoothscroll" ).toInt() ),
	m_enableAutoSave( ConfigManager::inst()->value( "ui", "enableautosave" ).toInt() ),
	m_enableRunningAutoSave( ConfigManager::inst()->value( "ui", "enablerunningautosave" ).toInt() ),
	m_saveInterval(	ConfigManager::inst()->value( "ui", "saveinterval" ).toInt() < 1 ?
					MainWindow::DEFAULT_SAVE_INTERVAL_MINUTES :
			ConfigManager::inst()->value( "ui", "saveinterval" ).toInt() ),
	m_oneInstrumentTrackWindow( ConfigManager::inst()->value( "ui",
					"oneinstrumenttrackwindow" ).toInt() ),
	m_compactTrackButtons( ConfigManager::inst()->value( "ui",
					"compacttrackbuttons" ).toInt() ),
	m_syncVSTPlugins( ConfigManager::inst()->value( "ui",
							"syncvstplugins" ).toInt() ),
	m_animateAFP(ConfigManager::inst()->value( "ui",
						   "animateafp").toInt() ),
	m_printNoteLabels(ConfigManager::inst()->value( "ui",
						   "printnotelabels").toInt() ),
	m_displayWaveform(ConfigManager::inst()->value( "ui",
						   "displaywaveform").toInt() ),
	m_disableAutoQuit(ConfigManager::inst()->value( "ui",
						   "disableautoquit").toInt() )
{
	setWindowIcon( embed::getIconPixmap( "setup_general" ) );
	setWindowTitle( tr( "Setup LMMS" ) );
	setModal( true );
	setFixedSize( 452, 520 );

	Engine::projectJournal()->setJournalling( false );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );
	QWidget * settings = new QWidget( this );
	QHBoxLayout * hlayout = new QHBoxLayout( settings );
	hlayout->setSpacing( 0 );
	hlayout->setMargin( 0 );

	m_tabBar = new TabBar( settings, QBoxLayout::TopToBottom );
	m_tabBar->setExclusive( true );
	m_tabBar->setFixedWidth( 72 );

	QWidget * ws = new QWidget( settings );
	int wsHeight = 370;
#ifdef LMMS_HAVE_STK
	wsHeight += 50;
#endif
#ifdef LMMS_HAVE_FLUIDSYNTH
	wsHeight += 50;
#endif
	ws->setFixedSize( 360, wsHeight );
	QWidget * general = new QWidget( ws );
	general->setFixedSize( 360, 240 );
	QVBoxLayout * gen_layout = new QVBoxLayout( general );
	gen_layout->setSpacing( 0 );
	gen_layout->setMargin( 0 );
	labelWidget( general, tr( "General settings" ) );

	TabWidget * bufsize_tw = new TabWidget( tr( "BUFFER SIZE" ), general );
	bufsize_tw->setFixedHeight( 80 );

	m_bufSizeSlider = new QSlider( Qt::Horizontal, bufsize_tw );
	m_bufSizeSlider->setRange( 1, 256 );
	m_bufSizeSlider->setTickPosition( QSlider::TicksBelow );
	m_bufSizeSlider->setPageStep( 8 );
	m_bufSizeSlider->setTickInterval( 8 );
	m_bufSizeSlider->setGeometry( 10, 16, 340, 18 );
	m_bufSizeSlider->setValue( m_bufferSize / 64 );

	connect( m_bufSizeSlider, SIGNAL( valueChanged( int ) ), this,
						SLOT( setBufferSize( int ) ) );

	m_bufSizeLbl = new QLabel( bufsize_tw );
	m_bufSizeLbl->setGeometry( 10, 40, 200, 24 );
	setBufferSize( m_bufSizeSlider->value() );

	QPushButton * bufsize_reset_btn = new QPushButton(
			embed::getIconPixmap( "reload" ), "", bufsize_tw );
	bufsize_reset_btn->setGeometry( 290, 40, 28, 28 );
	connect( bufsize_reset_btn, SIGNAL( clicked() ), this,
						SLOT( resetBufSize() ) );
	ToolTip::add( bufsize_reset_btn, tr( "Reset to default-value" ) );

	QPushButton * bufsize_help_btn = new QPushButton(
			embed::getIconPixmap( "help" ), "", bufsize_tw );
	bufsize_help_btn->setGeometry( 320, 40, 28, 28 );
	connect( bufsize_help_btn, SIGNAL( clicked() ), this,
						SLOT( displayBufSizeHelp() ) );


	TabWidget * misc_tw = new TabWidget( tr( "MISC" ), general );
	const int XDelta = 10;
	const int YDelta = 18;
	const int HeaderSize = 30;
	int labelNumber = 0;


	LedCheckBox * enable_tooltips = new LedCheckBox(
							tr( "Enable tooltips" ),
								misc_tw );
	labelNumber++;
	enable_tooltips->move( XDelta, YDelta*labelNumber );
	enable_tooltips->setChecked( m_toolTips );
	connect( enable_tooltips, SIGNAL( toggled( bool ) ),
					this, SLOT( toggleToolTips( bool ) ) );


	LedCheckBox * restart_msg = new LedCheckBox(
			tr( "Show restart warning after changing settings" ),
								misc_tw );
	labelNumber++;
	restart_msg->move( XDelta, YDelta*labelNumber );
	restart_msg->setChecked( m_warnAfterSetup );
	connect( restart_msg, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleWarnAfterSetup( bool ) ) );


	LedCheckBox * dbfs = new LedCheckBox( tr( "Display volume as dBFS " ),
								misc_tw );
	labelNumber++;
	dbfs->move( XDelta, YDelta*labelNumber );
	dbfs->setChecked( m_displaydBFS );
	connect( dbfs, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleDisplaydBFS( bool ) ) );


	LedCheckBox * mmpz = new LedCheckBox(
				tr( "Compress project files per default" ),
								misc_tw );
	labelNumber++;
	mmpz->move( XDelta, YDelta*labelNumber );
	mmpz->setChecked( m_MMPZ );
	connect( mmpz, SIGNAL( toggled( bool ) ),
					this, SLOT( toggleMMPZ( bool ) ) );

	LedCheckBox * oneitw = new LedCheckBox(
				tr( "One instrument track window mode" ),
								misc_tw );
	labelNumber++;
	oneitw->move( XDelta, YDelta*labelNumber );
	oneitw->setChecked( m_oneInstrumentTrackWindow );
	connect( oneitw, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleOneInstrumentTrackWindow( bool ) ) );

	LedCheckBox * hqaudio = new LedCheckBox(
				tr( "HQ-mode for output audio-device" ),
								misc_tw );
	labelNumber++;
	hqaudio->move( XDelta, YDelta*labelNumber );
	hqaudio->setChecked( m_hqAudioDev );
	connect( hqaudio, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleHQAudioDev( bool ) ) );

	LedCheckBox * compacttracks = new LedCheckBox(
				tr( "Compact track buttons" ),
								misc_tw );
	labelNumber++;
	compacttracks->move( XDelta, YDelta*labelNumber );
	compacttracks->setChecked( m_compactTrackButtons );
	connect( compacttracks, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleCompactTrackButtons( bool ) ) );


	LedCheckBox * syncVST = new LedCheckBox(
				tr( "Sync VST plugins to host playback" ),
								misc_tw );
	labelNumber++;
	syncVST->move( XDelta, YDelta*labelNumber );
	syncVST->setChecked( m_syncVSTPlugins );
	connect( syncVST, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleSyncVSTPlugins( bool ) ) );

	LedCheckBox * noteLabels = new LedCheckBox(
				tr( "Enable note labels in piano roll" ),
								misc_tw );
	labelNumber++;
	noteLabels->move( XDelta, YDelta*labelNumber );
	noteLabels->setChecked( m_printNoteLabels );
	connect( noteLabels, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleNoteLabels( bool ) ) );

	LedCheckBox * displayWaveform = new LedCheckBox(
				tr( "Enable waveform display by default" ),
								misc_tw );
	labelNumber++;
	displayWaveform->move( XDelta, YDelta*labelNumber );
	displayWaveform->setChecked( m_displayWaveform );
	connect( displayWaveform, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleDisplayWaveform( bool ) ) );

	LedCheckBox * disableAutoquit = new LedCheckBox(
				tr( "Keep effects running even without input" ),
								misc_tw );
	labelNumber++;
	disableAutoquit->move( XDelta, YDelta*labelNumber );
	disableAutoquit->setChecked( m_disableAutoQuit );
	connect( disableAutoquit, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleDisableAutoquit( bool ) ) );

	LedCheckBox * disableBackup = new LedCheckBox(
				tr( "Create backup file when saving a project" ),
								misc_tw );
	labelNumber++;
	disableBackup->move( XDelta, YDelta*labelNumber );
	disableBackup->setChecked( m_disableBackup );
	connect( disableBackup, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleDisableBackup( bool ) ) );

	LedCheckBox * openLastProject = new LedCheckBox(
				tr( "Reopen last project on start" ),
								misc_tw );
	labelNumber++;
	openLastProject->move( XDelta, YDelta*labelNumber );
	openLastProject->setChecked( m_openLastProject );
	connect( openLastProject, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleOpenLastProject( bool ) ) );

	misc_tw->setFixedHeight( YDelta*labelNumber + HeaderSize );

	TabWidget * lang_tw = new TabWidget( tr( "LANGUAGE" ), general );
	lang_tw->setFixedHeight( 48 );
	QComboBox * changeLang = new QComboBox( lang_tw );
	changeLang->move( XDelta, YDelta );

	QDir dir( ConfigManager::inst()->localeDir() );
	QStringList fileNames = dir.entryList( QStringList( "*.qm" ) );
	for( int i = 0; i < fileNames.size(); ++i )
	{
		// get locale extracted by filename
		fileNames[i].truncate( fileNames[i].lastIndexOf( '.' ) );
		m_languages.append( fileNames[i] );
		QString lang = QLocale( m_languages.last() ).nativeLanguageName();
		changeLang->addItem( lang );
	}
	connect( changeLang, SIGNAL( currentIndexChanged( int ) ),
							this, SLOT( setLanguage( int ) ) );

	//If language unset, fallback to system language when available
	if( m_lang == "" )
	{
		QString tmp = QLocale::system().name().left( 2 );
		if( m_languages.contains( tmp ) )
		{
			m_lang = tmp;
		}
		else
		{
			m_lang = "en";
		}
	}

	for( int i = 0; i < changeLang->count(); ++i )
	{
		if( m_lang == m_languages.at( i ) )
		{
			changeLang->setCurrentIndex( i );
			break;
		}
	}

	gen_layout->addWidget( bufsize_tw );
	gen_layout->addSpacing( 10 );
	gen_layout->addWidget( misc_tw );
	gen_layout->addSpacing( 10 );
	gen_layout->addWidget( lang_tw );
	gen_layout->addStretch();



	QWidget * paths = new QWidget( ws );
	int pathsHeight = 370;
#ifdef LMMS_HAVE_STK
	pathsHeight += 55;
#endif
#ifdef LMMS_HAVE_FLUIDSYNTH
	pathsHeight += 55;
#endif
	paths->setFixedSize( 360, pathsHeight );
	QVBoxLayout * dir_layout = new QVBoxLayout( paths );
	dir_layout->setSpacing( 0 );
	dir_layout->setMargin( 0 );
	labelWidget( paths, tr( "Paths" ) );
	QLabel * title = new QLabel( tr( "Directories" ), paths );
	QFont f = title->font();
	f.setBold( true );
	title->setFont( pointSize<12>( f ) );


	QScrollArea *pathScroll = new QScrollArea( paths );

	QWidget *pathSelectors = new QWidget( ws );
	QVBoxLayout *pathSelectorLayout = new QVBoxLayout;
	pathScroll->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	pathScroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	pathScroll->resize( 362, pathsHeight - 50  );
	pathScroll->move( 0, 30 );
	pathSelectors->resize( 360, pathsHeight - 50 );

	const int txtLength = 284;
	const int btnStart = 297;


	// working-dir
	TabWidget * lmms_wd_tw = new TabWidget( tr(
					"LMMS working directory" ).toUpper(),
								pathSelectors );
	lmms_wd_tw->setFixedHeight( 48 );

	m_wdLineEdit = new QLineEdit( m_workingDir, lmms_wd_tw );
	m_wdLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_wdLineEdit, SIGNAL( textChanged( const QString & ) ), this,
				SLOT( setWorkingDir( const QString & ) ) );

	QPushButton * workingdir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
							"", lmms_wd_tw );
	workingdir_select_btn->setFixedSize( 24, 24 );
	workingdir_select_btn->move( btnStart, 16 );
	connect( workingdir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openWorkingDir() ) );


	// artwork-dir
	TabWidget * artwork_tw = new TabWidget( tr(
					"Themes directory" ).toUpper(),
								pathSelectors );
	artwork_tw->setFixedHeight( 48 );

	m_adLineEdit = new QLineEdit( m_artworkDir, artwork_tw );
	m_adLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_adLineEdit, SIGNAL( textChanged( const QString & ) ), this,
				SLOT( setArtworkDir( const QString & ) ) );

	QPushButton * artworkdir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
							"", artwork_tw );
	artworkdir_select_btn->setFixedSize( 24, 24 );
	artworkdir_select_btn->move( btnStart, 16 );
	connect( artworkdir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openArtworkDir() ) );



	// background artwork file
	TabWidget * backgroundArtwork_tw = new TabWidget( tr(
			"Background artwork" ).toUpper(), paths );
	backgroundArtwork_tw->setFixedHeight( 48 );

	m_baLineEdit = new QLineEdit( m_backgroundArtwork, 
			backgroundArtwork_tw );
	m_baLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_baLineEdit, SIGNAL( textChanged( const QString & ) ), this,
			SLOT( setBackgroundArtwork( const QString & ) ) );

	QPushButton * backgroundartworkdir_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ),
			"", backgroundArtwork_tw );
	backgroundartworkdir_select_btn->setFixedSize( 24, 24 );
	backgroundartworkdir_select_btn->move( btnStart, 16 );
	connect( backgroundartworkdir_select_btn, SIGNAL( clicked() ), this,
					SLOT( openBackgroundArtwork() ) );

	// vst-dir
	TabWidget * vst_tw = new TabWidget( tr(
					"VST-plugin directory" ).toUpper(),
								pathSelectors );
	vst_tw->setFixedHeight( 48 );

	m_vdLineEdit = new QLineEdit( m_vstDir, vst_tw );
	m_vdLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_vdLineEdit, SIGNAL( textChanged( const QString & ) ), this,
					SLOT( setVSTDir( const QString & ) ) );

	QPushButton * vstdir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
								"", vst_tw );
	vstdir_select_btn->setFixedSize( 24, 24 );
	vstdir_select_btn->move( btnStart, 16 );
	connect( vstdir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openVSTDir() ) );

	// gig-dir
	TabWidget * gig_tw = new TabWidget( tr(
					"GIG directory" ).toUpper(),
								pathSelectors );
	gig_tw->setFixedHeight( 48 );

	m_gigLineEdit = new QLineEdit( m_gigDir, gig_tw );
	m_gigLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_gigLineEdit, SIGNAL( textChanged( const QString & ) ), this,
					SLOT( setGIGDir( const QString & ) ) );

	QPushButton * gigdir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
								"", gig_tw );
	gigdir_select_btn->setFixedSize( 24, 24 );
	gigdir_select_btn->move( btnStart, 16 );
	connect( gigdir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openGIGDir() ) );

	// sf2-dir
	TabWidget * sf2_tw = new TabWidget( tr(
					"SF2 directory" ).toUpper(),
								pathSelectors );
	sf2_tw->setFixedHeight( 48 );

	m_sf2LineEdit = new QLineEdit( m_sf2Dir, sf2_tw );
	m_sf2LineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_sf2LineEdit, SIGNAL( textChanged( const QString & ) ), this,
					SLOT( setSF2Dir( const QString & ) ) );

	QPushButton * sf2dir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
								"", sf2_tw );
	sf2dir_select_btn->setFixedSize( 24, 24 );
	sf2dir_select_btn->move( btnStart, 16 );
	connect( sf2dir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openSF2Dir() ) );



	// LADSPA-dir
	TabWidget * lad_tw = new TabWidget( tr(
			"LADSPA plugin directories" ).toUpper(),
							paths );
	lad_tw->setFixedHeight( 48 );

	m_ladLineEdit = new QLineEdit( m_ladDir, lad_tw );
	m_ladLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_ladLineEdit, SIGNAL( textChanged( const QString & ) ), this,
		 		SLOT( setLADSPADir( const QString & ) ) );

	QPushButton * laddir_select_btn = new QPushButton(
				embed::getIconPixmap( "add_folder", 16, 16 ),
								"", lad_tw );
	laddir_select_btn->setFixedSize( 24, 24 );
	laddir_select_btn->move( btnStart, 16 );
	connect( laddir_select_btn, SIGNAL( clicked() ), this,
				 		SLOT( openLADSPADir() ) );


#ifdef LMMS_HAVE_STK
	// STK-dir
	TabWidget * stk_tw = new TabWidget( tr(
			"STK rawwave directory" ).toUpper(),
							paths );
	stk_tw->setFixedHeight( 48 );

	m_stkLineEdit = new QLineEdit( m_stkDir, stk_tw );
	m_stkLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_stkLineEdit, SIGNAL( textChanged( const QString & ) ), this,
		 SLOT( setSTKDir( const QString & ) ) );

	QPushButton * stkdir_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ),
								"", stk_tw );
	stkdir_select_btn->setFixedSize( 24, 24 );
	stkdir_select_btn->move( btnStart, 16 );
	connect( stkdir_select_btn, SIGNAL( clicked() ), this,
		 SLOT( openSTKDir() ) );
#endif

#ifdef LMMS_HAVE_FLUIDSYNTH
	// Soundfont
	TabWidget * sf_tw = new TabWidget( tr(
			"Default Soundfont File" ).toUpper(), paths );
	sf_tw->setFixedHeight( 48 );

	m_sfLineEdit = new QLineEdit( m_defaultSoundfont, sf_tw );
	m_sfLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_sfLineEdit, SIGNAL( textChanged( const QString & ) ), this,
		 		SLOT( setDefaultSoundfont( const QString & ) ) );

	QPushButton * sf_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
								"", sf_tw );
	sf_select_btn->setFixedSize( 24, 24 );
	sf_select_btn->move( btnStart, 16 );
	connect( sf_select_btn, SIGNAL( clicked() ), this,
				 		SLOT( openDefaultSoundfont() ) );
#endif	

	pathSelectors->setLayout( pathSelectorLayout );

	pathSelectorLayout->addWidget( lmms_wd_tw );
	pathSelectorLayout->addSpacing( 10 );
	pathSelectorLayout->addWidget( gig_tw );
	pathSelectorLayout->addSpacing( 10 );
	pathSelectorLayout->addWidget( sf2_tw );
	pathSelectorLayout->addSpacing( 10 );
	pathSelectorLayout->addWidget( vst_tw );
	pathSelectorLayout->addSpacing( 10 );
	pathSelectorLayout->addWidget( lad_tw );
#ifdef LMMS_HAVE_STK
	pathSelectorLayout->addSpacing( 10 );
	pathSelectorLayout->addWidget( stk_tw );
#endif	
#ifdef LMMS_HAVE_FLUIDSYNTH
	pathSelectorLayout->addSpacing( 10 );
	pathSelectorLayout->addWidget( sf_tw );
#endif	
	pathSelectorLayout->addSpacing( 10 );
	pathSelectorLayout->addWidget( artwork_tw );
	pathSelectorLayout->addSpacing( 10 );
	pathSelectorLayout->addStretch();
	pathSelectorLayout->addWidget( backgroundArtwork_tw );
	pathSelectorLayout->addSpacing( 10 );

	dir_layout->addWidget( pathSelectors );

	pathScroll->setWidget( pathSelectors );
	pathScroll->setWidgetResizable( true );



	QWidget * performance = new QWidget( ws );
	performance->setFixedSize( 360, 200 );
	QVBoxLayout * perf_layout = new QVBoxLayout( performance );
	perf_layout->setSpacing( 0 );
	perf_layout->setMargin( 0 );
	labelWidget( performance, tr( "Performance settings" ) );


	TabWidget * auto_save_tw = new TabWidget(
			tr( "Auto save" ).toUpper(), performance );
	auto_save_tw->setFixedHeight( 110 );

	m_saveIntervalSlider = new QSlider( Qt::Horizontal, auto_save_tw );
	m_saveIntervalSlider->setRange( 1, 20 );
	m_saveIntervalSlider->setTickPosition( QSlider::TicksBelow );
	m_saveIntervalSlider->setPageStep( 1 );
	m_saveIntervalSlider->setTickInterval( 1 );
	m_saveIntervalSlider->setGeometry( 10, 16, 340, 18 );
	m_saveIntervalSlider->setValue( m_saveInterval );

	connect( m_saveIntervalSlider, SIGNAL( valueChanged( int ) ), this,
						SLOT( setAutoSaveInterval( int ) ) );

	m_saveIntervalLbl = new QLabel( auto_save_tw );
	m_saveIntervalLbl->setGeometry( 10, 40, 200, 24 );
	setAutoSaveInterval( m_saveIntervalSlider->value() );

	m_autoSave = new LedCheckBox(
			tr( "Enable auto-save" ), auto_save_tw );
	m_autoSave->move( 10, 70 );
	m_autoSave->setChecked( m_enableAutoSave );
	connect( m_autoSave, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleAutoSave( bool ) ) );

	m_runningAutoSave = new LedCheckBox(
			tr( "Allow auto-save while playing" ), auto_save_tw );
	m_runningAutoSave->move( 20, 90 );
	m_runningAutoSave->setChecked( m_enableRunningAutoSave );
	connect( m_runningAutoSave, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleRunningAutoSave( bool ) ) );

	QPushButton * autoSaveResetBtn = new QPushButton(
			embed::getIconPixmap( "reload" ), "", auto_save_tw );
	autoSaveResetBtn->setGeometry( 290, 70, 28, 28 );
	connect( autoSaveResetBtn, SIGNAL( clicked() ), this,
						SLOT( resetAutoSave() ) );
	ToolTip::add( autoSaveResetBtn, tr( "Reset to default-value" ) );

	QPushButton * saveIntervalBtn = new QPushButton(
			embed::getIconPixmap( "help" ), "", auto_save_tw );
	saveIntervalBtn->setGeometry( 320, 70, 28, 28 );
	connect( saveIntervalBtn, SIGNAL( clicked() ), this,
						SLOT( displaySaveIntervalHelp() ) );

	m_saveIntervalSlider->setEnabled( m_enableAutoSave );
	m_runningAutoSave->setVisible( m_enableAutoSave );


	perf_layout->addWidget( auto_save_tw );
	perf_layout->addSpacing( 10 );


	TabWidget * ui_fx_tw = new TabWidget( tr( "UI effects vs. "
						"performance" ).toUpper(),
								performance );
	ui_fx_tw->setFixedHeight( 70 );

	LedCheckBox * smoothScroll = new LedCheckBox(
			tr( "Smooth scroll in Song Editor" ), ui_fx_tw );
	smoothScroll->move( 10, 20 );
	smoothScroll->setChecked( m_smoothScroll );
	connect( smoothScroll, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleSmoothScroll( bool ) ) );

	LedCheckBox * animAFP = new LedCheckBox(
				tr( "Show playback cursor in AudioFileProcessor" ),
								ui_fx_tw );
	animAFP->move( 10, 40 );
	animAFP->setChecked( m_animateAFP );
	connect( animAFP, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleAnimateAFP( bool ) ) );



	perf_layout->addWidget( ui_fx_tw );
	perf_layout->addStretch();



	QWidget * audio = new QWidget( ws );
	audio->setFixedSize( 360, 200 );
	QVBoxLayout * audio_layout = new QVBoxLayout( audio );
	audio_layout->setSpacing( 0 );
	audio_layout->setMargin( 0 );
	labelWidget( audio, tr( "Audio settings" ) );

	TabWidget * audioiface_tw = new TabWidget( tr( "AUDIO INTERFACE" ),
									audio );
	audioiface_tw->setFixedHeight( 60 );

	m_audioInterfaces = new QComboBox( audioiface_tw );
	m_audioInterfaces->setGeometry( 10, 20, 240, 22 );


	QPushButton * audio_help_btn = new QPushButton(
			embed::getIconPixmap( "help" ), "", audioiface_tw );
	audio_help_btn->setGeometry( 320, 20, 28, 28 );
	connect( audio_help_btn, SIGNAL( clicked() ), this,
						SLOT( displayAudioHelp() ) );


	// create ifaces-settings-widget
	QWidget * asw = new QWidget( audio );
	asw->setFixedHeight( 60 );

	QHBoxLayout * asw_layout = new QHBoxLayout( asw );
	asw_layout->setSpacing( 0 );
	asw_layout->setMargin( 0 );
	//asw_layout->setAutoAdd( true );

#ifdef LMMS_HAVE_JACK
	m_audioIfaceSetupWidgets[AudioJack::name()] =
					new AudioJack::setupWidget( asw );
#endif

#ifdef LMMS_HAVE_ALSA
	m_audioIfaceSetupWidgets[AudioAlsa::name()] =
					new AudioAlsaSetupWidget( asw );
#endif

#ifdef LMMS_HAVE_PULSEAUDIO
	m_audioIfaceSetupWidgets[AudioPulseAudio::name()] =
					new AudioPulseAudio::setupWidget( asw );
#endif

#ifdef LMMS_HAVE_PORTAUDIO
	m_audioIfaceSetupWidgets[AudioPortAudio::name()] =
					new AudioPortAudio::setupWidget( asw );
#endif

#ifdef LMMS_HAVE_SOUNDIO
	m_audioIfaceSetupWidgets[AudioSoundIo::name()] =
					new AudioSoundIo::setupWidget( asw );
#endif

#ifdef LMMS_HAVE_SDL
	m_audioIfaceSetupWidgets[AudioSdl::name()] =
					new AudioSdl::setupWidget( asw );
#endif

#ifdef LMMS_HAVE_OSS
	m_audioIfaceSetupWidgets[AudioOss::name()] =
					new AudioOss::setupWidget( asw );
#endif

#ifdef LMMS_HAVE_SNDIO
	m_audioIfaceSetupWidgets[AudioSndio::name()] =
					new AudioSndio::setupWidget( asw );
#endif
	m_audioIfaceSetupWidgets[AudioDummy::name()] =
					new AudioDummy::setupWidget( asw );


	for( AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
				it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		m_audioIfaceNames[tr( it.key().toLatin1())] = it.key();
	}
	for( trMap::iterator it = m_audioIfaceNames.begin();
				it != m_audioIfaceNames.end(); ++it )
	{
		QWidget * audioWidget = m_audioIfaceSetupWidgets[it.value()];
		audioWidget->hide();
		asw_layout->addWidget( audioWidget );
		m_audioInterfaces->addItem( it.key() );
	}

	QString audioDevName = 
		ConfigManager::inst()->value( "mixer", "audiodev" );
	if( audioDevName.length() == 0 )
	{
		audioDevName = Engine::mixer()->audioDevName();
		ConfigManager::inst()->setValue(
					"mixer", "audiodev", audioDevName );
	}
	m_audioInterfaces->
		setCurrentIndex( m_audioInterfaces->findText( audioDevName ) );
	m_audioIfaceSetupWidgets[audioDevName]->show();

	connect( m_audioInterfaces, SIGNAL( activated( const QString & ) ),
		this, SLOT( audioInterfaceChanged( const QString & ) ) );


	audio_layout->addWidget( audioiface_tw );
	audio_layout->addSpacing( 20 );
	audio_layout->addWidget( asw );
	audio_layout->addStretch();



	QWidget * midi = new QWidget( ws );
	QVBoxLayout * midi_layout = new QVBoxLayout( midi );
	midi_layout->setSpacing( 0 );
	midi_layout->setMargin( 0 );
	labelWidget( midi, tr( "MIDI settings" ) );

	TabWidget * midiiface_tw = new TabWidget( tr( "MIDI INTERFACE" ),
									midi );
	midiiface_tw->setFixedHeight( 60 );

	m_midiInterfaces = new QComboBox( midiiface_tw );
	m_midiInterfaces->setGeometry( 10, 20, 240, 22 );


	QPushButton * midi_help_btn = new QPushButton(
			embed::getIconPixmap( "help" ), "", midiiface_tw );
	midi_help_btn->setGeometry( 320, 20, 28, 28 );
	connect( midi_help_btn, SIGNAL( clicked() ), this,
						SLOT( displayMIDIHelp() ) );


	// create ifaces-settings-widget
	QWidget * msw = new QWidget( midi );
	msw->setFixedHeight( 60 );

	QHBoxLayout * msw_layout = new QHBoxLayout( msw );
	msw_layout->setSpacing( 0 );
	msw_layout->setMargin( 0 );
	//msw_layout->setAutoAdd( true );

#ifdef LMMS_HAVE_ALSA
	m_midiIfaceSetupWidgets[MidiAlsaSeq::name()] =
					MidiSetupWidget::create<MidiAlsaSeq>( msw );
	m_midiIfaceSetupWidgets[MidiAlsaRaw::name()] =
					MidiSetupWidget::create<MidiAlsaRaw>( msw );
#endif

#ifdef LMMS_HAVE_JACK
	m_midiIfaceSetupWidgets[MidiJack::name()] =
					MidiSetupWidget::create<MidiJack>( msw );
#endif

#ifdef LMMS_HAVE_OSS
	m_midiIfaceSetupWidgets[MidiOss::name()] =
					MidiSetupWidget::create<MidiOss>( msw );
#endif

#ifdef LMMS_HAVE_SNDIO
	m_midiIfaceSetupWidgets[MidiSndio::name()] =
					MidiSetupWidget::create<MidiSndio>( msw );
#endif

#ifdef LMMS_BUILD_WIN32
	m_midiIfaceSetupWidgets[MidiWinMM::name()] =
					MidiSetupWidget::create<MidiWinMM>( msw );
#endif

#ifdef LMMS_BUILD_APPLE
    m_midiIfaceSetupWidgets[MidiApple::name()] =
                    MidiSetupWidget::create<MidiApple>( msw );
#endif

	m_midiIfaceSetupWidgets[MidiDummy::name()] =
					MidiSetupWidget::create<MidiDummy>( msw );


	for( MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
				it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		m_midiIfaceNames[tr( it.key().toLatin1())] = it.key();
	}
	for( trMap::iterator it = m_midiIfaceNames.begin();
				it != m_midiIfaceNames.end(); ++it )
	{
		QWidget * midiWidget = m_midiIfaceSetupWidgets[it.value()];
		midiWidget->hide();
		msw_layout->addWidget( midiWidget );
		m_midiInterfaces->addItem( it.key() );
	}

	QString midiDevName = 
		ConfigManager::inst()->value( "mixer", "mididev" );
	if( midiDevName.length() == 0 )
	{
		midiDevName = Engine::mixer()->midiClientName();
		ConfigManager::inst()->setValue(
					"mixer", "mididev", midiDevName );
	}
	m_midiInterfaces->setCurrentIndex( 
		m_midiInterfaces->findText( midiDevName ) );
	m_midiIfaceSetupWidgets[midiDevName]->show();

	connect( m_midiInterfaces, SIGNAL( activated( const QString & ) ),
		this, SLOT( midiInterfaceChanged( const QString & ) ) );


	midi_layout->addWidget( midiiface_tw );
	midi_layout->addSpacing( 20 );
	midi_layout->addWidget( msw );
	midi_layout->addStretch();


	m_tabBar->addTab( general, tr( "General settings" ), 0, false, true 
			)->setIcon( embed::getIconPixmap( "setup_general" ) );
	m_tabBar->addTab( paths, tr( "Paths" ), 1, false, true 
			)->setIcon( embed::getIconPixmap(
							"setup_directories" ) );
	m_tabBar->addTab( performance, tr( "Performance settings" ), 2, false,
				true )->setIcon( embed::getIconPixmap(
							"setup_performance" ) );
	m_tabBar->addTab( audio, tr( "Audio settings" ), 3, false, true
			)->setIcon( embed::getIconPixmap( "setup_audio" ) );
	m_tabBar->addTab( midi, tr( "MIDI settings" ), 4, true, true
			)->setIcon( embed::getIconPixmap( "setup_midi" ) );


	m_tabBar->setActiveTab( _tab_to_open );

	hlayout->addWidget( m_tabBar );
	hlayout->addSpacing( 10 );
	hlayout->addWidget( ws );
	hlayout->addSpacing( 10 );
	hlayout->addStretch();

	QWidget * buttons = new QWidget( this );
	QHBoxLayout * btn_layout = new QHBoxLayout( buttons );
	btn_layout->setSpacing( 0 );
	btn_layout->setMargin( 0 );
	QPushButton * ok_btn = new QPushButton( embed::getIconPixmap( "apply" ),
						tr( "OK" ), buttons );
	connect( ok_btn, SIGNAL( clicked() ), this, SLOT( accept() ) );

	QPushButton * cancel_btn = new QPushButton( embed::getIconPixmap(
								"cancel" ),
							tr( "Cancel" ),
							buttons );
	connect( cancel_btn, SIGNAL( clicked() ), this, SLOT( reject() ) );

	btn_layout->addStretch();
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( ok_btn );
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( cancel_btn );
	btn_layout->addSpacing( 10 );

	vlayout->addWidget( settings );
	vlayout->addSpacing( 10 );
	vlayout->addWidget( buttons );
	vlayout->addSpacing( 10 );
	vlayout->addStretch();

	show();


}




SetupDialog::~SetupDialog()
{
	Engine::projectJournal()->setJournalling( true );
}




void SetupDialog::accept()
{
	ConfigManager::inst()->setValue( "mixer", "framesperaudiobuffer",
					QString::number( m_bufferSize ) );
	ConfigManager::inst()->setValue( "mixer", "audiodev",
			m_audioIfaceNames[m_audioInterfaces->currentText()] );
	ConfigManager::inst()->setValue( "mixer", "mididev",
			m_midiIfaceNames[m_midiInterfaces->currentText()] );
	ConfigManager::inst()->setValue( "tooltips", "disabled",
					QString::number( !m_toolTips ) );
	ConfigManager::inst()->setValue( "app", "nomsgaftersetup",
					QString::number( !m_warnAfterSetup ) );
	ConfigManager::inst()->setValue( "app", "displaydbfs",
					QString::number( m_displaydBFS ) );
	ConfigManager::inst()->setValue( "app", "nommpz",
						QString::number( !m_MMPZ ) );
	ConfigManager::inst()->setValue( "app", "disablebackup",
					QString::number( !m_disableBackup ) );
	ConfigManager::inst()->setValue( "app", "openlastproject",
					QString::number( m_openLastProject ) );
	ConfigManager::inst()->setValue( "mixer", "hqaudio",
					QString::number( m_hqAudioDev ) );
	ConfigManager::inst()->setValue( "ui", "smoothscroll",
					QString::number( m_smoothScroll ) );
	ConfigManager::inst()->setValue( "ui", "enableautosave",
					QString::number( m_enableAutoSave ) );
	ConfigManager::inst()->setValue( "ui", "saveinterval",
					QString::number( m_saveInterval ) );
	ConfigManager::inst()->setValue( "ui", "enablerunningautosave",
					QString::number( m_enableRunningAutoSave ) );
	ConfigManager::inst()->setValue( "ui", "oneinstrumenttrackwindow",
					QString::number( m_oneInstrumentTrackWindow ) );
	ConfigManager::inst()->setValue( "ui", "compacttrackbuttons",
					QString::number( m_compactTrackButtons ) );
	ConfigManager::inst()->setValue( "ui", "syncvstplugins",
					QString::number( m_syncVSTPlugins ) );
	ConfigManager::inst()->setValue( "ui", "animateafp",
					QString::number( m_animateAFP ) );
	ConfigManager::inst()->setValue( "ui", "printnotelabels",
					QString::number( m_printNoteLabels ) );
	ConfigManager::inst()->setValue( "ui", "displaywaveform",
					QString::number( m_displayWaveform ) );
	ConfigManager::inst()->setValue( "ui", "disableautoquit",
					QString::number( m_disableAutoQuit ) );
	ConfigManager::inst()->setValue( "app", "language", m_lang );


	ConfigManager::inst()->setWorkingDir(QDir::fromNativeSeparators(m_workingDir));
	ConfigManager::inst()->setVSTDir(QDir::fromNativeSeparators(m_vstDir));
	ConfigManager::inst()->setGIGDir(QDir::fromNativeSeparators(m_gigDir));
	ConfigManager::inst()->setSF2Dir(QDir::fromNativeSeparators(m_sf2Dir));
	ConfigManager::inst()->setArtworkDir(QDir::fromNativeSeparators(m_artworkDir));
	ConfigManager::inst()->setLADSPADir(QDir::fromNativeSeparators(m_ladDir));
#ifdef LMMS_HAVE_FLUIDSYNTH
	ConfigManager::inst()->setDefaultSoundfont( m_defaultSoundfont );
#endif
#ifdef LMMS_HAVE_STK
	ConfigManager::inst()->setSTKDir(QDir::fromNativeSeparators(m_stkDir));
#endif	
	ConfigManager::inst()->setBackgroundArtwork( m_backgroundArtwork );

	// tell all audio-settings-widget to save their settings
	for( AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
				it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		it.value()->saveSettings();
	}
	// tell all MIDI-settings-widget to save their settings
	for( MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
				it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		it.value()->saveSettings();
	}

	ConfigManager::inst()->saveConfigFile();

	QDialog::accept();
	if( m_warnAfterSetup )
	{
		QMessageBox::information( NULL, tr( "Restart LMMS" ),
					tr( "Please note that most changes "
						"won't take effect until "
						"you restart LMMS!" ),
					QMessageBox::Ok );
	}
}




void SetupDialog::setBufferSize( int _value )
{
	const int step = DEFAULT_BUFFER_SIZE / 64;
	if( _value > step && _value % step )
	{
		int mod_value = _value % step;
		if( mod_value < step / 2 )
		{
			m_bufSizeSlider->setValue( _value - mod_value );
		}
		else
		{
			m_bufSizeSlider->setValue( _value + step - mod_value );
		}
		return;
	}

	if( m_bufSizeSlider->value() != _value )
	{
		m_bufSizeSlider->setValue( _value );
	}

	m_bufferSize = _value * 64;
	m_bufSizeLbl->setText( tr( "Frames: %1\nLatency: %2 ms" ).arg(
					m_bufferSize ).arg(
						1000.0f * m_bufferSize /
				Engine::mixer()->processingSampleRate(),
						0, 'f', 1 ) );
}




void SetupDialog::resetBufSize()
{
	setBufferSize( DEFAULT_BUFFER_SIZE / 64 );
}




void SetupDialog::displayBufSizeHelp()
{
	QWhatsThis::showText( QCursor::pos(),
			tr( "Here you can setup the internal buffer-size "
					"used by LMMS. Smaller values result "
					"in a lower latency but also may cause "
					"unusable sound or bad performance, "
					"especially on older computers or "
					"systems with a non-realtime "
					"kernel." ) );
}




void SetupDialog::toggleToolTips( bool _enabled )
{
	m_toolTips = _enabled;
}




void SetupDialog::toggleWarnAfterSetup( bool _enabled )
{
	m_warnAfterSetup = _enabled;
}




void SetupDialog::toggleDisplaydBFS( bool _enabled )
{
	m_displaydBFS = _enabled;
}




void SetupDialog::toggleMMPZ( bool _enabled )
{
	m_MMPZ = _enabled;
}




void SetupDialog::toggleDisableBackup( bool _enabled )
{
	m_disableBackup = _enabled;
}




void SetupDialog::toggleOpenLastProject( bool _enabled )
{
	m_openLastProject = _enabled;
}




void SetupDialog::toggleHQAudioDev( bool _enabled )
{
	m_hqAudioDev = _enabled;
}




void SetupDialog::toggleSmoothScroll( bool _enabled )
{
	m_smoothScroll = _enabled;
}




void SetupDialog::toggleAutoSave( bool _enabled )
{
	m_enableAutoSave = _enabled;
	m_saveIntervalSlider->setEnabled( _enabled );
	m_runningAutoSave->setVisible( _enabled );
	setAutoSaveInterval( m_saveIntervalSlider->value() );
}




void SetupDialog::toggleRunningAutoSave( bool _enabled )
{
	m_enableRunningAutoSave = _enabled;
}




void SetupDialog::toggleCompactTrackButtons( bool _enabled )
{
	m_compactTrackButtons = _enabled;
}





void SetupDialog::toggleSyncVSTPlugins( bool _enabled )
{
	m_syncVSTPlugins = _enabled;
}

void SetupDialog::toggleAnimateAFP( bool _enabled )
{
	m_animateAFP = _enabled;
}


void SetupDialog::toggleNoteLabels( bool en )
{
	m_printNoteLabels = en;
}


void SetupDialog::toggleDisplayWaveform( bool en )
{
	m_displayWaveform = en;
}


void SetupDialog::toggleDisableAutoquit( bool en )
{
	m_disableAutoQuit = en;
}


void SetupDialog::toggleOneInstrumentTrackWindow( bool _enabled )
{
	m_oneInstrumentTrackWindow = _enabled;
}

void SetupDialog::setLanguage( int lang )
{
	m_lang = m_languages[lang];
}





void SetupDialog::openWorkingDir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
					tr( "Choose LMMS working directory" ), m_workingDir );
	if( new_dir != QString::null )
	{
		m_wdLineEdit->setText( new_dir );
	}
}

void SetupDialog::openGIGDir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
				tr( "Choose your GIG directory" ),
							m_gigDir );
	if( new_dir != QString::null )
	{
		m_gigLineEdit->setText( new_dir );
	}
}

void SetupDialog::openSF2Dir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
				tr( "Choose your SF2 directory" ),
							m_sf2Dir );
	if( new_dir != QString::null )
	{
		m_sf2LineEdit->setText( new_dir );
	}
}




void SetupDialog::setWorkingDir( const QString & _wd )
{
	m_workingDir = _wd;
}




void SetupDialog::openVSTDir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
				tr( "Choose your VST-plugin directory" ),
							m_vstDir );
	if( new_dir != QString::null )
	{
		m_vdLineEdit->setText( new_dir );
	}
}




void SetupDialog::setVSTDir( const QString & _vd )
{
	m_vstDir = _vd;
}

void SetupDialog::setGIGDir(const QString &_gd)
{
	m_gigDir = _gd;
}

void SetupDialog::setSF2Dir(const QString &_sfd)
{
	m_sf2Dir = _sfd;
}




void SetupDialog::openArtworkDir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
				tr( "Choose artwork-theme directory" ),
							m_artworkDir );
	if( new_dir != QString::null )
	{
		m_adLineEdit->setText( new_dir );
	}
}




void SetupDialog::setArtworkDir( const QString & _ad )
{
	m_artworkDir = _ad;
}




void SetupDialog::openLADSPADir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
				tr( "Choose LADSPA plugin directory" ),
							m_ladDir );
	if( new_dir != QString::null )
	{
		if( m_ladLineEdit->text() == "" )
		{
			m_ladLineEdit->setText( new_dir );
		}
		else
		{
			m_ladLineEdit->setText( m_ladLineEdit->text() + "," +
								new_dir );
		}
	}
}



void SetupDialog::openSTKDir()
{
#ifdef LMMS_HAVE_STK
	QString new_dir = FileDialog::getExistingDirectory( this,
				tr( "Choose STK rawwave directory" ),
							m_stkDir );
	if( new_dir != QString::null )
	{
		m_stkLineEdit->setText( new_dir );
	}
#endif
}




void SetupDialog::openDefaultSoundfont()
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	QString new_file = FileDialog::getOpenFileName( this,
				tr( "Choose default SoundFont" ), m_defaultSoundfont, 
				"SoundFont2 Files (*.sf2)" );
	
	if( new_file != QString::null )
	{
		m_sfLineEdit->setText( new_file );
	}
#endif
}




void SetupDialog::openBackgroundArtwork()
{
	QList<QByteArray> fileTypesList = QImageReader::supportedImageFormats();
	QString fileTypes;
	for( int i = 0; i < fileTypesList.count(); i++ )
	{
		if( fileTypesList[i] != fileTypesList[i].toUpper() )
		{
			if( !fileTypes.isEmpty() )
			{
				fileTypes += " ";
			}
			fileTypes += "*." + QString( fileTypesList[i] );
		}
	}

	QString dir = ( m_backgroundArtwork.isEmpty() ) ?
		m_artworkDir :
		m_backgroundArtwork;
	QString new_file = FileDialog::getOpenFileName( this,
			tr( "Choose background artwork" ), dir, 
			"Image Files (" + fileTypes + ")" );
	
	if( new_file != QString::null )
	{
		m_baLineEdit->setText( new_file );
	}
}




void SetupDialog::setLADSPADir( const QString & _fd )
{
	m_ladDir = _fd;
}




void SetupDialog::setSTKDir( const QString & _fd )
{
#ifdef LMMS_HAVE_STK
	m_stkDir = _fd;
#endif
}




void SetupDialog::setDefaultSoundfont( const QString & _sf )
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_defaultSoundfont = _sf;
#endif
}




void SetupDialog::setBackgroundArtwork( const QString & _ba )
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_backgroundArtwork = _ba;
#endif
}




void SetupDialog::setAutoSaveInterval( int value )
{
	m_saveInterval = value;
	m_saveIntervalSlider->setValue( m_saveInterval );
	QString minutes = m_saveInterval > 1 ? tr( "minutes" ) : tr( "minute" );
	minutes = QString( "%1 %2" ).arg( QString::number( m_saveInterval ), minutes );
	minutes = m_enableAutoSave ?  minutes : tr( "Disabled" );
	m_saveIntervalLbl->setText( tr( "Auto-save interval: %1" ).arg( minutes ) );
}




void SetupDialog::resetAutoSave()
{
	setAutoSaveInterval( MainWindow::DEFAULT_SAVE_INTERVAL_MINUTES );
	m_autoSave->setChecked( true );
	m_runningAutoSave->setChecked( true );
}




void SetupDialog::displaySaveIntervalHelp()
{
	QWhatsThis::showText( QCursor::pos(),
			tr( "Set the time between automatic backup to %1.\n"
			"Remember to also save your project manually. "
			"You can choose to disable saving while playing, "
			"something some older systems find difficult." ).arg(
			ConfigManager::inst()->recoveryFile() ) );
}




void SetupDialog::audioInterfaceChanged( const QString & _iface )
{
	for( AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
				it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		it.value()->hide();
	}

	m_audioIfaceSetupWidgets[m_audioIfaceNames[_iface]]->show();
}




void SetupDialog::displayAudioHelp()
{
	QWhatsThis::showText( QCursor::pos(),
				tr( "Here you can select your preferred "
					"audio-interface. Depending on the "
					"configuration of your system during "
					"compilation time you can choose "
					"between ALSA, JACK, OSS and more. "
					"Below you see a box which offers "
					"controls to setup the selected "
					"audio-interface." ) );
}




void SetupDialog::midiInterfaceChanged( const QString & _iface )
{
	for( MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
				it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		it.value()->hide();
	}

	m_midiIfaceSetupWidgets[m_midiIfaceNames[_iface]]->show();
}




void SetupDialog::displayMIDIHelp()
{
	QWhatsThis::showText( QCursor::pos(),
				tr( "Here you can select your preferred "
					"MIDI-interface. Depending on the "
					"configuration of your system during "
					"compilation time you can choose "
					"between ALSA, OSS and more. "
					"Below you see a box which offers "
					"controls to setup the selected "
					"MIDI-interface." ) );
}
