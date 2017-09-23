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
#include <QWhatsThis>
#include <QScrollArea>

#include "SetupDialog.h"
#include "TabBar.h"
#include "TabButton.h"
#include "gui_templates.h"
#include "Mixer.h"
#include "MainWindow.h"
#include "ProjectJournal.h"
#include "embed.h"
#include "Engine.h"
#include "debug.h"
#include "ToolTip.h"
#include "FileDialog.h"


// Platform-specific audio-interface-classes.
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

// Platform-specific midi-interface-classes.
#include "MidiAlsaRaw.h"
#include "MidiAlsaSeq.h"
#include "MidiJack.h"
#include "MidiOss.h"
#include "MidiSndio.h"
#include "MidiWinMM.h"
#include "MidiApple.h"
#include "MidiDummy.h"



inline void labelWidget( QWidget * w, const QString & txt )
{
	QLabel * title = new QLabel( txt, w );
	QFont f = title->font();
	f.setBold( true );
	title->setFont( pointSize<12>( f ) );


	assert( dynamic_cast<QBoxLayout *>( w->layout() ) != NULL );

	dynamic_cast<QBoxLayout *>( w->layout() )->addSpacing( 5 );
	dynamic_cast<QBoxLayout *>( w->layout() )->addWidget( title );
	dynamic_cast<QBoxLayout *>( w->layout() )->addSpacing( 10 );
}




SetupDialog::SetupDialog( ConfigTabs tab_to_open ) :
	m_warnAfterSetup( !ConfigManager::inst()->value(
			"app", "nomsgaftersetup" ).toInt() ),
	m_tooltips( !ConfigManager::inst()->value(
			"tooltips", "disabled" ).toInt() ),
	m_displaydBFS( ConfigManager::inst()->value(
			"app", "displaydbfs" ).toInt() ),
	m_displayWaveform(ConfigManager::inst()->value(
			"ui", "displaywaveform").toInt() ),
	m_printNoteLabels(ConfigManager::inst()->value(
			"ui", "printnotelabels").toInt() ),
	m_compactTrackButtons( ConfigManager::inst()->value(
			"ui", "compacttrackbuttons" ).toInt() ),
	m_oneInstrumentTrackWindow( ConfigManager::inst()->value(
			"ui", "oneinstrumenttrackwindow" ).toInt() ),
	m_MMPZ( !ConfigManager::inst()->value(
			"app", "nommpz" ).toInt() ),
	m_disableBackup( !ConfigManager::inst()->value(
			"app", "disablebackup" ).toInt() ),
	m_openLastProject( ConfigManager::inst()->value(
			"app", "openlastproject" ).toInt() ),
	m_lang( ConfigManager::inst()->value(
			"app", "language" ) ),
	m_saveInterval(	ConfigManager::inst()->value(
			"ui", "saveinterval" ).toInt() < 1 ?
			MainWindow::DEFAULT_SAVE_INTERVAL_MINUTES :
			ConfigManager::inst()->value(
			"ui", "saveinterval" ).toInt() ),
	m_enableAutoSave( ConfigManager::inst()->value(
			"ui", "enableautosave", "1" ).toInt() ),
	m_enableRunningAutoSave( ConfigManager::inst()->value(
			"ui", "enablerunningautosave", "0" ).toInt() ),
	m_smoothScroll( ConfigManager::inst()->value(
			"ui", "smoothscroll" ).toInt() ),
	m_animateAFP(ConfigManager::inst()->value(
			"ui", "animateafp", "1" ).toInt() ),
	m_syncVSTPlugins( ConfigManager::inst()->value(
			"ui", "syncvstplugins" ).toInt() ),
	m_disableAutoQuit(ConfigManager::inst()->value(
			"ui", "disableautoquit").toInt() ),
	m_hqAudioDev( ConfigManager::inst()->value(
			"mixer", "hqaudio" ).toInt() ),
	m_bufferSize( ConfigManager::inst()->value(
			"mixer", "framesperaudiobuffer" ).toInt() ),
	m_workingDir( QDir::toNativeSeparators( ConfigManager::inst()->workingDir() ) ),
	m_vstDir( QDir::toNativeSeparators( ConfigManager::inst()->vstDir() ) ),
	m_ladspaDir( QDir::toNativeSeparators( ConfigManager::inst()->ladspaDir() ) ),
	m_gigDir( QDir::toNativeSeparators( ConfigManager::inst()->gigDir() ) ),
	m_sf2Dir( QDir::toNativeSeparators( ConfigManager::inst()->sf2Dir() ) ),
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_sf2File( QDir::toNativeSeparators( ConfigManager::inst()->sf2File() ) ),
#endif
	m_themeDir( QDir::toNativeSeparators( ConfigManager::inst()->themeDir() ) ),
	m_backgroundPicFile( QDir::toNativeSeparators( ConfigManager::inst()->backgroundPicFile() ) )
{
	setWindowIcon(
			embed::getIconPixmap( "setup_general" ) );
	setWindowTitle(
			tr( "Settings" ) );
	setModal( true );
	setFixedSize( 452, 520 );

	Engine::projectJournal()->setJournalling( false );

	// Vertical layout.
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );
		
	// Main widget.
	QWidget * main_w = new QWidget( this );

	// Horizontal layout.
	QHBoxLayout * hlayout = new QHBoxLayout( main_w );
	hlayout->setSpacing( 0 );
	hlayout->setMargin( 0 );

	// The tab bar that holds the major tabs.
	m_tabBar = new TabBar( main_w, QBoxLayout::TopToBottom );
	m_tabBar->setExclusive( true );
	m_tabBar->setFixedWidth( 72 );

	// Settings widget.
	QWidget * settings_w = new QWidget( main_w );
	int settings_w_height = 370;
#ifdef LMMS_HAVE_FLUIDSYNTH
	settings_w_height += 50;
