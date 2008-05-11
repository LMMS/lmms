#ifndef SINGLE_SOURCE_COMPILE

/*
 * setup_dialog.cpp - dialog for setting up LMMS
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QComboBox>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QSlider>
#include <QtGui/QWhatsThis>


#include "setup_dialog.h"
#include "tab_bar.h"
#include "tab_button.h"
#include "tab_widget.h"
#include "gui_templates.h"
#include "mixer.h"
#include "project_journal.h"
#include "config_mgr.h"
#include "embed.h"
#include "engine.h"
#include "debug.h"
#include "tooltip.h"
#include "led_checkbox.h"
#include "lcd_spinbox.h"


// platform-specific audio-interface-classes
#include "audio_alsa.h"
#include "audio_jack.h"
#include "audio_oss.h"
#include "audio_sdl.h"
#include "audio_dummy.h"

// platform-specific midi-interface-classes
#include "midi_alsa_raw.h"
#include "midi_alsa_seq.h"
#include "midi_oss.h"
#include "midi_dummy.h"


inline void labelWidget( QWidget * _w, const QString & _txt )
{
	QLabel * title = new QLabel( _txt, _w );
	QFont f = title->font();
	f.setBold( TRUE );
	title->setFont( pointSize<12>( f ) );

#ifdef LMMS_DEBUG
	assert( dynamic_cast<QBoxLayout *>( _w->layout() ) != NULL );
#endif
	dynamic_cast<QBoxLayout *>( _w->layout() )->addSpacing( 5 );
	dynamic_cast<QBoxLayout *>( _w->layout() )->addWidget( title );
	dynamic_cast<QBoxLayout *>( _w->layout() )->addSpacing( 10 );
}




setupDialog::setupDialog( ConfigTabs _tab_to_open ) :
	m_bufferSize( configManager::inst()->value( "mixer",
					"framesperaudiobuffer" ).toInt() ),
	m_toolTips( !configManager::inst()->value( "tooltips",
							"disabled" ).toInt() ),
	m_classicalKnobUsability( configManager::inst()->value( "knobs",
					"classicalusability" ).toInt() ),
	m_MDI( !configManager::inst()->value( "app",
						"gimplikewindows" ).toInt() ),
	m_wizard( !configManager::inst()->value( "app", "nowizard" ).toInt() ),
	m_warnAfterSetup( !configManager::inst()->value( "app",
						"nomsgaftersetup" ).toInt() ),
	m_displaydBV( configManager::inst()->value( "app", 
		      				"displaydbv" ).toInt() ),
	m_MMPZ( !configManager::inst()->value( "app", "nommpz" ).toInt() ),
	m_hqAudioDev( configManager::inst()->value( "mixer",
							"hqaudio" ).toInt() ),
	m_workingDir( configManager::inst()->workingDir() ),
	m_vstDir( configManager::inst()->vstDir() ),
	m_artworkDir( configManager::inst()->artworkDir() ),
	m_flDir( configManager::inst()->flDir() ),
	m_ladDir( configManager::inst()->ladspaDir() ),
#ifdef HAVE_STK_H
	m_stkDir( configManager::inst()->stkDir() ),
#endif
	m_disableChActInd( configManager::inst()->value( "ui",
				"disablechannelactivityindicators" ).toInt() ),
	m_manualChPiano( configManager::inst()->value( "ui",
					"manualchannelpiano" ).toInt() )
{
	setWindowIcon( embed::getIconPixmap( "setup_general" ) );
	setWindowTitle( tr( "Setup LMMS" ) );
	setModal( TRUE );

	engine::getProjectJournal()->setJournalling( FALSE );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );
	QWidget * settings = new QWidget( this );
	QHBoxLayout * hlayout = new QHBoxLayout( settings );
	hlayout->setSpacing( 0 );
	hlayout->setMargin( 0 );

	m_tabBar = new tabBar( settings, QBoxLayout::TopToBottom );
	m_tabBar->setExclusive( TRUE );
	m_tabBar->setFixedWidth( 72 );

	QWidget * ws = new QWidget( settings );
#ifdef HAVE_STK_H
	ws->setFixedSize( 360, 422 );
#else
	ws->setFixedSize( 360, 366 );
#endif
	QWidget * general = new QWidget( ws );
	general->setFixedSize( 360, 240 );
	QVBoxLayout * gen_layout = new QVBoxLayout( general );
	gen_layout->setSpacing( 0 );
	gen_layout->setMargin( 0 );
	labelWidget( general, tr( "General settings" ) );

	tabWidget * bufsize_tw = new tabWidget( tr( "BUFFER SIZE" ), general );
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
	toolTip::add( bufsize_reset_btn, tr( "Reset to default-value" ) );

	QPushButton * bufsize_help_btn = new QPushButton(
			embed::getIconPixmap( "help" ), "", bufsize_tw );
	bufsize_help_btn->setGeometry( 320, 40, 28, 28 );
	connect( bufsize_help_btn, SIGNAL( clicked() ), this,
						SLOT( displayBufSizeHelp() ) );


	tabWidget * misc_tw = new tabWidget( tr( "MISC" ), general );
	misc_tw->setFixedHeight( 164 );

	ledCheckBox * enable_tooltips = new ledCheckBox(
							tr( "Enable tooltips" ),
								misc_tw );
	enable_tooltips->move( 10, 18 );
	enable_tooltips->setChecked( m_toolTips );
	connect( enable_tooltips, SIGNAL( toggled( bool ) ),
					this, SLOT( toggleToolTips( bool ) ) );


	ledCheckBox * classical_knob_usability = new ledCheckBox(
					tr( "Classical knob usability (move "
						"cursor around knob to change "
						"value)" ), misc_tw );
	classical_knob_usability->move( 10, 36 );
	classical_knob_usability->setChecked( m_classicalKnobUsability );
	connect( classical_knob_usability, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleKnobUsability( bool ) ) );


	ledCheckBox * mdi_windows = new ledCheckBox(
					tr( "Multiple Document Interface" ),
								misc_tw );
	mdi_windows->move( 10, 54 );
	mdi_windows->setChecked( m_MDI );
	connect( mdi_windows, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleMdiWindows( bool ) ) );


	ledCheckBox * wizard = new ledCheckBox(
					tr( "Show wizard after up-/downgrade" ),
								misc_tw );
	wizard->move( 10, 72 );
	wizard->setChecked( m_wizard );
	connect( wizard, SIGNAL( toggled( bool ) ),
					this, SLOT( toggleWizard( bool ) ) );


	ledCheckBox * restart_msg = new ledCheckBox(
			tr( "Show restart warning after changing settings" ),
								misc_tw );
	restart_msg->move( 10, 90 );
	restart_msg->setChecked( m_warnAfterSetup );
	connect( restart_msg, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleWarnAfterSetup( bool ) ) );


	ledCheckBox * dbv = new ledCheckBox( tr( "Display volume as dBV " ),
								misc_tw );
	dbv->move( 10, 108 );
	dbv->setChecked( m_displaydBV );
	connect( dbv, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleDisplaydBV( bool ) ) );


	ledCheckBox * mmpz = new ledCheckBox(
				tr( "Compress project files per default" ),
								misc_tw );
	mmpz->move( 10, 126 );
	mmpz->setChecked( m_MMPZ );
	connect( mmpz, SIGNAL( toggled( bool ) ),
					this, SLOT( toggleMMPZ( bool ) ) );

	ledCheckBox * hqaudio = new ledCheckBox(
				tr( "HQ-mode for output audio-device" ),
								misc_tw );
	hqaudio->move( 10, 144 );
	hqaudio->setChecked( m_hqAudioDev );
	connect( hqaudio, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleHQAudioDev( bool ) ) );



	gen_layout->addWidget( bufsize_tw );
	gen_layout->addSpacing( 10 );
	gen_layout->addWidget( misc_tw );
	gen_layout->addStretch();



	QWidget * directories = new QWidget( ws );
#ifdef HAVE_STK_H
	directories->setFixedSize( 360, 372 );
#else
	directories->setFixedSize( 360, 326 );
#endif
	QVBoxLayout * dir_layout = new QVBoxLayout( directories );
	dir_layout->setSpacing( 0 );
	dir_layout->setMargin( 0 );
	labelWidget( directories, tr( "Directories" ) );

	// working-dir
	tabWidget * lmms_wd_tw = new tabWidget( tr(
					"LMMS working directory" ).toUpper(),
								directories );
	lmms_wd_tw->setFixedHeight( 56 );

	m_wdLineEdit = new QLineEdit( m_workingDir, lmms_wd_tw );
	m_wdLineEdit->setGeometry( 10, 20, 300, 16 );
	connect( m_wdLineEdit, SIGNAL( textChanged( const QString & ) ), this,
				SLOT( setWorkingDir( const QString & ) ) );

	QPushButton * workingdir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
							"", lmms_wd_tw );
	workingdir_select_btn->setFixedSize( 24, 24 );
	workingdir_select_btn->move( 320, 20 );
	connect( workingdir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openWorkingDir() ) );

	// vst-dir
	tabWidget * vst_tw = new tabWidget( tr(
					"VST-plugin directory" ).toUpper(),
								directories );
	vst_tw->setFixedHeight( 56 );

	m_vdLineEdit = new QLineEdit( m_vstDir, vst_tw );
	m_vdLineEdit->setGeometry( 10, 20, 300, 16 );
	connect( m_vdLineEdit, SIGNAL( textChanged( const QString & ) ), this,
					SLOT( setVSTDir( const QString & ) ) );

	QPushButton * vstdir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
								"", vst_tw );
	vstdir_select_btn->setFixedSize( 24, 24 );
	vstdir_select_btn->move( 320, 20 );
	connect( vstdir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openVSTDir() ) );

	// artwork-dir
	tabWidget * artwork_tw = new tabWidget( tr(
					"Artwork directory" ).toUpper(),
								directories );
	artwork_tw->setFixedHeight( 56 );

	m_adLineEdit = new QLineEdit( m_artworkDir, artwork_tw );
	m_adLineEdit->setGeometry( 10, 20, 300, 16 );
	connect( m_adLineEdit, SIGNAL( textChanged( const QString & ) ), this,
				SLOT( setArtworkDir( const QString & ) ) );

	QPushButton * artworkdir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
							"", artwork_tw );
	artworkdir_select_btn->setFixedSize( 24, 24 );
	artworkdir_select_btn->move( 320, 20 );
	connect( artworkdir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openArtworkDir() ) );

	// FL Studio-dir
	tabWidget * fl_tw = new tabWidget( tr(
				"FL Studio installation directory" ).toUpper(),
								directories );
	fl_tw->setFixedHeight( 56 );

	m_fdLineEdit = new QLineEdit( m_flDir, fl_tw );
	m_fdLineEdit->setGeometry( 10, 20, 300, 16 );
	connect( m_fdLineEdit, SIGNAL( textChanged( const QString & ) ), this,
					SLOT( setFLDir( const QString & ) ) );

	QPushButton * fldir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
								"", fl_tw );
	fldir_select_btn->setFixedSize( 24, 24 );
	fldir_select_btn->move( 320, 20 );
	connect( fldir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openFLDir() ) );
	// LADSPA-dir
	tabWidget * lad_tw = new tabWidget( tr(
			"LADSPA plugin directories" ).toUpper(),
							directories );
	lad_tw->setFixedHeight( 56 );

	m_ladLineEdit = new QLineEdit( m_ladDir, lad_tw );
	m_ladLineEdit->setGeometry( 10, 20, 300, 16 );
	connect( m_ladLineEdit, SIGNAL( textChanged( const QString & ) ), this,
		 		SLOT( setLADSPADir( const QString & ) ) );

	QPushButton * laddir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open", 16, 16 ),
								"", lad_tw );
	laddir_select_btn->setFixedSize( 24, 24 );
	laddir_select_btn->move( 320, 20 );
	connect( laddir_select_btn, SIGNAL( clicked() ), this,
				 		SLOT( openLADSPADir() ) );

#ifdef HAVE_STK_H
	// STK-dir
	tabWidget * stk_tw = new tabWidget( tr(
			"STK rawwave directory" ).toUpper(),
							directories );
	stk_tw->setFixedHeight( 56 );

	m_stkLineEdit = new QLineEdit( m_stkDir, stk_tw );
	m_stkLineEdit->setGeometry( 10, 20, 300, 16 );
	connect( m_stkLineEdit, SIGNAL( textChanged( const QString & ) ), this,
		 SLOT( setSTKDir( const QString & ) ) );

	QPushButton * stkdir_select_btn = new QPushButton(
			embed::getIconPixmap( "project_open", 16, 16 ),
								"", stk_tw );
	stkdir_select_btn->setFixedSize( 24, 24 );
	stkdir_select_btn->move( 320, 20 );
	connect( stkdir_select_btn, SIGNAL( clicked() ), this,
		 SLOT( openSTKDir() ) );
#endif

	dir_layout->addWidget( lmms_wd_tw );
	dir_layout->addSpacing( 10 );
	dir_layout->addWidget( vst_tw );
	dir_layout->addSpacing( 10 );
	dir_layout->addWidget( artwork_tw );
	dir_layout->addSpacing( 10 );
	dir_layout->addWidget( fl_tw );
	dir_layout->addSpacing( 10 );
	dir_layout->addWidget( lad_tw );
#ifdef HAVE_STK_H
	dir_layout->addSpacing( 10 );
	dir_layout->addWidget( stk_tw );
#endif	
	dir_layout->addStretch();






	QWidget * performance = new QWidget( ws );
	performance->setFixedSize( 360, 240 );
	QVBoxLayout * perf_layout = new QVBoxLayout( performance );
	perf_layout->setSpacing( 0 );
	perf_layout->setMargin( 0 );
	labelWidget( performance, tr( "Performance settings" ) );

	tabWidget * ui_fx_tw = new tabWidget( tr( "UI effects vs. "
						"performance" ).toUpper(),
								performance );
	ui_fx_tw->setFixedHeight( 70 );

	ledCheckBox * disable_ch_act_ind = new ledCheckBox(
				tr( "Disable channel activity indicators" ),
								ui_fx_tw );
	disable_ch_act_ind->move( 10, 20 );
	disable_ch_act_ind->setChecked( m_disableChActInd );
	connect( disable_ch_act_ind, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleDisableChActInd( bool ) ) );


	ledCheckBox * manual_ch_piano = new ledCheckBox(
			tr( "Only press keys on channel-piano manually" ),
								ui_fx_tw );
	manual_ch_piano->move( 10, 40 );
	manual_ch_piano->setChecked( m_manualChPiano );
	connect( manual_ch_piano, SIGNAL( toggled( bool ) ),
				this, SLOT( toggleManualChPiano( bool ) ) );



	tabWidget * smp_supp_tw = new tabWidget( tr( "SMP support" ).toUpper(),
								performance );
	smp_supp_tw->setFixedHeight( 200 );

	perf_layout->addWidget( ui_fx_tw );
	perf_layout->addSpacing( 15 );
	perf_layout->addWidget( smp_supp_tw );
	perf_layout->addStretch();



	QWidget * audio = new QWidget( ws );
	audio->setFixedSize( 360, 200 );
	QVBoxLayout * audio_layout = new QVBoxLayout( audio );
	audio_layout->setSpacing( 0 );
	audio_layout->setMargin( 0 );
	labelWidget( audio, tr( "Audio settings" ) );

	tabWidget * audioiface_tw = new tabWidget( tr( "AUDIO INTERFACE" ),
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
	//asw_layout->setAutoAdd( TRUE );

#ifdef JACK_SUPPORT
	m_audioIfaceSetupWidgets[audioJACK::name()] =
					new audioJACK::setupWidget( asw );
#endif

#ifdef ALSA_SUPPORT
	m_audioIfaceSetupWidgets[audioALSA::name()] =
					new audioALSA::setupWidget( asw );
#endif

#ifdef SDL_AUDIO_SUPPORT
	m_audioIfaceSetupWidgets[audioSDL::name()] =
					new audioSDL::setupWidget( asw );
#endif

#ifdef OSS_SUPPORT
	m_audioIfaceSetupWidgets[audioOSS::name()] =
					new audioOSS::setupWidget( asw );
#endif
	m_audioIfaceSetupWidgets[audioDummy::name()] =
					new audioDummy::setupWidget( asw );


	for( aswMap::iterator it = m_audioIfaceSetupWidgets.begin();
				it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		m_audioIfaceNames[tr( it.key().toAscii())] = it.key();
	}
	for( trMap::iterator it = m_audioIfaceNames.begin();
				it != m_audioIfaceNames.end(); ++it )
	{
		QWidget * audioWidget = m_audioIfaceSetupWidgets[it.value()];
		audioWidget->hide();
		asw_layout->addWidget( audioWidget );
		m_audioInterfaces->addItem( it.key() );
	}
	m_audioInterfaces->setCurrentIndex( m_audioInterfaces->findText(
			tr( engine::getMixer()->audioDevName().toAscii() ) ) );
	m_audioIfaceSetupWidgets[engine::getMixer()->audioDevName()]->show();

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

	tabWidget * midiiface_tw = new tabWidget( tr( "MIDI INTERFACE" ),
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
	//msw_layout->setAutoAdd( TRUE );

#ifdef ALSA_SUPPORT
	m_midiIfaceSetupWidgets[midiALSASeq::name()] =
					new midiALSASeq::setupWidget( msw );
	m_midiIfaceSetupWidgets[midiALSARaw::name()] =
					new midiALSARaw::setupWidget( msw );
#endif

#ifdef OSS_SUPPORT
	m_midiIfaceSetupWidgets[midiOSS::name()] =
					new midiOSS::setupWidget( msw );
#endif
	m_midiIfaceSetupWidgets[midiDummy::name()] =
					new midiDummy::setupWidget( msw );


	for( mswMap::iterator it = m_midiIfaceSetupWidgets.begin();
				it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		m_midiIfaceNames[tr( it.key().toAscii())] = it.key();
	}
	for( trMap::iterator it = m_midiIfaceNames.begin();
				it != m_midiIfaceNames.end(); ++it )
	{
		QWidget * midiWidget = m_midiIfaceSetupWidgets[it.value()];
		midiWidget->hide();
		msw_layout->addWidget( midiWidget );
		m_midiInterfaces->addItem( it.key() );
	}

	m_midiInterfaces->setCurrentIndex( m_midiInterfaces->findText(
		tr( engine::getMixer()->midiClientName().toAscii() ) ) );
	m_midiIfaceSetupWidgets[engine::getMixer()->midiClientName()]->show();

	connect( m_midiInterfaces, SIGNAL( activated( const QString & ) ),
		this, SLOT( midiInterfaceChanged( const QString & ) ) );


	midi_layout->addWidget( midiiface_tw );
	midi_layout->addSpacing( 20 );
	midi_layout->addWidget( msw );
	midi_layout->addStretch();


	m_tabBar->addTab( general, tr( "General settings" ), 0, FALSE, TRUE 
			)->setIcon( embed::getIconPixmap( "setup_general" ) );
	m_tabBar->addTab( directories, tr( "Directories" ), 1, FALSE, TRUE 
			)->setIcon( embed::getIconPixmap(
							"setup_directories" ) );
	m_tabBar->addTab( performance, tr( "Performance settings" ), 2, FALSE,
				TRUE )->setIcon( embed::getIconPixmap(
							"setup_performance" ) );
	m_tabBar->addTab( audio, tr( "Audio settings" ), 3, FALSE, TRUE
			)->setIcon( embed::getIconPixmap( "setup_audio" ) );
	m_tabBar->addTab( midi, tr( "MIDI settings" ), 4, TRUE, TRUE
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




setupDialog::~setupDialog()
{
	engine::getProjectJournal()->setJournalling( TRUE );
}




void setupDialog::accept( void )
{
	configManager::inst()->setValue( "mixer", "framesperaudiobuffer",
					QString::number( m_bufferSize ) );
	configManager::inst()->setValue( "mixer", "audiodev",
			m_audioIfaceNames[m_audioInterfaces->currentText()] );
	configManager::inst()->setValue( "mixer", "mididev",
			m_midiIfaceNames[m_midiInterfaces->currentText()] );
	configManager::inst()->setValue( "tooltips", "disabled",
					QString::number( !m_toolTips ) );
	configManager::inst()->setValue( "knobs", "classicalusability",
				QString::number( m_classicalKnobUsability ) );
	configManager::inst()->setValue( "app", "gimplikewindows",
						QString::number( !m_MDI ) );
	configManager::inst()->setValue( "app", "nowizard",
						QString::number( !m_wizard ) );
	configManager::inst()->setValue( "app", "nomsgaftersetup",
					QString::number( !m_warnAfterSetup ) );
	configManager::inst()->setValue( "app", "displaydbv",
					QString::number( m_displaydBV ) );
	configManager::inst()->setValue( "app", "nommpz",
						QString::number( !m_MMPZ ) );
	configManager::inst()->setValue( "mixer", "hqaudio",
					QString::number( m_hqAudioDev ) );
	configManager::inst()->setValue( "ui",
					"disablechannelactivityindicators",
					QString::number( m_disableChActInd ) );
	configManager::inst()->setValue( "ui", "manualchannelpiano",
					QString::number( m_manualChPiano ) );

	configManager::inst()->setWorkingDir( m_workingDir );
	configManager::inst()->setVSTDir( m_vstDir );
	configManager::inst()->setArtworkDir( m_artworkDir );
	configManager::inst()->setFLDir( m_flDir );
	configManager::inst()->setLADSPADir( m_ladDir );
#ifdef HAVE_STK_H
	configManager::inst()->setSTKDir( m_stkDir );
#endif	

	// tell all audio-settings-widget to save their settings
	for( aswMap::iterator it = m_audioIfaceSetupWidgets.begin();
				it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		it.value()->saveSettings();
	}
	// tell all MIDI-settings-widget to save their settings
	for( mswMap::iterator it = m_midiIfaceSetupWidgets.begin();
				it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		it.value()->saveSettings();
	}

	configManager::inst()->saveConfigFile();

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




void setupDialog::setBufferSize( int _value )
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
				engine::getMixer()->processingSampleRate(),
						0, 'f', 1 ) );
}




void setupDialog::resetBufSize( void )
{
	setBufferSize( DEFAULT_BUFFER_SIZE / 64 );
}




void setupDialog::displayBufSizeHelp( void )
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




void setupDialog::toggleToolTips( bool _enabled )
{
	m_toolTips = _enabled;
}




void setupDialog::toggleKnobUsability( bool _classical )
{
	m_classicalKnobUsability = _classical;
}




void setupDialog::toggleMdiWindows( bool _enabled )
{
	m_MDI = _enabled;
}




void setupDialog::toggleWizard( bool _enabled )
{
	m_wizard = _enabled;
}




void setupDialog::toggleWarnAfterSetup( bool _enabled )
{
	m_warnAfterSetup = _enabled;
}




void setupDialog::toggleDisplaydBV( bool _enabled )
{
	m_displaydBV = _enabled;
}




void setupDialog::toggleMMPZ( bool _enabled )
{
	m_MMPZ = _enabled;
}




void setupDialog::toggleHQAudioDev( bool _enabled )
{
	m_hqAudioDev = _enabled;
}




void setupDialog::toggleDisableChActInd( bool _disabled )
{
	m_disableChActInd = _disabled;
}




void setupDialog::toggleManualChPiano( bool _enabled )
{
	m_manualChPiano = _enabled;
}





void setupDialog::openWorkingDir( void )
{
	QString new_dir = QFileDialog::getExistingDirectory( this,
					tr( "Choose LMMS working directory" ),
							m_workingDir );
	if( new_dir != QString::null )
	{
		m_wdLineEdit->setText( new_dir );
	}
}




void setupDialog::setWorkingDir( const QString & _wd )
{
	m_workingDir = _wd;
}




void setupDialog::openVSTDir( void )
{
	QString new_dir = QFileDialog::getExistingDirectory( this,
				tr( "Choose your VST-plugin directory" ),
							m_vstDir );
	if( new_dir != QString::null )
	{
		m_vdLineEdit->setText( new_dir );
	}
}




void setupDialog::setVSTDir( const QString & _vd )
{
	m_vstDir = _vd;
}




void setupDialog::openArtworkDir( void )
{
	QString new_dir = QFileDialog::getExistingDirectory( this,
				tr( "Choose artwork-theme directory" ),
							m_artworkDir );
	if( new_dir != QString::null )
	{
		m_adLineEdit->setText( new_dir );
	}
}




void setupDialog::setArtworkDir( const QString & _ad )
{
	m_artworkDir = _ad;
}




void setupDialog::openFLDir( void )
{
	QString new_dir = QFileDialog::getExistingDirectory( this,
				tr( "Choose FL Studio installation directory" ),
							m_flDir );
	if( new_dir != QString::null )
	{
		m_fdLineEdit->setText( new_dir );
	}
}




void setupDialog::openLADSPADir( void )
{
	QString new_dir = QFileDialog::getExistingDirectory( this,
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
			m_ladLineEdit->setText( m_ladLineEdit->text() + ":" +
								new_dir );
		}
	}
}




void setupDialog::openSTKDir( void )
{
#ifdef HAVE_STK_H
	QString new_dir = QFileDialog::getExistingDirectory( this,
				tr( "Choose STK rawwave directory" ),
							m_ladDir );
	if( new_dir != QString::null )
	{
		m_stkLineEdit->setText( new_dir );
	}
#endif
}




void setupDialog::setFLDir( const QString & _fd )
{
	m_flDir = _fd;
}




void setupDialog::setLADSPADir( const QString & _fd )
{
	m_ladDir = _fd;
}




void setupDialog::setSTKDir( const QString & _fd )
{
#ifdef HAVE_STK_H
	m_stkDir = _fd;
#endif
}




void setupDialog::audioInterfaceChanged( const QString & _iface )
{
	for( aswMap::iterator it = m_audioIfaceSetupWidgets.begin();
				it != m_audioIfaceSetupWidgets.end(); ++it )
	{
		it.value()->hide();
	}

	m_audioIfaceSetupWidgets[m_audioIfaceNames[_iface]]->show();
}




void setupDialog::displayAudioHelp( void )
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




void setupDialog::midiInterfaceChanged( const QString & _iface )
{
	for( mswMap::iterator it = m_midiIfaceSetupWidgets.begin();
				it != m_midiIfaceSetupWidgets.end(); ++it )
	{
		it.value()->hide();
	}

	m_midiIfaceSetupWidgets[m_midiIfaceNames[_iface]]->show();
}




void setupDialog::displayMIDIHelp( void )
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




#include "setup_dialog.moc"


#endif
