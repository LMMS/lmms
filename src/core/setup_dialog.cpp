/*
 * setup_dialog.cpp - dialog for setting up LMMS
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QLayout>
#include <QLabel>
#include <QSlider>
#include <QWhatsThis>
#include <QComboBox>
#include <QMessageBox>

#else

#include <qlayout.h>
#include <qlabel.h>
#include <qslider.h>
#include <qwhatsthis.h>
#include <qcombobox.h>
#include <qmessagebox.h>

#endif


#include "setup_dialog.h"
#include "tab_bar.h"
#include "tab_button.h"
#include "tab_widget.h"
#include "gui_templates.h"
#include "mixer.h"
#include "config_mgr.h"
#include "embed.h"
#include "debug.h"
#include "tooltip.h"
#include "led_checkbox.h"


// platform-specific audio-interface-classes
#include "audio_alsa.h"
#include "audio_jack.h"
#include "audio_oss.h"
#include "audio_sdl.h"
#include "audio_dummy.h"

// platform-specific midi-interface-classes
#include "midi_alsa_raw.h"
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



setupDialog::setupDialog( configTabs _tab_to_open ) :
	QDialog(),
	m_bufferSize( mixer::inst()->framesPerAudioBuffer() ),
	m_disableToolTips( configManager::inst()->value( "tooltips",
							"disabled" ).toInt() ),
	m_classicalKnobUsability( configManager::inst()->value( "knobs",
						"classicalusability" ).toInt() )
{
	setWindowIcon( embed::getIconPixmap( "setup_general" ) );
	setWindowTitle( tr( "Setup LMMS" ) );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	QWidget * settings = new QWidget( this );
	QHBoxLayout * hlayout = new QHBoxLayout( settings );

	m_tabBar = new tabBar( settings, QBoxLayout::TopToBottom );
	m_tabBar->setExclusive( TRUE );
	m_tabBar->setFixedWidth( 72 );

	QWidget * ws = new QWidget( settings );
	ws->setFixedSize( 360, 200 );

	QWidget * general = new QWidget( ws );
	general->setFixedSize( 360, 200 );
	QVBoxLayout * gen_layout = new QVBoxLayout( general );
	labelWidget( general, tr( "General settings" ) );

	tabWidget * bufsize_tw = new tabWidget( tr( "BUFFER SIZE" ), general );
	bufsize_tw->setFixedHeight( 80 );

	m_bufSizeSlider = new QSlider( Qt::Horizontal, bufsize_tw );
#ifdef QT4
	m_bufSizeSlider->setRange( 1, 256 );
	m_bufSizeSlider->setTickPosition( QSlider::TicksBelow );
#else
	m_bufSizeSlider->setMinimum( 1 );
	m_bufSizeSlider->setMaximum( 256 );
	m_bufSizeSlider->setLineStep( 4 );
	m_bufSizeSlider->setTickmarks( QSlider::Below );
#endif
	m_bufSizeSlider->setPageStep( 4 );
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
	misc_tw->setFixedHeight( 60 );

	ledCheckBox * disable_tooltips = new ledCheckBox(
					tr( "Disable tooltips (no spurious "
						"interrupts while playing)" ),
								misc_tw );
	disable_tooltips->move( 10, 18 );
	disable_tooltips->setChecked( m_disableToolTips );
	connect( disable_tooltips, SIGNAL( toggled( bool ) ),
			this, SLOT( toggleToolTips( bool ) ) );

	ledCheckBox * classical_knob_usability = new ledCheckBox(
					tr( "Classical knob usability (move "
						"cursor around knob to change "
						"value)" ),
								misc_tw );
	classical_knob_usability->move( 10, 36 );
	classical_knob_usability->setChecked( m_classicalKnobUsability );
	connect( classical_knob_usability, SIGNAL( toggled( bool ) ),
			this, SLOT( toggleKnobUsability( bool ) ) );


	gen_layout->addWidget( bufsize_tw );
	gen_layout->addSpacing( 10 );
	gen_layout->addWidget( misc_tw );
	gen_layout->addStretch();




	QWidget * audio = new QWidget( ws );
	audio->setFixedSize( 360, 200 );
	QVBoxLayout * audio_layout = new QVBoxLayout( audio );
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
#ifndef QT4
	asw->setBackgroundMode( NoBackground );
#endif

	QHBoxLayout * asw_layout = new QHBoxLayout( asw );
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
#ifdef QT4
		it.value()->hide();
		asw_layout->addWidget( it.value() );
#else
		it.data()->hide();
		asw_layout->addWidget( it.data() );
#endif
		m_audioInterfaces->addItem( it.key() );
	}
#ifdef QT4
	m_audioInterfaces->setCurrentIndex( m_audioInterfaces->findText(
					mixer::inst()->audioDevName() ) );
#else
	m_audioInterfaces->setCurrentText( mixer::inst()->audioDevName() );
#endif
	m_audioIfaceSetupWidgets[mixer::inst()->audioDevName()]->show();

	connect( m_audioInterfaces, SIGNAL( activated( const QString & ) ),
		this, SLOT( audioInterfaceChanged( const QString & ) ) );


	audio_layout->addWidget( audioiface_tw );
	audio_layout->addSpacing( 20 );
	audio_layout->addWidget( asw );
	audio_layout->addStretch();




	QWidget * midi = new QWidget( ws );
	QVBoxLayout * midi_layout = new QVBoxLayout( midi );
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
#ifndef QT4
	msw->setBackgroundMode( NoBackground );
#endif

	QHBoxLayout * msw_layout = new QHBoxLayout( msw );
	//msw_layout->setAutoAdd( TRUE );

#ifdef ALSA_SUPPORT
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
#ifdef QT4
		it.value()->hide();
		msw_layout->addWidget( it.value() );
#else
		msw_layout->addWidget( it.data() );
		it.data()->hide();
#endif
		m_midiInterfaces->addItem( it.key() );
	}

#ifdef QT4
	m_midiInterfaces->setCurrentIndex( m_midiInterfaces->findText(
					mixer::inst()->midiDevName() ) );
#else
	m_midiInterfaces->setCurrentText( mixer::inst()->midiDevName() );
#endif
	m_midiIfaceSetupWidgets[mixer::inst()->midiDevName()]->show();

	connect( m_midiInterfaces, SIGNAL( activated( const QString & ) ),
		this, SLOT( midiInterfaceChanged( const QString & ) ) );


	midi_layout->addWidget( midiiface_tw );
	midi_layout->addSpacing( 20 );
	midi_layout->addWidget( msw );
	midi_layout->addStretch();


#ifndef QT
#define setIcon setPixmap
#endif

	m_tabBar->addTab( general, tr( "General settings" ), 0, FALSE, TRUE 
			)->setIcon( embed::getIconPixmap( "setup_general" ) );
	m_tabBar->addTab( audio, tr( "Audio settings" ), 1, FALSE, TRUE
			)->setIcon( embed::getIconPixmap( "setup_audio" ) );
	m_tabBar->addTab( midi, tr( "MIDI settings" ), 2, TRUE, TRUE
			)->setIcon( embed::getIconPixmap( "setup_midi" ) );

#undef setIcon

	m_tabBar->setActiveTab( _tab_to_open );

	hlayout->addWidget( m_tabBar );
	hlayout->addSpacing( 10 );
	hlayout->addWidget( ws );
	hlayout->addSpacing( 10 );
	hlayout->addStretch();

	QWidget * buttons = new QWidget( this );
	QHBoxLayout * btn_layout = new QHBoxLayout( buttons );
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
}




void setupDialog::accept( void )
{
	configManager::inst()->setValue( "mixer", "framesperaudiobuffer",
					QString::number( m_bufferSize ) );
	configManager::inst()->setValue( "mixer", "audiodev",
					m_audioInterfaces->currentText() );
	configManager::inst()->setValue( "mixer", "mididev",
					m_midiInterfaces->currentText() );
	configManager::inst()->setValue( "tooltips", "disabled",
					QString::number( m_disableToolTips ) );
	configManager::inst()->setValue( "knobs", "classicalusability",
				QString::number( m_classicalKnobUsability ) );
	// tell all audio-settings-widget to save their settings
	for( aswMap::iterator it = m_audioIfaceSetupWidgets.begin();
				it != m_audioIfaceSetupWidgets.end(); ++it )
	{
#ifdef QT4
		it.value()->saveSettings();
#else
		it.data()->saveSettings();
#endif
	}
	// tell all MIDI-settings-widget to save their settings
	for( mswMap::iterator it = m_midiIfaceSetupWidgets.begin();
				it != m_midiIfaceSetupWidgets.end(); ++it )
	{
#ifdef QT4
		it.value()->saveSettings();
#else
		it.data()->saveSettings();
#endif
	}

	configManager::inst()->saveConfigFile();

	QDialog::accept();
	QMessageBox::information( NULL, tr( "Restart LMMS" ),
					tr( "Please note that most changes "
						"won't take effect until "
						"you restart LMMS!" ),
					QMessageBox::Ok );
}




void setupDialog::setBufferSize( int _value )
{
	if( m_bufSizeSlider->value() != _value )
	{
		m_bufSizeSlider->setValue( _value );
	}

	m_bufferSize = _value * 64;
	m_bufSizeLbl->setText( tr( "FRAMES: %1\nLATENCY: %2 ms" ).arg(
					m_bufferSize ).arg(
						1000.0f * m_bufferSize /
						mixer::inst()->sampleRate(),
						0, 'f', 1 ) );
}




void setupDialog::resetBufSize( void )
{
	setBufferSize( DEFAULT_BUFFER_SIZE / 64 );
}




void setupDialog::displayBufSizeHelp( void )
{
#ifdef QT4
	QWhatsThis::showText( QCursor::pos(),
#else
	QWhatsThis::display(
#endif
			tr( "Here you can setup the internal buffer-size "
					"used by LMMS. Smaller values result "
					"in a lower latency but also may cause "
					"unusable sound or bad performance, "
					"especially on older computers or "
					"systems with a non-realtime "
					"kernel." ) );
}




void setupDialog::audioInterfaceChanged( const QString & _iface )
{
	for( aswMap::iterator it = m_audioIfaceSetupWidgets.begin();
				it != m_audioIfaceSetupWidgets.end(); ++it )
	{
#ifdef QT4
		it.value()->hide();
#else
		it.data()->hide();
#endif
	}

	m_audioIfaceSetupWidgets[_iface]->show();
}




void setupDialog::displayAudioHelp( void )
{
#ifdef QT4
	QWhatsThis::showText( QCursor::pos(),
#else
	QWhatsThis::display(
#endif
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
#ifdef QT4
		it.value()->hide();
#else
		it.data()->hide();
#endif
	}

	m_midiIfaceSetupWidgets[_iface]->show();
}




void setupDialog::displayMIDIHelp( void )
{
#ifdef QT4
	QWhatsThis::showText( QCursor::pos(),
#else
	QWhatsThis::display(
#endif
				tr( "Here you can select your preferred "
					"MIDI-interface. Depending on the "
					"configuration of your system during "
					"compilation time you can choose "
					"between ALSA, OSS and more. "
					"Below you see a box which offers "
					"controls to setup the selected "
					"MIDI-interface." ) );
}




void setupDialog::toggleToolTips( bool _disabled )
{
	m_disableToolTips = _disabled;
}




void setupDialog::toggleKnobUsability( bool _classical )
{
	m_classicalKnobUsability = _classical;
}




#include "setup_dialog.moc"