#endif
	settings_w->setFixedSize( 360, settings_w_height );

	// General widget.
	QWidget * general_w = new QWidget( settings_w );
	general_w->setFixedSize( 360, 240 );
	QVBoxLayout * general_layout = new QVBoxLayout( general_w );
	general_layout->setSpacing( 0 );
	general_layout->setMargin( 0 );
	labelWidget( general_w,
			tr( "General settings" ) );


	const int XDelta = 10;
	const int YDelta = 18;
	
	int labelNumber1 = 0;

	// GUI tab.
	TabWidget * gui_tw = new TabWidget(
			tr( "Graphical user interface (GUI)" ), general_w );

	LedCheckBox * enableTooltips = new LedCheckBox(
			tr( "Enable tooltips" ), gui_tw );
	labelNumber1++;
	enableTooltips->move( XDelta, YDelta*labelNumber1 );
	enableTooltips->setChecked( m_tooltips );
	connect( enableTooltips, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleTooltips( bool ) ) );

	LedCheckBox * dbfs = new LedCheckBox(
			tr( "Display volume as dBFS " ), gui_tw );
	labelNumber1++;
	dbfs->move( XDelta, YDelta*labelNumber1 );
	dbfs->setChecked( m_displaydBFS );
	connect( dbfs, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleDisplaydBFS( bool ) ) );

	LedCheckBox * displayWaveform = new LedCheckBox(
			tr( "Enable master oscilloscope by default" ), gui_tw );
	labelNumber1++;
	displayWaveform->move( XDelta, YDelta*labelNumber1 );
	displayWaveform->setChecked( m_displayWaveform );
	connect( displayWaveform, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleDisplayWaveform( bool ) ) );

	LedCheckBox * noteLabels = new LedCheckBox(
			tr( "Enable all note labels in piano roll" ), gui_tw );
	labelNumber1++;
	noteLabels->move( XDelta, YDelta*labelNumber1 );
	noteLabels->setChecked( m_printNoteLabels );
	connect( noteLabels, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleNoteLabels( bool ) ) );

	LedCheckBox * compacttracks = new LedCheckBox(
			tr( "Compact track buttons" ), gui_tw );
	labelNumber1++;
	compacttracks->move( XDelta, YDelta*labelNumber1 );
	compacttracks->setChecked( m_compactTrackButtons );
	connect( compacttracks, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleCompactTrackButtons( bool ) ) );

	LedCheckBox * oneitw = new LedCheckBox(
			tr( "One instrument-track-window mode" ), gui_tw );
	labelNumber1++;
	oneitw->move( XDelta, YDelta*labelNumber1 );
	oneitw->setChecked( m_oneInstrumentTrackWindow );
	connect( oneitw, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleOneInstrumentTrackWindow( bool ) ) );


	gui_tw->setFixedHeight( YDelta*labelNumber1 );


	int labelNumber2 = 0;

	// Projects tab.
	TabWidget * projects_tw = new TabWidget(
			tr( "Projects" ), general_w );


	LedCheckBox * mmpz = new LedCheckBox(
			tr( "Compress project files by default" ), projects_tw );
	labelNumber2++;
	mmpz->move( XDelta, YDelta*labelNumber2 );
	mmpz->setChecked( m_MMPZ );
	connect( mmpz, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleMMPZ( bool ) ) );

	LedCheckBox * disableBackup = new LedCheckBox(
			tr( "Create a backup file when saving a project" ), projects_tw );
	labelNumber2++;
	disableBackup->move( XDelta, YDelta*labelNumber2 );
	disableBackup->setChecked( m_disableBackup );
	connect( disableBackup, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleDisableBackup( bool ) ) );

	LedCheckBox * openLastProject = new LedCheckBox(
			tr( "Reopen last project on startup" ), projects_tw );
	labelNumber2++;
	openLastProject->move( XDelta, YDelta*labelNumber2 );
	openLastProject->setChecked( m_openLastProject );
	connect( openLastProject, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleOpenLastProject( bool ) ) );


	projects_tw->setFixedHeight( YDelta*labelNumber2 );

	// Language tab.
	TabWidget * lang_tw = new TabWidget(
			tr( "Language" ), general_w );
	lang_tw->setFixedHeight( 48 );
	QComboBox * changeLang = new QComboBox( lang_tw );
	changeLang->move( XDelta, YDelta );

	QDir dir( ConfigManager::inst()->localeDir() );
	QStringList fileNames = dir.entryList( QStringList( "*.qm" ) );
	for( int i = 0; i < fileNames.size(); ++i )
	{
		// Get locale extracted by filename.
		fileNames[i].truncate( fileNames[i].lastIndexOf( '.' ) );
		m_languages.append( fileNames[i] );
		QString lang = QLocale( m_languages.last() ).nativeLanguageName();
		changeLang->addItem( lang );
	}
	connect( changeLang, SIGNAL( currentIndexChanged( int ) ), this,
			SLOT( setLanguage( int ) ) );

	// If language unset, fallback to system language when available.
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


	// General layout ordering.
	general_layout->addWidget( gui_tw );
	general_layout->addSpacing( 10 );
	general_layout->addWidget( projects_tw );
	general_layout->addSpacing( 10 );
	general_layout->addWidget( lang_tw );
	general_layout->addStretch();



	// Performance widget.
	QWidget * performance_w = new QWidget( settings_w );
	performance_w->setFixedSize( 360, 200 );
	QVBoxLayout * performance_layout = new QVBoxLayout( performance_w );
	performance_layout->setSpacing( 0 );
	performance_layout->setMargin( 0 );
	labelWidget( performance_w,
			tr( "Performance settings" ) );

	// Autosave tab.
	TabWidget * auto_save_tw = new TabWidget(
			tr( "Autosave" ), performance_w );
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
			tr( "Enable autosave" ), auto_save_tw );
	m_autoSave->move( 10, 70 );
	m_autoSave->setChecked( m_enableAutoSave );
	connect( m_autoSave, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleAutoSave( bool ) ) );

	m_runningAutoSave = new LedCheckBox(
			tr( "Allow autosave while playing" ), auto_save_tw );
	m_runningAutoSave->move( 20, 90 );
	m_runningAutoSave->setChecked( m_enableRunningAutoSave );
	connect( m_runningAutoSave, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleRunningAutoSave( bool ) ) );

	QPushButton * autoSaveResetBtn = new QPushButton(
			embed::getIconPixmap( "reload" ), "", auto_save_tw );
	autoSaveResetBtn->setGeometry( 290, 70, 28, 28 );
	connect( autoSaveResetBtn, SIGNAL( clicked() ), this,
			SLOT( resetAutoSave() ) );
	ToolTip::add( autoSaveResetBtn,
			tr( "Reset to default value" ) );

	QPushButton * saveIntervalBtn = new QPushButton(
			embed::getIconPixmap( "help" ), "", auto_save_tw );
	saveIntervalBtn->setGeometry( 320, 70, 28, 28 );
	connect( saveIntervalBtn, SIGNAL( clicked() ), this,
			SLOT( displaySaveIntervalHelp() ) );
	ToolTip::add( saveIntervalBtn,
			tr( "Help" ) );

	m_saveIntervalSlider->setEnabled( m_enableAutoSave );
	m_runningAutoSave->setVisible( m_enableAutoSave );

	// UI effect vs. performance tab.
	TabWidget * ui_fx_tw = new TabWidget(
			tr( "UI effects vs. performance" ), performance_w );
	ui_fx_tw->setFixedHeight( 60 );

	LedCheckBox * smoothScroll = new LedCheckBox(
			tr( "Smooth scroll in song editor" ), ui_fx_tw );
	smoothScroll->move( 10, 20 );
	smoothScroll->setChecked( m_smoothScroll );
	connect( smoothScroll, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleSmoothScroll( bool ) ) );

	LedCheckBox * animAFP = new LedCheckBox(
			tr( "Display playback cursor in AudioFileProcessor" ), ui_fx_tw );
	animAFP->move( 10, 40 );
	animAFP->setChecked( m_animateAFP );
	connect( animAFP, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleAnimateAFP( bool ) ) );

	// Plugins tab.
	TabWidget * plugins_tw = new TabWidget(
			tr( "Plugins" ), performance_w );
	plugins_tw->setFixedHeight( 60 );

	LedCheckBox * syncVST = new LedCheckBox(
			tr( "Sync VST plugins to host playback" ), plugins_tw );
	syncVST->move( 10, 20 );
	syncVST->setChecked( m_syncVSTPlugins );
	connect( syncVST, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleSyncVSTPlugins( bool ) ) );

	LedCheckBox * disableAutoQuit = new LedCheckBox(
			tr( "Keep effects running even without input" ), plugins_tw );
	disableAutoQuit->move( 10, 40 );
	disableAutoQuit->setChecked( m_disableAutoQuit );
	connect( disableAutoQuit, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleDisableAutoquit( bool ) ) );


	// Performance layout ordering.
	performance_layout->addWidget( auto_save_tw );
	performance_layout->addSpacing( 10 );
	performance_layout->addWidget( ui_fx_tw );
	performance_layout->addSpacing( 10 );
	performance_layout->addWidget( plugins_tw );
	performance_layout->addStretch();



	// Audio widget.
	QWidget * audio_w = new QWidget( settings_w );
	audio_w->setFixedSize( 360, 200 );
	QVBoxLayout * audio_layout = new QVBoxLayout( audio_w );
	audio_layout->setSpacing( 0 );
	audio_layout->setMargin( 0 );
	labelWidget( audio_w,
			tr( "Audio settings" ) );

	// Audio interface tab.
	TabWidget * audioiface_tw = new TabWidget(
			tr( "Audio interface" ), audio_w );
	audioiface_tw->setFixedHeight( 60 );

	m_audioInterfaces = new QComboBox( audioiface_tw );
	m_audioInterfaces->setGeometry( 10, 20, 240, 22 );


	QPushButton * audio_help_btn = new QPushButton(
			embed::getIconPixmap( "help" ), "", audioiface_tw );
	audio_help_btn->setGeometry( 320, 20, 28, 28 );
	connect( audio_help_btn, SIGNAL( clicked() ), this,
			SLOT( displayAudioHelp() ) );
	ToolTip::add( audio_help_btn,
			tr( "Help" ) );

	// Ifaces-settings-widget.
	QWidget * as_w = new QWidget( audio_w );
	as_w->setFixedHeight( 60 );

	QHBoxLayout * as_w_layout = new QHBoxLayout( as_w );
	as_w_layout->setSpacing( 0 );
	as_w_layout->setMargin( 0 );
	// as_w_layout->setAutoAdd( true );

