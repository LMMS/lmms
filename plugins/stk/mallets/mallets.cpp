/*
 * mallets.cpp - tuned instruments that one would bang upon
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "mallets.h"

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "BandedWG.h"
#include "ModalBar.h"
#include "TubeBell.h"

#include "engine.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT malletsstk_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Mallets",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Tuneful things to bang on" ),
	"Danny McRae <khjklujn/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


malletsInstrument::malletsInstrument( InstrumentTrack * _instrument_track ):
	Instrument( _instrument_track, &malletsstk_plugin_descriptor ),
	m_hardnessModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Hardness" )),
	m_positionModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Position" )),
	m_vibratoGainModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Vibrato Gain" )),
	m_vibratoFreqModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Vibrato Freq" )),
	m_stickModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Stick Mix" )),
	m_modulatorModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Modulator" )),
	m_crossfadeModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Crossfade" )),
	m_lfoSpeedModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "LFO Speed" )),
	m_lfoDepthModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "LFO Depth" )),
	m_adsrModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "ADSR" )),
	m_pressureModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Pressure" )),
	m_motionModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Motion" )),
	m_velocityModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Speed" )),
	m_strikeModel( false, this, tr( "Bowed" ) ),
	m_presetsModel(this),
	m_spreadModel(0, 0, 255, 1, this, tr( "Spread" )),
	m_filesMissing( !QDir( configManager::inst()->stkDir() ).exists() ||
		!QFileInfo( configManager::inst()->stkDir() + QDir::separator()
						+ "sinewave.raw" ).exists() )
{

	// ModalBar
	m_presetsModel.addItem( tr( "Marimba" ) );
	m_scalers.append( 4.0 );
	m_presetsModel.addItem( tr( "Vibraphone" ) );
	m_scalers.append( 4.0 );
	m_presetsModel.addItem( tr( "Agogo" ) );
	m_scalers.append( 5.0 );
	m_presetsModel.addItem( tr( "Wood1" ) );
	m_scalers.append( 4.0 );
	m_presetsModel.addItem( tr( "Reso" ) );
	m_scalers.append( 2.5 );
	m_presetsModel.addItem( tr( "Wood2" ) );
	m_scalers.append( 5.0 );
	m_presetsModel.addItem( tr( "Beats" ) );
	m_scalers.append( 20.0 );
	m_presetsModel.addItem( tr( "Two Fixed" ) );
	m_scalers.append( 5.0 );
	m_presetsModel.addItem( tr( "Clump" ) );
	m_scalers.append( 4.0 );
	
	// TubeBell
	m_presetsModel.addItem( tr( "Tubular Bells" ) );
	m_scalers.append( 1.8 );
	
	// BandedWG
	m_presetsModel.addItem( tr( "Uniform Bar" ) );
	m_scalers.append( 25.0 );
	m_presetsModel.addItem( tr( "Tuned Bar" ) );
	m_scalers.append( 10.0 );
	m_presetsModel.addItem( tr( "Glass" ) );
	m_scalers.append( 16.0 );
	m_presetsModel.addItem( tr( "Tibetan Bowl" ) );
	m_scalers.append( 7.0 );
}




malletsInstrument::~malletsInstrument()
{
}




void malletsInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// ModalBar
	m_hardnessModel.saveSettings( _doc, _this, "hardness" );
	m_positionModel.saveSettings( _doc, _this, "position" );
	m_vibratoGainModel.saveSettings( _doc, _this, "vib_gain" );
	m_vibratoFreqModel.saveSettings( _doc, _this, "vib_freq" );
	m_stickModel.saveSettings( _doc, _this, "stick_mix" );

	// TubeBell
	m_modulatorModel.saveSettings( _doc, _this, "modulator" );
	m_crossfadeModel.saveSettings( _doc, _this, "crossfade" );
	m_lfoSpeedModel.saveSettings( _doc, _this, "lfo_speed" );
	m_lfoDepthModel.saveSettings( _doc, _this, "lfo_depth" );
	m_adsrModel.saveSettings( _doc, _this, "adsr" );
	
	// BandedWG
	m_pressureModel.saveSettings( _doc, _this, "pressure" );
	m_motionModel.saveSettings( _doc, _this, "motion" );
	m_vibratoModel.saveSettings( _doc, _this, "vibrato" );
	m_velocityModel.saveSettings( _doc, _this, "velocity" );
	m_strikeModel.saveSettings( _doc, _this, "strike" );

	m_presetsModel.saveSettings( _doc, _this, "preset" );
	m_spreadModel.saveSettings( _doc, _this, "spread" );
}




void malletsInstrument::loadSettings( const QDomElement & _this )
{
	// ModalBar
	m_hardnessModel.loadSettings( _this, "hardness" );
	m_positionModel.loadSettings( _this, "position" );
	m_vibratoGainModel.loadSettings( _this, "vib_gain" );
	m_vibratoFreqModel.loadSettings( _this, "vib_freq" );
	m_stickModel.loadSettings( _this, "stick_mix" );

	// TubeBell
	m_modulatorModel.loadSettings( _this, "modulator" );
	m_crossfadeModel.loadSettings( _this, "crossfade" );
	m_lfoSpeedModel.loadSettings( _this, "lfo_speed" );
	m_lfoDepthModel.loadSettings( _this, "lfo_depth" );
	m_adsrModel.loadSettings( _this, "adsr" );
	
	// BandedWG
	m_pressureModel.loadSettings( _this, "pressure" );
	m_motionModel.loadSettings( _this, "motion" );
	m_vibratoModel.loadSettings( _this, "vibrato" );
	m_velocityModel.loadSettings( _this, "velocity" );
	m_strikeModel.loadSettings( _this, "strike" );

	m_presetsModel.loadSettings( _this, "preset" );
	m_spreadModel.loadSettings( _this, "spread" );
}




QString malletsInstrument::nodeName() const
{
	return( malletsstk_plugin_descriptor.name );
}




void malletsInstrument::playNote( NotePlayHandle * _n,
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
		if( p < 9 )
		{
			_n->m_pluginData = new malletsSynth( freq,
						vel,
						m_vibratoGainModel.value(),
						m_hardnessModel.value(),
						m_positionModel.value(),
						m_stickModel.value(),
						m_vibratoFreqModel.value(),
						p,
						(uint8_t) m_spreadModel.value(),
				engine::mixer()->processingSampleRate() );
		}
		else if( p == 9 )
		{
			_n->m_pluginData = new malletsSynth( freq,
						vel,
						p,
						m_lfoDepthModel.value(),
						m_modulatorModel.value(),
						m_crossfadeModel.value(),
						m_lfoSpeedModel.value(),
						m_adsrModel.value(),
						(uint8_t) m_spreadModel.value(),
				engine::mixer()->processingSampleRate() );
		}
		else
		{
			_n->m_pluginData = new malletsSynth( freq,
						vel,
						m_pressureModel.value(),
						m_motionModel.value(),
						m_vibratoModel.value(),
						p - 10,
						m_strikeModel.value() * 128.0,
						m_velocityModel.value(),
						(uint8_t) m_spreadModel.value(),
				engine::mixer()->processingSampleRate() );
		}
		m.unlock();
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	malletsSynth * ps = static_cast<malletsSynth *>( _n->m_pluginData );
	ps->setFrequency( freq );

	sample_t add_scale = 0.0f;
	if( p == 10 )
	{
		add_scale = static_cast<sample_t>( m_strikeModel.value() ) * freq * 2.5f;
	}
	for( fpp_t frame = 0; frame < frames; ++frame )
	{
		_working_buffer[frame][0] = ps->nextSampleLeft() * 
				( m_scalers[m_presetsModel.value()] + add_scale );
		_working_buffer[frame][1] = ps->nextSampleRight() * 
				( m_scalers[m_presetsModel.value()] + add_scale );
	}
	
	instrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );
}




void malletsInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<malletsSynth *>( _n->m_pluginData );
}




PluginView * malletsInstrument::instantiateView( QWidget * _parent )
{
	return( new malletsInstrumentView( this, _parent ) );
}




malletsInstrumentView::malletsInstrumentView( malletsInstrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	m_modalBarWidget = setupModalBarControls( this );
	setWidgetBackground( m_modalBarWidget, "artwork" );
	m_modalBarWidget->show();
	m_modalBarWidget->move( 0,0 );
	
	m_tubeBellWidget = setupTubeBellControls( this );
	setWidgetBackground( m_tubeBellWidget, "artwork" );
	m_tubeBellWidget->hide();
	m_tubeBellWidget->move( 0,0 );
	
	m_bandedWGWidget = setupBandedWGControls( this );
	setWidgetBackground( m_bandedWGWidget, "artwork" );
	m_bandedWGWidget->hide();
	m_bandedWGWidget->move( 0,0 );
	
	m_presetsCombo = new comboBox( this, tr( "Instrument" ) );
	m_presetsCombo->setGeometry( 140, 50, 99, 22 );
	m_presetsCombo->setFont( pointSize<8>( m_presetsCombo->font() ) );
	
	connect( &_instrument->m_presetsModel, SIGNAL( dataChanged() ),
		 this, SLOT( changePreset() ) );
	
	m_spreadKnob = new knob( knobVintage_32, this );
	m_spreadKnob->setLabel( tr( "Spread" ) );
	m_spreadKnob->move( 190, 140 );
	m_spreadKnob->setHintText( tr( "Spread:" ) + " ", "" );

	// try to inform user about missing Stk-installation
	if( _instrument->m_filesMissing && engine::hasGUI() )
	{
		QMessageBox::information( 0, tr( "Missing files" ),
				tr( "Your Stk-installation seems to be "
					"incomplete. Please make sure "
					"the full Stk-package is installed!" ),
				QMessageBox::Ok );
	}
}




malletsInstrumentView::~malletsInstrumentView()
{
}



void malletsInstrumentView::setWidgetBackground( QWidget * _widget, const QString & _pic )
{
	_widget->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( _widget->backgroundRole(),
		PLUGIN_NAME::getIconPixmap( _pic.toAscii().constData() ) );
	_widget->setPalette( pal );
}




QWidget * malletsInstrumentView::setupModalBarControls( QWidget * _parent )
{
	QWidget * widget = new QWidget( _parent );
	widget->setFixedSize( 250, 250 );
		
	m_hardnessKnob = new knob( knobVintage_32, widget );
	m_hardnessKnob->setLabel( tr( "Hardness" ) );
	m_hardnessKnob->move( 30, 90 );
	m_hardnessKnob->setHintText( tr( "Hardness:" ) + " ", "" );

	m_positionKnob = new knob( knobVintage_32, widget );
	m_positionKnob->setLabel( tr( "Position" ) );
	m_positionKnob->move( 110, 90 );
	m_positionKnob->setHintText( tr( "Position:" ) + " ", "" );

	m_vibratoGainKnob = new knob( knobVintage_32, widget );
	m_vibratoGainKnob->setLabel( tr( "Vib Gain" ) );
	m_vibratoGainKnob->move( 30, 140 );
	m_vibratoGainKnob->setHintText( tr( "Vib Gain:" ) + " ", "" );

	m_vibratoFreqKnob = new knob( knobVintage_32, widget );
	m_vibratoFreqKnob->setLabel( tr( "Vib Freq" ) );
	m_vibratoFreqKnob->move( 110, 140 );
	m_vibratoFreqKnob->setHintText( tr( "Vib Freq:" ) + " ", "" );

	m_stickKnob = new knob( knobVintage_32, widget );
	m_stickKnob->setLabel( tr( "Stick Mix" ) );
	m_stickKnob->move( 190, 90 );
	m_stickKnob->setHintText( tr( "Stick Mix:" ) + " ", "" );
	
	return( widget );
}




QWidget * malletsInstrumentView::setupTubeBellControls( QWidget * _parent )
{
	QWidget * widget = new QWidget( _parent );
	widget->setFixedSize( 250, 250 );
	
	m_modulatorKnob = new knob( knobVintage_32, widget );
	m_modulatorKnob->setLabel( tr( "Modulator" ) );
	m_modulatorKnob->move( 30, 90 );
	m_modulatorKnob->setHintText( tr( "Modulator:" ) + " ", "" );

	m_crossfadeKnob = new knob( knobVintage_32, widget );
	m_crossfadeKnob->setLabel( tr( "Crossfade" ) );
	m_crossfadeKnob->move( 110, 90 );
	m_crossfadeKnob->setHintText( tr( "Crossfade:" ) + " ", "" );
	
	m_lfoSpeedKnob = new knob( knobVintage_32, widget );
	m_lfoSpeedKnob->setLabel( tr( "LFO Speed" ) );
	m_lfoSpeedKnob->move( 30, 140 );
	m_lfoSpeedKnob->setHintText( tr( "LFO Speed:" ) + " ", "" );
	
	m_lfoDepthKnob = new knob( knobVintage_32, widget );
	m_lfoDepthKnob->setLabel( tr( "LFO Depth" ) );
	m_lfoDepthKnob->move( 110, 140 );
	m_lfoDepthKnob->setHintText( tr( "LFO Depth:" ) + " ", "" );
	
	m_adsrKnob = new knob( knobVintage_32, widget );
	m_adsrKnob->setLabel( tr( "ADSR" ) );
	m_adsrKnob->move( 190, 90 );
	m_adsrKnob->setHintText( tr( "ADSR:" ) + " ", "" );

	return( widget );
}




QWidget * malletsInstrumentView::setupBandedWGControls( QWidget * _parent )
{
	// BandedWG
	QWidget * widget = new QWidget( _parent );
	widget->setFixedSize( 250, 250 );
	
	m_strikeLED = new ledCheckBox( tr( "Bowed" ), widget );
	m_strikeLED->move( 138, 25 );

	m_pressureKnob = new knob( knobVintage_32, widget );
	m_pressureKnob->setLabel( tr( "Pressure" ) );
	m_pressureKnob->move( 30, 90 );
	m_pressureKnob->setHintText( tr( "Pressure:" ) + " ", "" );

	m_motionKnob = new knob( knobVintage_32, widget );
	m_motionKnob->setLabel( tr( "Motion" ) );
	m_motionKnob->move( 110, 90 );
	m_motionKnob->setHintText( tr( "Motion:" ) + " ", "" );
	
	m_velocityKnob = new knob( knobVintage_32, widget );
	m_velocityKnob->setLabel( tr( "Speed" ) );
	m_velocityKnob->move( 30, 140 );
	m_velocityKnob->setHintText( tr( "Speed:" ) + " ", "" );
	
	m_vibratoKnob = new knob( knobVintage_32, widget, tr( "Vibrato" ) );
	m_vibratoKnob->setLabel( tr( "Vibrato" ) );
	m_vibratoKnob->move( 110, 140 );
	m_vibratoKnob->setHintText( tr( "Vibrato:" ) + " ", "" );
	
	return( widget );
}




void malletsInstrumentView::modelChanged()
{
	malletsInstrument * inst = castModel<malletsInstrument>();
	m_hardnessKnob->setModel( &inst->m_hardnessModel );
	m_positionKnob->setModel( &inst->m_positionModel );
	m_vibratoGainKnob->setModel( &inst->m_vibratoGainModel );
	m_vibratoFreqKnob->setModel( &inst->m_vibratoFreqModel );
	m_stickKnob->setModel( &inst->m_stickModel );
	m_modulatorKnob->setModel( &inst->m_modulatorModel );
	m_crossfadeKnob->setModel( &inst->m_crossfadeModel );
	m_lfoSpeedKnob->setModel( &inst->m_lfoSpeedModel );
	m_lfoDepthKnob->setModel( &inst->m_lfoDepthModel );
	m_adsrKnob->setModel( &inst->m_adsrModel );
	m_pressureKnob->setModel( &inst->m_pressureModel );
	m_motionKnob->setModel( &inst->m_motionModel );
	m_vibratoKnob->setModel( &inst->m_vibratoModel );
	m_velocityKnob->setModel( &inst->m_velocityModel );
	m_strikeLED->setModel( &inst->m_strikeModel );
	m_presetsCombo->setModel( &inst->m_presetsModel );
	m_spreadKnob->setModel( &inst->m_spreadModel );
}




void malletsInstrumentView::changePreset()
{
	malletsInstrument * inst = castModel<malletsInstrument>();
	int _preset = inst->m_presetsModel.value();
	
	printf("malletsInstrumentView %d\n", _preset);
	if( _preset < 9 )
	{
		m_tubeBellWidget->hide();
		m_bandedWGWidget->hide();
		m_modalBarWidget->show();
	}
	else if( _preset == 9 )
	{
		m_modalBarWidget->hide();
		m_bandedWGWidget->hide();
		m_tubeBellWidget->show();
	}
	else
	{
		m_modalBarWidget->hide();
		m_tubeBellWidget->hide();
		m_bandedWGWidget->show();
	}		
}



// ModalBar
malletsSynth::malletsSynth( const StkFloat _pitch,
				const StkFloat _velocity,
				const StkFloat _control1,
				const StkFloat _control2,
				const StkFloat _control4,
				const StkFloat _control8,
				const StkFloat _control11,
				const int _control16,
				const uint8_t _delay,
				const sample_rate_t _sample_rate )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( configManager::inst()->stkDir()
						.toAscii().constData() );
	
		m_voice = new ModalBar();
	
		m_voice->controlChange( 1, _control1 );
		m_voice->controlChange( 2, _control2 );
		m_voice->controlChange( 4, _control4 );
		m_voice->controlChange( 8, _control8 );
		m_voice->controlChange( 11, _control11 );
		m_voice->controlChange( 16, _control16 );
		m_voice->controlChange( 128, 128.0f );
		
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




// TubeBell
malletsSynth::malletsSynth( const StkFloat _pitch,
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
		Stk::setRawwavePath( configManager::inst()->stkDir()
						.toAscii().constData() );
	
		m_voice = new TubeBell();
	
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




// BandedWG
malletsSynth::malletsSynth( const StkFloat _pitch,
				const StkFloat _velocity,
				const StkFloat _control2,
				const StkFloat _control4,
				const StkFloat _control11,
				const int _control16,
				const StkFloat _control64,
				const StkFloat _control128,
				const uint8_t _delay,
				const sample_rate_t _sample_rate )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( configManager::inst()->stkDir()
						.toAscii().constData() );

		m_voice = new BandedWG();
	
		m_voice->controlChange( 1, 128.0 );
		m_voice->controlChange( 2, _control2 );
		m_voice->controlChange( 4, _control4 );
		m_voice->controlChange( 11, _control11 );
		m_voice->controlChange( 16, _control16 );
		m_voice->controlChange( 64, _control64 );
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
	return new malletsInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}


#include "moc_mallets.cxx"


