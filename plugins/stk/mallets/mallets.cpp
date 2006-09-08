/*
 * mallets.h - tuned instruments that one would bang upon
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>

#else

#include <qdom.h>

#endif

#include "BandedWG.h"
#include "ModalBar.h"
#include "TubeBell.h"

#include "mallets.h"
#include "instrument_track.h"
#include "note_play_handle.h"
#include "templates.h"
#include "knob.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor malletsstk_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Mallets",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Tuneful things to bang on" ),
	"Danny McRae <khjklujn/at/users.sf.net>",
	0x0100,
	plugin::INSTRUMENT,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) )
} ;

}


mallets::mallets( instrumentTrack * _channel_track ) :
	instrument( _channel_track, &malletsstk_plugin_descriptor )
{
	m_modalBarWidget = setupModalBarControls( this, _channel_track );
	setWidgetBackground( m_modalBarWidget, "artwork" );
	
	m_tubeBellWidget = setupTubeBellControls( this, _channel_track );
	setWidgetBackground( m_tubeBellWidget, "artwork" );
	m_tubeBellWidget->hide();
	
	m_bandedWGWidget = setupBandedWGControls( this, _channel_track );
	setWidgetBackground( m_bandedWGWidget, "artwork" );
	m_bandedWGWidget->hide();
	
	m_presets = setupPresets( this, _channel_track );
	
	m_spread = new knob( knobBright_26, this, tr( "Spread" ),
					eng(), _channel_track );
	m_spread->setLabel( tr( "Spread" ) );
	m_spread->setRange( 0, 255, 1 );
	m_spread->setInitValue( 0 );
	m_spread->move( 178, 173 );
	m_spread->setHintText( tr( "Spread:" ) + " ", "" );

	m_buffer = bufferAllocator::alloc<sampleFrame>(
			eng()->getMixer()->framesPerAudioBuffer() );
}




mallets::~mallets()
{
	bufferAllocator::free( m_buffer );
}




void mallets::setWidgetBackground( QWidget * _widget, const QString & _pic )
{
#ifdef QT4
	_widget->setAutoFillBackground( TRUE );
	_widget->QPalette pal;
	_widget->pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								_pic ) );
	_widget->setPalette( pal );
#else
	_widget->setErasePixmap( PLUGIN_NAME::getIconPixmap( _pic ) );
#endif
}




QWidget * mallets::setupModalBarControls( QWidget * _parent, track * _track )
{
	QWidget * widget = new QWidget( _parent, "ModalBar" );
	widget->setFixedSize( 250, 250 );
		
	m_hardness = new knob( knobBright_26, widget, tr( "Hardness" ),
					eng(), _track );
	m_hardness->setLabel( tr( "Hardness" ) );
	m_hardness->setRange( 0.0, 128.0, 0.1 );
	m_hardness->setInitValue( 64.0 );
	m_hardness->move( 145, 24 );
	m_hardness->setHintText( tr( "Hardness:" ) + " ", "" );

	m_position = new knob( knobBright_26, widget, tr( "Position" ),
					eng(), _track );
	m_position->setLabel( tr( "Position" ) );
	m_position->setRange( 0.0, 128.0, 0.1 );
	m_position->setInitValue( 64.0 );
	m_position->move( 195, 24 );
	m_position->setHintText( tr( "Position:" ) + " ", "" );

	m_vibratoGain = new knob( knobBright_26, widget, tr( "Vibrato Gain" ),
					eng(), _track );
	m_vibratoGain->setLabel( tr( "Vib Gain" ) );
	m_vibratoGain->setRange( 0.0, 128.0, 0.1 );
	m_vibratoGain->setInitValue( 64.0 );
	m_vibratoGain->move( 56, 86 );
	m_vibratoGain->setHintText( tr( "Vib Gain:" ) + " ", "" );

	m_vibratoFreq = new knob( knobBright_26, widget, tr( "Vibrato Freq" ),
					eng(), _track );
	m_vibratoFreq->setLabel( tr( "Vib Freq" ) );
	m_vibratoFreq->setRange( 0.0, 128.0, 0.1 );
	m_vibratoFreq->setInitValue( 64.0 );
	m_vibratoFreq->move( 117, 86 );
	m_vibratoFreq->setHintText( tr( "Vib Freq:" ) + " ", "" );

	m_stick = new knob( knobBright_26, widget, tr( "Stick Mix" ),
					eng(), _track );
	m_stick->setLabel( tr( "Stick Mix" ) );
	m_stick->setRange( 0.0, 128.0, 0.1 );
	m_stick->setInitValue( 64.0 );
	m_stick->move( 178, 86 );
	m_stick->setHintText( tr( "Stick Mix:" ) + " ", "" );
	
	return( widget );
}




QWidget * mallets::setupTubeBellControls( QWidget * _parent, track * _track )
{
	QWidget * widget = new QWidget( _parent, "TubeBellWidget" );
	widget->setFixedSize( 250, 250 );
	
	m_modulator = new knob( knobBright_26, widget, tr( "Modulator" ),
					eng(), _track );
	m_modulator->setLabel( tr( "Modulator" ) );
	m_modulator->setRange( 0.0, 128.0, 0.1 );
	m_modulator->setInitValue( 100.0 );
	m_modulator->move( 145, 24 );
	m_modulator->setHintText( tr( "Modulator:" ) + " ", "" );

	m_crossfade = new knob( knobBright_26, widget, tr( "Crossfade" ),
					eng(), _track );
	m_crossfade->setLabel( tr( "Crossfade" ) );
	m_crossfade->setRange( 0.0, 128.0, 0.1 );
	m_crossfade->setInitValue( 0.0 );
	m_crossfade->move( 195, 24 );
	m_crossfade->setHintText( tr( "Crossfade:" ) + " ", "" );
	
	m_lfoSpeed = new knob( knobBright_26, widget, tr( "LFO Speed" ),
					eng(), _track );
	m_lfoSpeed->setLabel( tr( "LFO Speed" ) );
	m_lfoSpeed->setRange( 0.0, 128.0, 0.1 );
	m_lfoSpeed->setInitValue( 20.0 );
	m_lfoSpeed->move( 56, 86 );
	m_lfoSpeed->setHintText( tr( "LFO Speed:" ) + " ", "" );
	
	m_lfoDepth = new knob( knobBright_26, widget, tr( "LFO Depth" ),
					eng(), _track );
	m_lfoDepth->setLabel( tr( "LFO Depth" ) );
	m_lfoDepth->setRange( 0.0, 128.0, 0.1 );
	m_lfoDepth->setInitValue( 10.0 );
	m_lfoDepth->move( 117, 86 );
	m_lfoDepth->setHintText( tr( "LFO Depth:" ) + " ", "" );
	
	m_adsr = new knob( knobBright_26, widget, tr( "ADSR" ),
					eng(), _track );
	m_adsr->setLabel( tr( "ADSR" ) );
	m_adsr->setRange( 0.0, 128.0, 0.1 );
	m_adsr->setInitValue( 0.0 );
	m_adsr->move( 178, 86 );
	m_adsr->setHintText( tr( "ADSR:" ) + " ", "" );

	return( widget );
}




QWidget * mallets::setupBandedWGControls( QWidget * _parent, track * _track )
{
	// BandedWG
	QWidget * widget = new QWidget( _parent, "BandedWGWidget" );
	widget->setFixedSize( 250, 250 );
	
	m_strike = new ledCheckBox( tr( "Bowed" ), widget, tr( "Bowed" ),
					eng(), _track );
	m_strike->move( 165, 30 );
	
	m_pressure = new knob( knobBright_26, widget, tr( "Pressure" ),
					eng(), _track );
	m_pressure->setLabel( tr( "Pressure" ) );
	m_pressure->setRange( 0.0, 128.0, 0.1 );
	m_pressure->setInitValue( 64.0 );
	m_pressure->move( 56, 86 );
	m_pressure->setHintText( tr( "Pressure:" ) + " ", "" );

	m_motion = new knob( knobBright_26, widget, tr( "Motion" ),
					eng(), _track );
	m_motion->setLabel( tr( "Motion" ) );
	m_motion->setRange( 0.0, 128.0, 0.1 );
	m_motion->setInitValue( 64.0 );
	m_motion->move( 117, 86 );
	m_motion->setHintText( tr( "Motion:" ) + " ", "" );
	
	m_velocity = new knob( knobBright_26, widget, tr( "Speed" ),
					eng(), _track );
	m_velocity->setLabel( tr( "Speed" ) );
	m_velocity->setRange( 0.0, 128.0, 0.1 );
	m_velocity->setInitValue( 74.5 );
	m_velocity->move( 178, 86 );
	m_velocity->setHintText( tr( "Speed:" ) + " ", "" );
	
	m_vibrato = new knob( knobBright_26, widget, tr( "Vibrato" ),
					eng(), _track );
	m_vibrato->setLabel( tr( "Vibrato" ) );
	m_vibrato->setRange( 0.0, 128.0, 0.1 );
	m_vibrato->setInitValue( 64.0 );
	m_vibrato->move( 178, 129 );
	m_vibrato->setHintText( tr( "Vibrato:" ) + " ", "" );
	
	return( widget );
}




comboBox * mallets::setupPresets( QWidget * _parent, track * _track )
{
	comboBox * presets = new comboBox( _parent, tr( "Instrument" ),
					eng(), _track );
	presets->setGeometry( 64, 157, 99, 22 );
	presets->setFont( pointSize<8>( presets->font() ) );
	
	connect( presets, SIGNAL( valueChanged( int ) ),
			_parent, SLOT( changePreset( int ) ) );
	
	// ModalBar
	presets->addItem( tr( "Marimba" ) );
	m_scalers.append( 4.0 );
	presets->addItem( tr( "Vibraphone" ) );
	m_scalers.append( 4.0 );
	presets->addItem( tr( "Agogo" ) );
	m_scalers.append( 5.0 );
	presets->addItem( tr( "Wood1" ) );
	m_scalers.append( 4.0 );
	presets->addItem( tr( "Reso" ) );
	m_scalers.append( 2.5 );
	presets->addItem( tr( "Wood2" ) );
	m_scalers.append( 5.0 );
	presets->addItem( tr( "Beats" ) );
	m_scalers.append( 20.0 );
	presets->addItem( tr( "Two Fixed" ) );
	m_scalers.append( 5.0 );
	presets->addItem( tr( "Clump" ) );
	m_scalers.append( 4.0 );
	
	// TubeBell
	presets->addItem( tr( "Tubular Bells" ) );
	m_scalers.append( 1.8 );
	
	// BandedWG
	presets->addItem( tr( "Uniform Bar" ) );
	m_scalers.append( 25.0 );
	presets->addItem( tr( "Tuned Bar" ) );
	m_scalers.append( 10.0 );
	presets->addItem( tr( "Glass" ) );
	m_scalers.append( 16.0 );
	presets->addItem( tr( "Tibetan Bowl" ) );
	m_scalers.append( 7.0 );
	
	return( presets );
}




void mallets::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// ModalBar
	m_hardness->saveSettings( _doc, _this, "hardness" );
	m_position->saveSettings( _doc, _this, "position" );
	m_vibratoGain->saveSettings( _doc, _this, "vib_gain" );
	m_vibratoFreq->saveSettings( _doc, _this, "vib_freq" );
	m_stick->saveSettings( _doc, _this, "stick_mix" );

	// TubeBell
	m_modulator->saveSettings( _doc, _this, "modulator" );
	m_crossfade->saveSettings( _doc, _this, "crossfade" );
	m_lfoSpeed->saveSettings( _doc, _this, "lfo_speed" );
	m_lfoDepth->saveSettings( _doc, _this, "lfo_depth" );
	m_adsr->saveSettings( _doc, _this, "adsr" );
	
	// BandedWG
	m_pressure->saveSettings( _doc, _this, "pressure" );
	m_motion->saveSettings( _doc, _this, "motion" );
	m_vibrato->saveSettings( _doc, _this, "vibrato" );
	m_velocity->saveSettings( _doc, _this, "velocity" );
	m_strike->saveSettings( _doc, _this, "strike" );
	
	m_presets->saveSettings( _doc, _this, "preset" );
	m_spread->saveSettings( _doc, _this, "spread" );
}




void mallets::loadSettings( const QDomElement & _this )
{
	// ModalBar
	m_hardness->loadSettings( _this, "hardness" );
	m_position->loadSettings( _this, "position" );
	m_vibratoGain->loadSettings( _this, "vib_gain" );
	m_vibratoFreq->loadSettings( _this, "vib_freq" );
	m_stick->loadSettings( _this, "stick_mix" );

	// TubeBell
	m_modulator->loadSettings( _this, "modulator" );
	m_crossfade->loadSettings( _this, "crossfade" );
	m_lfoSpeed->loadSettings( _this, "lfo_speed" );
	m_lfoDepth->loadSettings( _this, "lfo_depth" );
	m_adsr->loadSettings( _this, "adsr" );
	
	// BandedWG
	m_pressure->loadSettings( _this, "pressure" );
	m_motion->loadSettings( _this, "motion" );
	m_vibrato->loadSettings( _this, "vibrato" );
	m_velocity->loadSettings( _this, "velocity" );
	m_strike->loadSettings( _this, "strike" );
	
	m_presets->loadSettings( _this, "preset" );
	m_spread->loadSettings( _this, "spread" );
}




QString mallets::nodeName( void ) const
{
	return( malletsstk_plugin_descriptor.name );
}




void mallets::playNote( notePlayHandle * _n )
{
	int p = m_presets->value();
	
	float freq = getInstrumentTrack()->frequency( _n );
	if ( _n->totalFramesPlayed() == 0 )
	{
		float vel = static_cast<float>( _n->getVolume() ) / 100.0f;
		
		if( p < 9 )
		{
			_n->m_pluginData = new malletsSynth( 
					freq,
					vel,
					m_vibratoGain->value(),
					m_hardness->value(),
					m_position->value(),
					m_stick->value(),
					m_vibratoFreq->value(),
					p,
					(Uint8) m_spread->value(),
					eng()->getMixer()->sampleRate() );
		}
		else if( p == 9 )
		{
			_n->m_pluginData = new malletsSynth( 
					freq,
					vel,
					p,
					m_lfoDepth->value(),
					m_modulator->value(),
					m_crossfade->value(),
					m_lfoSpeed->value(),
					m_adsr->value(),
					(Uint8) m_spread->value(),
					eng()->getMixer()->sampleRate() );
		}
		else
		{
			_n->m_pluginData = new malletsSynth(
					freq,
					vel,
					m_pressure->value(),
					m_motion->value(),
					m_vibrato->value(),
					p - 10,
					m_strike->value() * 128.0,
					m_velocity->value(),
					(Uint8) m_spread->value(),
					eng()->getMixer()->sampleRate() );
		}
	}

	const Uint32 frames = eng()->getMixer()->framesPerAudioBuffer();

	malletsSynth * ps = static_cast<malletsSynth *>( _n->m_pluginData );
	sample_t add_scale = 0.0f;
	if( p == 10 )
	{
		add_scale = 
		static_cast<sample_t>( m_strike->value() ) * freq * 2.5f;
	}
	for( Uint32 frame = 0; frame < frames; ++frame )
	{
		const sample_t left = ps->nextSampleLeft() * 
				( m_scalers[m_presets->value()] + add_scale );
		const sample_t right = ps->nextSampleRight() * 
				( m_scalers[m_presets->value()] + add_scale );
		for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS / 2; ++chnl )
		{
			m_buffer[frame][chnl * DEFAULT_CHANNELS / 2] = left;
			m_buffer[frame][( chnl + 1 ) * DEFAULT_CHANNELS / 2] =
									right;
		}
	}
	
	getInstrumentTrack()->processAudioBuffer( m_buffer, frames, _n );
}




void mallets::deleteNotePluginData( notePlayHandle * _n )
{
	delete static_cast<malletsSynth *>( _n->m_pluginData );
}




void mallets::changePreset( int _preset )
{
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
				const Uint8 _delay,
				const sample_rate_t _sample_rate )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( configManager::inst()->stkDir() );
	
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
	
	m_delay = bufferAllocator::alloc<StkFloat>( 256 );
	m_delayRead = 0;
	m_delayWrite = _delay;
	for( Uint16 i = 0; i < 256; i++ )
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
				const Uint8 _delay,
				const sample_rate_t _sample_rate )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( configManager::inst()->stkDir() );
	
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
	
	m_delay = bufferAllocator::alloc<StkFloat>( 256 );
	m_delayRead = 0;
	m_delayWrite = _delay;
	for( Uint16 i = 0; i < 256; i++ )
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
				const Uint8 _delay,
				const sample_rate_t _sample_rate )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( configManager::inst()->stkDir() );
	
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
	
	m_delay = bufferAllocator::alloc<StkFloat>( 256 );
	m_delayRead = 0;
	m_delayWrite = _delay;
	for( Uint16 i = 0; i < 256; i++ )
	{
		m_delay[i] = 0.0;
	}
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new mallets( static_cast<instrumentTrack *>( _data ) ) );
}


}


#include "mallets.moc"