#ifdef LMMS_HAVE_JACK
	m_audioIfaceSetupWidgets[AudioJack::name()] = 
			new AudioJack::setupWidget( as_w );
#endif

#ifdef LMMS_HAVE_ALSA
	m_audioIfaceSetupWidgets[AudioAlsa::name()] =
			new AudioAlsaSetupWidget( as_w );
#endif

#ifdef LMMS_HAVE_PULSEAUDIO
	m_audioIfaceSetupWidgets[AudioPulseAudio::name()] =
			new AudioPulseAudio::setupWidget( as_w );
#endif

#ifdef LMMS_HAVE_PORTAUDIO
	m_audioIfaceSetupWidgets[AudioPortAudio::name()] =
			new AudioPortAudio::setupWidget( as_w );
#endif

#ifdef LMMS_HAVE_SOUNDIO
	m_audioIfaceSetupWidgets[AudioSoundIo::name()] =
			new AudioSoundIo::setupWidget( as_w );
#endif

#ifdef LMMS_HAVE_SDL
	m_audioIfaceSetupWidgets[AudioSdl::name()] =
			new AudioSdl::setupWidget( as_w );
#endif

#ifdef LMMS_HAVE_OSS
	m_audioIfaceSetupWidgets[AudioOss::name()] =
			new AudioOss::setupWidget( as_w );
#endif

#ifdef LMMS_HAVE_SNDIO
	m_audioIfaceSetupWidgets[AudioSndio::name()] =
			new AudioSndio::setupWidget( as_w );
