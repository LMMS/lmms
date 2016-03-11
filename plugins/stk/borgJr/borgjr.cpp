/*
 * borgjr.h - FM synthesis
 *
 * Copyright (c) 2016 Oskar Wallgren <oskar.wallgren13/at/gmail.com>
 *
 * Heavily based on 'Mallets' by Danny McRae and Tobias Doerffel.
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009-2015 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "borgjr.h"

#include <QDir>
#include <QMessageBox>

#include <BeeThree.h>
#include <FMVoices.h>
#include <HevyMetl.h>
#include <PercFlut.h>
#include <Rhodey.h>
#include <TubeBell.h>
#include <Wurley.h>

#include "Engine.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT borgjrstk_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Borg Jr",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Simple FM Synthesis" ),
	"Oskar Wallgren <oskar.wallgren13/at/gmail.com>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


borgjrInstrument::borgjrInstrument( InstrumentTrack * _instrument_track ):
	Instrument( _instrument_track, &borgjrstk_plugin_descriptor ),
	m_modulatorModel(0.0f, 0.0f, 128.0f, 0.1f, this, tr( "Modulator 1" )),
	m_crossfadeModel(0.0f, 0.0f, 127.9f, 0.1f, this, tr( "Modulator 2" )),
	m_lfoSpeedModel(0.0f, 0.0f, 128.0f, 0.1f, this, tr( "LFO Speed" )),
	m_lfoDepthModel(0.0f, 0.0f, 128.0f, 0.1f, this, tr( "LFO Depth" )),
	m_adsrModel(0.1f, 0.1f, 128.0f, 0.1f, this, tr( "ADSR" )),
	m_presetsModel(this),
	m_spreadModel(0, 0, 255, 1, this, tr( "Spread" )),
	m_filesMissing( !QDir( ConfigManager::inst()->stkDir() ).exists() ||
		!QFileInfo( ConfigManager::inst()->stkDir() + "/sinewave.raw" ).exists() )
{
	// BeeThree
	m_presetsModel.addItem( tr( "BeeThree" ) );
	m_scalers.append( 1.8 );
	
	// FMVoices
	m_presetsModel.addItem( tr( "FMVoices" ) );
	m_scalers.append( 1.8 );

	// HevyMetl
	m_presetsModel.addItem( tr( "Metal" ) );
	m_scalers.append( 1.8 );

	// PercFlut
	m_presetsModel.addItem( tr( "PercFlut" ) );
	m_scalers.append( 1.8 );

	// Rhodey
	m_presetsModel.addItem( tr( "Rhodey" ) );
	m_scalers.append( 1.8 );

	// TubeBell
	m_presetsModel.addItem( tr( "Tubular Bells" ) );
	m_scalers.append( 1.8 );

	// Wurley
	m_presetsModel.addItem( tr( "Wurley" ) );
	m_scalers.append( 1.8 );
}



borgjrInstrument::~borgjrInstrument()
{
}




void borgjrInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// TubeBell, Metal
	m_modulatorModel.saveSettings( _doc, _this, "modulator" );
	m_crossfadeModel.saveSettings( _doc, _this, "crossfade" );
	m_lfoSpeedModel.saveSettings( _doc, _this, "lfo_speed" );
	m_lfoDepthModel.saveSettings( _doc, _this, "lfo_depth" );
	m_adsrModel.saveSettings( _doc, _this, "adsr" );

	m_presetsModel.saveSettings( _doc, _this, "preset" );
	m_spreadModel.saveSettings( _doc, _this, "spread" );
}




void borgjrInstrument::loadSettings( const QDomElement & _this )
{
	// TubeBell, Metal
	m_modulatorModel.loadSettings( _this, "modulator" );
	m_crossfadeModel.loadSettings( _this, "crossfade" );
	m_lfoSpeedModel.loadSettings( _this, "lfo_speed" );
	m_lfoDepthModel.loadSettings( _this, "lfo_depth" );
	m_adsrModel.loadSettings( _this, "adsr" );

	m_presetsModel.loadSettings( _this, "preset" );
	m_spreadModel.loadSettings( _this, "spread" );
}




QString borgjrInstrument::nodeName() const
{
	return( borgjrstk_plugin_descriptor.name );
}




void borgjrInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if( m_filesMissing )
	{
		return;
	}

	int p = m_presetsModel.value();
	
	const float freq = _n->frequency();
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		const float vel = _n->getVolume() / 100.0f;

		// critical section as STK is not thread-safe
		static QMutex m;
		m.lock();

		_n->m_pluginData = new borgjrSynth( freq,
					vel,
					p,
					m_lfoDepthModel.value(),
					m_modulatorModel.value(),
					m_crossfadeModel.value(),
					m_lfoSpeedModel.value(),
					m_adsrModel.value(),
					(uint8_t) m_spreadModel.value(),
				Engine::mixer()->processingSampleRate() );

		m.unlock();
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	borgjrSynth * ps = static_cast<borgjrSynth *>( _n->m_pluginData );
	ps->setFrequency( freq );

	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		_working_buffer[frame][0] = ps->nextSampleLeft() *
					( m_scalers[m_presetsModel.value()] );
		_working_buffer[frame][1] = ps->nextSampleRight() *
					( m_scalers[m_presetsModel.value()] );
	}

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}




void borgjrInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<borgjrSynth *>( _n->m_pluginData );
}




PluginView * borgjrInstrument::instantiateView( QWidget * _parent )
{
	return( new borgjrInstrumentView( this, _parent ) );
}




borgjrInstrumentView::borgjrInstrumentView( borgjrInstrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	m_FMSynthWidget = setupFMSynthControls( this );
	setWidgetBackground( m_FMSynthWidget, "artwork" );
	m_FMSynthWidget->show();
	m_FMSynthWidget->move( 0,0 );

	m_presetsCombo = new ComboBox( this, tr( "Instrument" ) );
	m_presetsCombo->setGeometry( 150, 100, 90, 22 );
	m_presetsCombo->setFont( pointSize<8>( m_presetsCombo->font() ) );

	connect( &_instrument->m_presetsModel, SIGNAL( dataChanged() ),
		 this, SLOT( changePreset() ) );

	m_modulatorKnob = new Knob( knobVintage_32, this );
	m_modulatorKnob->setLabel( tr( "Modulator 1" ) );
	m_modulatorKnob->move( 30, 136 );
	m_modulatorKnob->setHintText( tr( "Modulator 1:" ), "" );

	m_crossfadeKnob = new Knob( knobVintage_32, this );
	m_crossfadeKnob->setLabel( tr( "Modulator 2" ) );
	m_crossfadeKnob->move( 110, 136 );
	m_crossfadeKnob->setHintText( tr( "Modulator 2:" ), "" );
	
	m_lfoSpeedKnob = new Knob( knobVintage_32, this );
	m_lfoSpeedKnob->setLabel( tr( "LFO Speed" ) );
	m_lfoSpeedKnob->move( 30, 186 );
	m_lfoSpeedKnob->setHintText( tr( "LFO Speed:" ), "" );
	
	m_lfoDepthKnob = new Knob( knobVintage_32, this );
	m_lfoDepthKnob->setLabel( tr( "LFO Depth" ) );
	m_lfoDepthKnob->move( 110, 186 );
	m_lfoDepthKnob->setHintText( tr( "LFO Depth:" ), "" );
	
	m_adsrKnob = new Knob( knobVintage_32, this );
	m_adsrKnob->setLabel( tr( "ADSR" ) );
	m_adsrKnob->move( 190, 136 );
	m_adsrKnob->setHintText( tr( "ADSR:" ), "" );

	m_spreadKnob = new Knob( knobVintage_32, this );
	m_spreadKnob->setLabel( tr( "Spread" ) );
	m_spreadKnob->move( 190, 186 );
	m_spreadKnob->setHintText( tr( "Spread:" ), "" );

	// try to inform user about missing Stk-installation
	if( _instrument->m_filesMissing && gui != NULL )
	{
		QMessageBox::information( 0, tr( "Missing files" ),
				tr( "Your Stk-installation seems to be "
					"incomplete. Please make sure "
					"the full Stk-package is installed!" ),
				QMessageBox::Ok );
	}
}




borgjrInstrumentView::~borgjrInstrumentView()
{
}



void borgjrInstrumentView::setWidgetBackground( QWidget * _widget, const QString & _pic )
{
	_widget->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( _widget->backgroundRole(),
		PLUGIN_NAME::getIconPixmap( _pic.toLatin1().constData() ) );
	_widget->setPalette( pal );
}




QWidget * borgjrInstrumentView::setupFMSynthControls( QWidget * _parent )
{
	QWidget * widget = new QWidget( _parent );
	widget->setFixedSize( 250, 250 );

	return( widget );
}




void borgjrInstrumentView::modelChanged()
{
	borgjrInstrument * inst = castModel<borgjrInstrument>();
	m_modulatorKnob->setModel( &inst->m_modulatorModel );
	m_crossfadeKnob->setModel( &inst->m_crossfadeModel );
	m_lfoSpeedKnob->setModel( &inst->m_lfoSpeedModel );
	m_lfoDepthKnob->setModel( &inst->m_lfoDepthModel );
	m_adsrKnob->setModel( &inst->m_adsrModel );
	m_presetsCombo->setModel( &inst->m_presetsModel );
	m_spreadKnob->setModel( &inst->m_spreadModel );
}




void borgjrInstrumentView::changePreset()
{
	m_FMSynthWidget->show();
}



// TubeBell, Metal
borgjrSynth::borgjrSynth( const StkFloat _pitch,
				const StkFloat _velocity,
				const int _preset,
				const StkFloat _control1,
				const StkFloat _control2,
				const StkFloat _control4,
				const StkFloat _control11,
				const StkFloat _control128,
				const uint8_t _delay,
				const sample_rate_t _sample_rate )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( QDir( ConfigManager::inst()->stkDir() ).absolutePath()
						.toLatin1().constData() );
	
		switch( _preset )
		{
			case 0:
				m_voice = new BeeThree();
				break;
			case 1:
				m_voice = new FMVoices();
				break;
			case 2:
				m_voice = new HevyMetl();
				break;
			case 3:
				m_voice = new PercFlut();
				break;
			case 4:
				m_voice = new Rhodey();
				break;
			case 5:
				m_voice = new TubeBell();
				break;
			case 6:
				m_voice = new Wurley();
				break;
		}
	
		m_voice->controlChange( 1, _control1 );
		m_voice->controlChange( 2, _control2 );
		m_voice->controlChange( 4, _control4 );
		m_voice->controlChange( 11, _control11 );
		m_voice->controlChange( 128, _control128 );
	
		m_voice->noteOn( _pitch, _velocity );
	}
	catch( ... )
	{
		m_voice = NULL;
	}

	m_delay = new StkFloat[256];
	m_delayRead = 0;
	m_delayWrite = _delay;
	for( int i = 0; i < 256; i++ )
	{
		m_delay[i] = 0.0;
	}
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new borgjrInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}