#endif

	m_audioIfaceSetupWidgets[AudioDummy::name()] =
			new AudioDummy::setupWidget( as_w );


	for( AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
		it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		m_audioIfaceNames[
			tr( it.key().toLatin1())] = it.key();
	}
	for( trMap::iterator it = m_audioIfaceNames.begin();
		it != m_audioIfaceNames.end(); ++it )
	{
		QWidget * audioWidget = m_audioIfaceSetupWidgets[it.value()];
		audioWidget->hide();
		as_w_layout->addWidget( audioWidget );
		m_audioInterfaces->addItem( it.key() );
	}

	// If no preferred audio device is saved, save the current one.
	QString audioDevName = ConfigManager::inst()->value( "mixer", "audiodev" );
	if( audioDevName.length() == 0 )
	{
		audioDevName = Engine::mixer()->audioDevName();
		ConfigManager::inst()->setValue( "mixer", "audiodev", audioDevName );
	}
	m_audioInterfaces->
		setCurrentIndex( m_audioInterfaces->findText( audioDevName ) );
	m_audioIfaceSetupWidgets[audioDevName]->show();

	connect( m_audioInterfaces, SIGNAL( activated( const QString & ) ), this,
			SLOT( audioInterfaceChanged( const QString & ) ) );


	// HQ mode LED.
	LedCheckBox * hqaudio = new LedCheckBox(
			tr( "HQ mode for output audio device" ), audio_w );
	hqaudio->move( 10, 0 );
	hqaudio->setChecked( m_hqAudioDev );
	connect( hqaudio, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleHQAudioDev( bool ) ) );


	// Buffer size tab.
	TabWidget * bufferSize_tw = new TabWidget(
			tr( "Buffer size" ), audio_w );
	bufferSize_tw->setFixedHeight( 80 );

	m_bufferSizeSlider = new QSlider( Qt::Horizontal, bufferSize_tw );
	m_bufferSizeSlider->setRange( 1, 256 );
	m_bufferSizeSlider->setTickPosition( QSlider::TicksBelow );
	m_bufferSizeSlider->setPageStep( 8 );
	m_bufferSizeSlider->setTickInterval( 8 );
	m_bufferSizeSlider->setGeometry( 10, 16, 340, 18 );
	m_bufferSizeSlider->setValue( m_bufferSize / 64 );

	connect( m_bufferSizeSlider, SIGNAL( valueChanged( int ) ), this,
			SLOT( setBufferSize( int ) ) );

	m_bufferSizeLbl = new QLabel( bufferSize_tw );
	m_bufferSizeLbl->setGeometry( 10, 40, 200, 24 );
	setBufferSize( m_bufferSizeSlider->value() );

	QPushButton * bufferSize_reset_btn = new QPushButton(
			embed::getIconPixmap( "reload" ), "", bufferSize_tw );
	bufferSize_reset_btn->setGeometry( 290, 40, 28, 28 );
	connect( bufferSize_reset_btn, SIGNAL( clicked() ), this,
			SLOT( resetBufferSize() ) );
	ToolTip::add( bufferSize_reset_btn,
			tr( "Reset to default value" ) );

	QPushButton * bufferSize_help_btn = new QPushButton(
			embed::getIconPixmap( "help" ), "", bufferSize_tw );
	bufferSize_help_btn->setGeometry( 320, 40, 28, 28 );
	connect( bufferSize_help_btn, SIGNAL( clicked() ), this,
			SLOT( displayBufferSizeHelp() ) );
	ToolTip::add( bufferSize_help_btn,
			tr( "Help" ) );


	// Audio layout ordering.
	audio_layout->addWidget( audioiface_tw );
	audio_layout->addSpacing( 10 );
	audio_layout->addWidget( as_w );
	audio_layout->addSpacing( 10 );
	audio_layout->addWidget( hqaudio );
	audio_layout->addSpacing( 10 );
	audio_layout->addWidget( bufferSize_tw );
	audio_layout->addStretch();



	// MIDI widget.
	QWidget * midi_w = new QWidget( settings_w );
	QVBoxLayout * midi_layout = new QVBoxLayout( midi_w );
	midi_layout->setSpacing( 0 );
	midi_layout->setMargin( 0 );
	labelWidget( midi_w,
			tr( "MIDI settings" ) );

	// MIDI interface tab.
	TabWidget * midiiface_tw = new TabWidget(
			tr( "MIDI interface" ), midi_w );
	midiiface_tw->setFixedHeight( 60 );

	m_midiInterfaces = new QComboBox( midiiface_tw );
	m_midiInterfaces->setGeometry( 10, 20, 240, 22 );


	QPushButton * midi_help_btn = new QPushButton(
			embed::getIconPixmap( "help" ), "", midiiface_tw );
	midi_help_btn->setGeometry( 320, 20, 28, 28 );
	connect( midi_help_btn, SIGNAL( clicked() ), this,
			SLOT( displayMIDIHelp() ) );
	ToolTip::add( midi_help_btn,
			tr( "Help" ) );

	// Ifaces-settings-widget.
	QWidget * ms_w = new QWidget( midi_w );
	ms_w->setFixedHeight( 60 );

	QHBoxLayout * ms_w_layout = new QHBoxLayout( ms_w );
	ms_w_layout->setSpacing( 0 );
	ms_w_layout->setMargin( 0 );
	// ms_w_layout->setAutoAdd( true );

#ifdef LMMS_HAVE_ALSA
	m_midiIfaceSetupWidgets[MidiAlsaSeq::name()] =
			MidiSetupWidget::create<MidiAlsaSeq>( ms_w );
	m_midiIfaceSetupWidgets[MidiAlsaRaw::name()] =
			MidiSetupWidget::create<MidiAlsaRaw>( ms_w );
#endif

#ifdef LMMS_HAVE_JACK
	m_midiIfaceSetupWidgets[MidiJack::name()] =
			MidiSetupWidget::create<MidiJack>( ms_w );
#endif

#ifdef LMMS_HAVE_OSS
	m_midiIfaceSetupWidgets[MidiOss::name()] =
			MidiSetupWidget::create<MidiOss>( ms_w );
#endif

#ifdef LMMS_HAVE_SNDIO
	m_midiIfaceSetupWidgets[MidiSndio::name()] =
			MidiSetupWidget::create<MidiSndio>( ms_w );
#endif

#ifdef LMMS_BUILD_WIN32
	m_midiIfaceSetupWidgets[MidiWinMM::name()] =
			MidiSetupWidget::create<MidiWinMM>( ms_w );
#endif

#ifdef LMMS_BUILD_APPLE
    m_midiIfaceSetupWidgets[MidiApple::name()] =
			MidiSetupWidget::create<MidiApple>( ms_w );
#endif

	m_midiIfaceSetupWidgets[MidiDummy::name()] =
			MidiSetupWidget::create<MidiDummy>( ms_w );


	for( MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
		it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		m_midiIfaceNames[
			tr( it.key().toLatin1())] = it.key();
	}
	for( trMap::iterator it = m_midiIfaceNames.begin();
		it != m_midiIfaceNames.end(); ++it )
	{
		QWidget * midiWidget = m_midiIfaceSetupWidgets[it.value()];
		midiWidget->hide();
		ms_w_layout->addWidget( midiWidget );
		m_midiInterfaces->addItem( it.key() );
	}

	QString midiDevName = ConfigManager::inst()->value( "mixer", "mididev" );
	if( midiDevName.length() == 0 )
	{
		midiDevName = Engine::mixer()->midiClientName();
		ConfigManager::inst()->setValue( "mixer", "mididev", midiDevName );
	}
	m_midiInterfaces->setCurrentIndex( m_midiInterfaces->findText( midiDevName ) );
	m_midiIfaceSetupWidgets[midiDevName]->show();

	connect( m_midiInterfaces, SIGNAL( activated( const QString & ) ), this,
			SLOT( midiInterfaceChanged( const QString & ) ) );


	// MIDI layout ordering.
	midi_layout->addWidget( midiiface_tw );
	midi_layout->addSpacing( 10 );
	midi_layout->addWidget( ms_w );
	midi_layout->addStretch();



	// Paths widget.
	QWidget * paths_w = new QWidget( settings_w );
	int paths_height = 370;
#ifdef LMMS_HAVE_FLUIDSYNTH
	paths_height += 55;
#endif
	paths_w->setFixedSize( 360, paths_height );
	QVBoxLayout * paths_layout = new QVBoxLayout( paths_w );
	paths_layout->setSpacing( 0 );
	paths_layout->setMargin( 0 );
	labelWidget( paths_w,
			tr( "Paths settings" ) );
	QLabel * title = new QLabel(
			tr( "Paths settings" ), paths_w );
	QFont f = title->font();
	f.setBold( true );
	title->setFont( pointSize<12>( f ) );


	QScrollArea * pathsScroll = new QScrollArea( paths_w );

	// Path selectors widget.
	QWidget * pathSelectors = new QWidget( settings_w );
	QVBoxLayout * pathSelectorsLayout = new QVBoxLayout;
	pathsScroll->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	pathsScroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	pathsScroll->resize( 360, paths_height - 50  ); // .362.
	pathsScroll->move( 0, 30 );
	pathSelectors->resize( 360, paths_height - 50 );

	const int txtLength = 284;
	const int btnStart = 300; // .297.


	// LMMS working directory tab.
	TabWidget * workingDir_tw = new TabWidget(
			tr( "LMMS working directory" ), pathSelectors );
	workingDir_tw->setFixedHeight( 48 );

	m_workingDirLineEdit = new QLineEdit( m_workingDir, workingDir_tw );
	m_workingDirLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_workingDirLineEdit, SIGNAL( textChanged( const QString & ) ), this,
			SLOT( setWorkingDir( const QString & ) ) );

	QPushButton * workingDir_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ), "", workingDir_tw );
	workingDir_select_btn->setFixedSize( 24, 24 );
	workingDir_select_btn->move( btnStart, 16 );
	connect( workingDir_select_btn, SIGNAL( clicked() ), this,
			SLOT( openWorkingDir() ) );

	// VST plugins directory tab.
	TabWidget * vstDir_tw = new TabWidget(
			tr( "VST plugins directory" ),
								pathSelectors );
	vstDir_tw->setFixedHeight( 48 );

	m_vstDirLineEdit = new QLineEdit( m_vstDir, vstDir_tw );
	m_vstDirLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_vstDirLineEdit, SIGNAL( textChanged( const QString & ) ), this,
			SLOT( setVSTDir( const QString & ) ) );

	QPushButton * vstDir_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ), "", vstDir_tw );
	vstDir_select_btn->setFixedSize( 24, 24 );
	vstDir_select_btn->move( btnStart, 16 );
	connect( vstDir_select_btn, SIGNAL( clicked() ), this,
			SLOT( openVSTDir() ) );

	// LADSPA plugins directory tab.
	TabWidget * ladspaDir_tw = new TabWidget(
			tr( "LADSPA plugins directories" ), paths_w );
	ladspaDir_tw->setFixedHeight( 48 );

	m_ladspaDirLineEdit = new QLineEdit( m_ladspaDir, ladspaDir_tw );
	m_ladspaDirLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_ladspaDirLineEdit, SIGNAL( textChanged( const QString & ) ), this,
		 	SLOT( setLADSPADir( const QString & ) ) );

	QPushButton * ladspaDir_select_btn = new QPushButton(
			embed::getIconPixmap( "add_folder", 16, 16 ), "", ladspaDir_tw );
	ladspaDir_select_btn->setFixedSize( 24, 24 );
	ladspaDir_select_btn->move( btnStart, 16 );
	connect( ladspaDir_select_btn, SIGNAL( clicked() ), this,
			SLOT( openLADSPADir() ) );

	// SF2 files direcroty tab.
	TabWidget * sf2Dir_tw = new TabWidget(
			tr( "SF2 files directory" ), pathSelectors );
	sf2Dir_tw->setFixedHeight( 48 );

	m_sf2DirLineEdit = new QLineEdit( m_sf2Dir, sf2Dir_tw );
	m_sf2DirLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_sf2DirLineEdit, SIGNAL( textChanged( const QString & ) ), this,
			SLOT( setSF2Dir( const QString & ) ) );

	QPushButton * sf2Dir_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ), "", sf2Dir_tw );
	sf2Dir_select_btn->setFixedSize( 24, 24 );
	sf2Dir_select_btn->move( btnStart, 16 );
	connect( sf2Dir_select_btn, SIGNAL( clicked() ), this,
			SLOT( openSF2Dir() ) );
	
#ifdef LMMS_HAVE_FLUIDSYNTH
	// Default SF2 file tab.
	TabWidget * sf2File_tw = new TabWidget(
			tr( "Default SF2 file" ), paths_w );
	sf2File_tw->setFixedHeight( 48 );

	m_sf2FileLineEdit = new QLineEdit( m_sf2File, sf2File_tw );
	m_sf2FileLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_sf2FileLineEdit, SIGNAL( textChanged( const QString & ) ), this,
		 	SLOT( setSF2File( const QString & ) ) );

	QPushButton * sf2File_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ), "", sf2File_tw );
	sf2File_select_btn->setFixedSize( 24, 24 );
	sf2File_select_btn->move( btnStart, 16 );
	connect( sf2File_select_btn, SIGNAL( clicked() ), this,
			SLOT( openSF2File() ) );
#endif
		
	// GIG files directory tab.
	TabWidget * gigDir_tw = new TabWidget(
			tr( "GIG files directory" ), pathSelectors );
	gigDir_tw->setFixedHeight( 48 );

	m_gigDirLineEdit = new QLineEdit( m_gigDir, gigDir_tw );
	m_gigDirLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_gigDirLineEdit, SIGNAL( textChanged( const QString & ) ), this,
			SLOT( setGIGDir( const QString & ) ) );

	QPushButton * gigDir_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ), "", gigDir_tw );
	gigDir_select_btn->setFixedSize( 24, 24 );
	gigDir_select_btn->move( btnStart, 16 );
	connect( gigDir_select_btn, SIGNAL( clicked() ), this,
			SLOT( openGIGDir() ) );

	// Theme directory tab.
	TabWidget * themeDir_tw = new TabWidget(
			tr( "Theme direcroty" ), pathSelectors );
	themeDir_tw->setFixedHeight( 48 );

	m_themeDirLineEdit = new QLineEdit( m_themeDir, themeDir_tw );
	m_themeDirLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_themeDirLineEdit, SIGNAL( textChanged( const QString & ) ), this,
			SLOT( setThemeDir( const QString & ) ) );

	QPushButton * themeDir_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ), "", themeDir_tw );
	themeDir_select_btn->setFixedSize( 24, 24 );
	themeDir_select_btn->move( btnStart, 16 );
	connect( themeDir_select_btn, SIGNAL( clicked() ), this,
			SLOT( openThemeDir() ) );

	// Background picture file tab.
	TabWidget * backgroundPicFile_tw = new TabWidget(
			tr( "Background picture file" ), paths_w );
	backgroundPicFile_tw->setFixedHeight( 48 );

	m_backgroundPicFileLineEdit = new QLineEdit( m_backgroundPicFile, backgroundPicFile_tw );
	m_backgroundPicFileLineEdit->setGeometry( 10, 20, txtLength, 16 );
	connect( m_backgroundPicFileLineEdit, SIGNAL( textChanged( const QString & ) ), this,
			SLOT( setBackgroundPicFile( const QString & ) ) );

	QPushButton * backgroundPicFile_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ), "", backgroundPicFile_tw );
	backgroundPicFile_select_btn->setFixedSize( 24, 24 );
	backgroundPicFile_select_btn->move( btnStart, 16 );
	connect( backgroundPicFile_select_btn, SIGNAL( clicked() ), this,
			SLOT( openBackgroundPicFile() ) );

	pathSelectors->setLayout( pathSelectorsLayout );

	// Path selectors layout ordering.
	pathSelectorsLayout->addWidget( workingDir_tw );
	pathSelectorsLayout->addSpacing( 10 );
	pathSelectorsLayout->addWidget( vstDir_tw );
	pathSelectorsLayout->addSpacing( 10 );
	pathSelectorsLayout->addWidget( ladspaDir_tw );
	pathSelectorsLayout->addSpacing( 10 );
	pathSelectorsLayout->addWidget( sf2Dir_tw );
	pathSelectorsLayout->addSpacing( 10 );
#ifdef LMMS_HAVE_FLUIDSYNTH
	pathSelectorsLayout->addWidget( sf2File_tw );
	pathSelectorsLayout->addSpacing( 10 );
#endif
	pathSelectorsLayout->addWidget( gigDir_tw );
	pathSelectorsLayout->addSpacing( 10 );
	pathSelectorsLayout->addWidget( themeDir_tw );
	pathSelectorsLayout->addSpacing( 10 );
	pathSelectorsLayout->addWidget( backgroundPicFile_tw );
	pathSelectorsLayout->addStretch();

	paths_layout->addWidget( pathSelectors );

	pathsScroll->setWidget( pathSelectors );
	pathsScroll->setWidgetResizable( true );



	// Major tabs ordering.
	m_tabBar->addTab( general_w,
			tr( "General settings" ), 0, false,
					true )->setIcon(
							embed::getIconPixmap( "setup_general" ) );
	m_tabBar->addTab( performance_w,
			tr( "Performance settings" ), 1, false,
					true )->setIcon(
							embed::getIconPixmap( "setup_performance" ) );
	m_tabBar->addTab( audio_w,
			tr( "Audio settings" ), 2, false,
					true )->setIcon(
							embed::getIconPixmap( "setup_audio" ) );
	m_tabBar->addTab( midi_w,
			tr( "MIDI settings" ), 3, false,
					true )->setIcon(
							embed::getIconPixmap( "setup_midi" ) );
	m_tabBar->addTab( paths_w,
			tr( "Paths settings" ), 4, true,
					true )->setIcon(
							embed::getIconPixmap( "setup_directories" ) );


	m_tabBar->setActiveTab( tab_to_open );

	// Horizontal layout ordering.
	hlayout->addWidget( m_tabBar );
	hlayout->addSpacing( 10 );
	hlayout->addWidget( settings_w );
	hlayout->addSpacing( 10 );
	hlayout->addStretch();

	// Extras widget.
	QWidget * extras_w = new QWidget( this );
	QHBoxLayout * extras_layout = new QHBoxLayout( extras_w );
	extras_layout->setSpacing( 0 );
	extras_layout->setMargin( 0 );

	// Restart warning LED.
	LedCheckBox * restartWarning = new LedCheckBox(
			tr( "Display restart warning after changing settings" ), extras_w );
	restartWarning->setChecked( m_warnAfterSetup );
	connect( restartWarning, SIGNAL( toggled( bool ) ), this,
			SLOT( toggleWarnAfterSetup( bool ) ) );

	// OK button.
	QPushButton * ok_btn = new QPushButton(
			embed::getIconPixmap( "apply" ),
			tr( "OK" ), extras_w );
	connect( ok_btn, SIGNAL( clicked() ), this,
			SLOT( accept() ) );

	// Cancel button.
	QPushButton * cancel_btn = new QPushButton(
			embed::getIconPixmap( "cancel" ),
			tr( "Cancel" ), extras_w );
	connect( cancel_btn, SIGNAL( clicked() ), this,
			SLOT( reject() ) );

	// Extras layout ordering.
	extras_layout->addStretch();
	extras_layout->addSpacing( 10 );
	extras_layout->addWidget( restartWarning );
	extras_layout->addSpacing( 10 );
	extras_layout->addWidget( ok_btn );
	extras_layout->addSpacing( 10 );
	extras_layout->addWidget( cancel_btn );
	extras_layout->addSpacing( 10 );

	// Vertical layout ordering.
	vlayout->addWidget( main_w );
	vlayout->addSpacing( 10 );
	vlayout->addWidget( extras_w );
	vlayout->addSpacing( 10 );
	vlayout->addStretch();

	show();
}




SetupDialog::~SetupDialog()
{
	Engine::projectJournal()->setJournalling( true );
}




void SetupDialog::accept()
{	ConfigManager::inst()->setValue( "app", "nomsgaftersetup",
					QString::number( !m_warnAfterSetup ) );
	ConfigManager::inst()->setValue( "mixer", "framesperaudiobuffer",
					QString::number( m_bufferSize ) );
	ConfigManager::inst()->setValue( "mixer", "audiodev",
					m_audioIfaceNames[m_audioInterfaces->currentText()] );
	ConfigManager::inst()->setValue( "mixer", "mididev",
					m_midiIfaceNames[m_midiInterfaces->currentText()] );
	ConfigManager::inst()->setValue( "tooltips", "disabled",
					QString::number( !m_tooltips ) );
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
	ConfigManager::inst()->setLADSPADir(QDir::fromNativeSeparators(m_ladspaDir));
	ConfigManager::inst()->setSF2Dir(QDir::fromNativeSeparators(m_sf2Dir));
#ifdef LMMS_HAVE_FLUIDSYNTH
	ConfigManager::inst()->setSF2File( m_sf2File );
#endif
	ConfigManager::inst()->setGIGDir(QDir::fromNativeSeparators(m_gigDir));
	ConfigManager::inst()->setThemeDir(QDir::fromNativeSeparators(m_themeDir));
	ConfigManager::inst()->setBackgroundPicFile( m_backgroundPicFile );

	// Tell all audio-settings-widgets to save their settings.
	for( AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
		it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		it.value()->saveSettings();
	}
	// Tell all MIDI-settings-widgets to save their settings.
	for( MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
		it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		it.value()->saveSettings();
	}

	ConfigManager::inst()->saveConfigFile();

	QDialog::accept();
	if( m_warnAfterSetup )
	{
		QMessageBox::information( NULL,	
				tr( "Warning" ),
				tr( "Please note that most changes "
					"won't take effect until "
					"you restart LMMS." ), QMessageBox::Ok );
	}
}




// General settings subroutine.

void SetupDialog::toggleWarnAfterSetup( bool enabled )
{
	m_warnAfterSetup = enabled;
}


void SetupDialog::toggleTooltips( bool enabled )
{
	m_tooltips = enabled;
}


void SetupDialog::toggleDisplaydBFS( bool enabled )
{
	m_displaydBFS = enabled;
}


void SetupDialog::toggleDisplayWaveform( bool enabled )
{
	m_displayWaveform = enabled;
}


void SetupDialog::toggleNoteLabels( bool enabled )
{
	m_printNoteLabels = enabled;
}


void SetupDialog::toggleCompactTrackButtons( bool enabled )
{
	m_compactTrackButtons = enabled;
}


void SetupDialog::toggleOneInstrumentTrackWindow( bool enabled )
{
	m_oneInstrumentTrackWindow = enabled;
}


void SetupDialog::toggleMMPZ( bool enabled )
{
	m_MMPZ = enabled;
}


void SetupDialog::toggleDisableBackup( bool enabled )
{
	m_disableBackup = enabled;
}


void SetupDialog::toggleOpenLastProject( bool enabled )
{
	m_openLastProject = enabled;
}


void SetupDialog::setLanguage( int lang )
{
	m_lang = m_languages[lang];
}




void SetupDialog::toggleAutoSave( bool enabled )
{
	m_enableAutoSave = enabled;
	m_saveIntervalSlider->setEnabled( enabled );
	m_runningAutoSave->setVisible( enabled );
	setAutoSaveInterval( m_saveIntervalSlider->value() );
}


void SetupDialog::toggleRunningAutoSave( bool enabled )
{
	m_enableRunningAutoSave = enabled;
}


void SetupDialog::toggleSmoothScroll( bool enabled )
{
	m_smoothScroll = enabled;
}


void SetupDialog::toggleAnimateAFP( bool enabled )
{
	m_animateAFP = enabled;
}


void SetupDialog::toggleSyncVSTPlugins( bool enabled )
{
	m_syncVSTPlugins = enabled;
}


void SetupDialog::toggleDisableAutoQuit( bool enabled )
{
	m_disableAutoQuit = enabled;
}




void SetupDialog::toggleHQAudioDev( bool enabled )
{
	m_hqAudioDev = enabled;
}



void SetupDialog::setAutoSaveInterval( int value )
{
	m_saveInterval = value;
	m_saveIntervalSlider->setValue( m_saveInterval );
	QString minutes = m_saveInterval > 1 ? tr( "minutes" ) : tr( "minute" );
	minutes = QString( "%1 %2" ).arg( QString::number( m_saveInterval ), minutes );
	minutes = m_enableAutoSave ?  minutes : tr( "disabled" );
	m_saveIntervalLbl->setText(
			tr( "Autosave interval: %1" ).arg( minutes ) );
}




void SetupDialog::resetAutoSave()
{
	setAutoSaveInterval( MainWindow::DEFAULT_SAVE_INTERVAL_MINUTES );
	m_autoSave->setChecked( true );
	m_runningAutoSave->setChecked( false );
}




void SetupDialog::displaySaveIntervalHelp()
{
	QWhatsThis::showText( QCursor::pos(),
			tr( "The autosave recovery file is:\n"
				"%1.\n"
				"Remember to also save your project manually. "
				"You can choose to disable saving while playing, "
				"something some older systems find difficult." ).arg( ConfigManager::inst()->recoveryFile() ) );
}




void SetupDialog::audioInterfaceChanged( const QString & iface )
{
	for( AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
		it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		it.value()->hide();
	}

	m_audioIfaceSetupWidgets[m_audioIfaceNames[iface]]->show();
}




void SetupDialog::displayAudioHelp()
{
	QWhatsThis::showText( QCursor::pos(),
			tr( "Here you can select your preferred audio interface. "
				"The audio interfaces available depend on "
				"the configuration of your system during compilation time. "
				"Below is a box which offers controls to "
				"setup the selected audio interface." ) );
}




void SetupDialog::setBufferSize( int value )
{
	const int step = DEFAULT_BUFFER_SIZE / 64;
	if( value > step && value % step )
	{
		int mod_value = value % step;
		if( mod_value < step / 2 )
		{
			m_bufferSizeSlider->setValue( value - mod_value );
		}
		else
		{
			m_bufferSizeSlider->setValue( value + step - mod_value );
		}
		return;
	}

	if( m_bufferSizeSlider->value() != value )
	{
		m_bufferSizeSlider->setValue( value );
	}

	m_bufferSize = value * 64;
	m_bufferSizeLbl->setText(
			tr( "Frames: %1\nLatency: %2 ms" ).arg(
			m_bufferSize ).arg(
			1000.0f * m_bufferSize / Engine::mixer()->processingSampleRate(),
			0, 'f', 1 ) );
}


void SetupDialog::resetBufferSize()
{
	setBufferSize( DEFAULT_BUFFER_SIZE / 64 );
}


void SetupDialog::displayBufferSizeHelp()
{
	QWhatsThis::showText( QCursor::pos(),
			tr( "Here you can setup the internal buffer size "
				"used by LMMS. Smaller values result "
				"in a lower latency but also may cause "
				"unusable sound or bad performance, "
				"especially on older computers or "
				"systems with a non-realtime "
				"kernel." ) );
}




void SetupDialog::midiInterfaceChanged( const QString & iface )
{
	for( MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
		it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		it.value()->hide();
	}

	m_midiIfaceSetupWidgets[m_midiIfaceNames[iface]]->show();
}




void SetupDialog::displayMIDIHelp()
{
	QWhatsThis::showText( QCursor::pos(),
			tr( "Here you can select your preferred MIDI interface. "
				"The MIDI interfaces available depend on "
				"the configuration of your system during compilation time. "
				"Below is a box which offers controls to "
				"setup the selected MIDI interface." ) );
}




void SetupDialog::openWorkingDir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
			tr( "Choose the LMMS working directory" ), m_workingDir );
	if( new_dir != QString::null )
	{
		m_workingDirLineEdit->setText( new_dir );
	}
}


void SetupDialog::setWorkingDir( const QString & workingDir )
{
	m_workingDir = workingDir;
}


void SetupDialog::openVSTDir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
			tr( "Choose your VST plugins directory" ), m_vstDir );
	if( new_dir != QString::null )
	{
		m_vstDirLineEdit->setText( new_dir );
	}
}


void SetupDialog::setVSTDir( const QString & vstDir )
{
	m_vstDir = vstDir;
}


void SetupDialog::openLADSPADir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
			tr( "Choose your LADSPA plugins directory" ), m_ladspaDir );
	if( new_dir != QString::null )
	{
		if( m_ladspaDirLineEdit->text() == "" )
		{
			m_ladspaDirLineEdit->setText( new_dir );
		}
		else
		{
			m_ladspaDirLineEdit->setText( m_ladspaDirLineEdit->text() + "," +
								new_dir );
		}
	}
}


void SetupDialog::setLADSPADir( const QString & ladspaDir )
{
	m_ladspaDir = ladspaDir;
}


void SetupDialog::openSF2Dir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
			tr( "Choose your SF2 files directory" ), m_sf2Dir );
	if( new_dir != QString::null )
	{
		m_sf2DirLineEdit->setText( new_dir );
	}
}


void SetupDialog::setSF2Dir(const QString & sf2Dir)
{
	m_sf2Dir = sf2Dir;
}


void SetupDialog::openSF2File()
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	QString new_file = FileDialog::getOpenFileName( this,
			tr( "Choose your default SF2 file" ), m_sf2File, "SoundFont 2 files (*.sf2)" );
	
	if( new_file != QString::null )
	{
		m_sf2FileLineEdit->setText( new_file );
	}
#endif
}


void SetupDialog::setSF2File( const QString & sf2File )
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_sf2File = sf2File;
#endif
}


void SetupDialog::openGIGDir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
			tr( "Choose your GIG files directory" ), m_gigDir );
	if( new_dir != QString::null )
	{
		m_gigDirLineEdit->setText( new_dir );
	}
}


void SetupDialog::setGIGDir(const QString & gigDir)
{
	m_gigDir = gigDir;
}


void SetupDialog::openThemeDir()
{
	QString new_dir = FileDialog::getExistingDirectory( this,
			tr( "Choose your theme directory" ), m_themeDir );
	if( new_dir != QString::null )
	{
		m_themeDirLineEdit->setText( new_dir );
	}
}


void SetupDialog::setThemeDir( const QString & themeDir )
{
	m_themeDir = themeDir;
}


void SetupDialog::openBackgroundPicFile()
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

	QString dir = ( m_backgroundPicFile.isEmpty() ) ?
		m_themeDir :
		m_backgroundPicFile;
	QString new_file = FileDialog::getOpenFileName( this,
			tr( "Choose your background picture file" ), dir, "Image files (" + fileTypes + ")" );
	
	if( new_file != QString::null )
	{
		m_backgroundPicFileLineEdit->setText( new_file );
	}
}


void SetupDialog::setBackgroundPicFile( const QString & backgroundPicFile )
{
	m_backgroundPicFile = backgroundPicFile;
}
