/*
 * mallets2.cpp - More tuned instruments that one would bang upon
 *
 * Copyright (c) 2016 Oskar Wallgren <oskarwallgren13/at/gmail.com>
 *
 * Improved version of Mallets by Danny McRae and Tobias Doerffel.
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

#include "mallets2.h"

#include <QDir>
#include <QMessageBox>

#include "BandedWG.h"
#include "ModalBar.h"
#include "TubeBell.h"

#include "Engine.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT mallets2stk_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Mallets 2",
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


mallets2Instrument::mallets2Instrument( InstrumentTrack * _instrument_track ):
	Instrument( _instrument_track, &mallets2stk_plugin_descriptor ),
	m_presetsModel(this),
	m_hardnessModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Hardness" )),
	m_positionModel(64.0f, 0.0f, 64.0f, 0.1f, this, tr( "Position" )),
	m_stickModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Stick Mix" )),
	m_vibratoGainModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Vibrato Gain" )),
	m_vibratoFreqModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Vibrato Freq" )),
	m_spreadModel(0, 0, 255, 1, this, tr( "Spread" )),
	m_randomModel(0.0f, 0.0f, 1.0f, 0.01f, this, tr( "Randomness" )),
	m_filesMissing( !QDir( ConfigManager::inst()->stkDir() ).exists() ||
		!QFileInfo( ConfigManager::inst()->stkDir() + "/sinewave.raw" ).exists() )
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
}




mallets2Instrument::~mallets2Instrument()
{
}




void mallets2Instrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// ModalBar
	m_presetsModel.saveSettings( _doc, _this, "preset" );
	m_hardnessModel.saveSettings( _doc, _this, "hardness" );
	m_positionModel.saveSettings( _doc, _this, "position" );
	m_stickModel.saveSettings( _doc, _this, "stick_mix" );
	m_vibratoGainModel.saveSettings( _doc, _this, "vib_gain" );
	m_vibratoFreqModel.saveSettings( _doc, _this, "vib_freq" );
	m_spreadModel.saveSettings( _doc, _this, "spread" );
	m_randomModel.saveSettings( _doc, _this, "randomness" );
}




void mallets2Instrument::loadSettings( const QDomElement & _this )
{
	// ModalBar
	m_presetsModel.loadSettings( _this, "preset" );
	m_hardnessModel.loadSettings( _this, "hardness" );
	m_positionModel.loadSettings( _this, "position" );
	m_stickModel.loadSettings( _this, "stick_mix" );
	m_vibratoGainModel.loadSettings( _this, "vib_gain" );
	m_vibratoFreqModel.loadSettings( _this, "vib_freq" );
	m_spreadModel.loadSettings( _this, "spread" );
	m_randomModel.loadSettings( _this, "randomness" );
}




QString mallets2Instrument::nodeName() const
{
	return( mallets2stk_plugin_descriptor.name );
}




void mallets2Instrument::playNote( NotePlayHandle * _n,
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

		const float random = m_randomModel.value();
		float hardness = m_hardnessModel.value();
		hardness += ( ( random * ( float )( rand() % 128 )  ) - 64.0 );
		if( hardness < 0.0 )
		{
			hardness = 0.0;
		}
		else if( hardness > 128.0 )
		{
			hardness = 128.0;
		}

		float position = m_positionModel.value();
		position += ( ( random * ( float )( rand() % 64 )  ) - 32.0 );
		if( position < 0.0 )
		{
			position = 0.0;
		}
		else if( position > 64.0 )
		{
			position = 64.0;
		}

		// critical section as STK is not thread-safe
		static QMutex m;
		m.lock();

		_n->m_pluginData = new mallets2Synth( freq,
					vel,
					p,
					hardness, //m_hardnessModel.value(),
					position, //m_positionModel.value(),
					m_stickModel.value(),
					m_vibratoGainModel.value(),
					m_vibratoFreqModel.value(),
					(uint8_t) m_spreadModel.value(),
					m_randomModel.value(),
				Engine::mixer()->processingSampleRate() );
		m.unlock();
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	mallets2Synth * ps = static_cast<mallets2Synth *>( _n->m_pluginData );
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




void mallets2Instrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<mallets2Synth *>( _n->m_pluginData );
}




PluginView * mallets2Instrument::instantiateView( QWidget * _parent )
{
	return( new mallets2InstrumentView( this, _parent ) );
}




mallets2InstrumentView::mallets2InstrumentView( mallets2Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	m_modalBarWidget = setupModalBarControls( this );
	setWidgetBackground( m_modalBarWidget, "artwork" );
	m_modalBarWidget->show();
	m_modalBarWidget->move( 0,0 );

	m_presetsCombo = new ComboBox( this, tr( "Instrument" ) );
	m_presetsCombo->setGeometry( 140, 50, 99, 22 );
	m_presetsCombo->setFont( pointSize<8>( m_presetsCombo->font() ) );

	connect( &_instrument->m_presetsModel, SIGNAL( dataChanged() ),
		 this, SLOT( changePreset() ) );

	m_hardnessKnob = new Knob( knobVintage_32, this );
	m_hardnessKnob->setLabel( tr( "Hardness" ) );
	m_hardnessKnob->move( 30, 90 );
	m_hardnessKnob->setHintText( tr( "Hardness:" ), "" );

	m_positionKnob = new Knob( knobVintage_32, this );
	m_positionKnob->setLabel( tr( "Position" ) );
	m_positionKnob->move( 110, 90 );
	m_positionKnob->setHintText( tr( "Position:" ), "" );

	m_stickKnob = new Knob( knobVintage_32, this );
	m_stickKnob->setLabel( tr( "Stick Mix" ) );
	m_stickKnob->move( 190, 90 );
	m_stickKnob->setHintText( tr( "Stick Mix:" ), "" );

	m_vibratoGainKnob = new Knob( knobVintage_32, this );
	m_vibratoGainKnob->setLabel( tr( "Vib Gain" ) );
	m_vibratoGainKnob->move( 30, 140 );
	m_vibratoGainKnob->setHintText( tr( "Vib Gain:" ), "" );

	m_vibratoFreqKnob = new Knob( knobVintage_32, this );
	m_vibratoFreqKnob->setLabel( tr( "Vib Freq" ) );
	m_vibratoFreqKnob->move( 110, 140 );
	m_vibratoFreqKnob->setHintText( tr( "Vib Freq:" ), "" );

	m_spreadKnob = new Knob( knobVintage_32, this );
	m_spreadKnob->setLabel( tr( "Spread" ) );
	m_spreadKnob->move( 190, 140 );
	m_spreadKnob->setHintText( tr( "Spread:" ), "" );

	m_randomKnob = new Knob( knobVintage_32, this );
	m_randomKnob->setLabel( tr( "Random" ) );
	m_randomKnob->move( 190, 190 );
	m_randomKnob->setHintText( tr( "Random:" ), "" );

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




mallets2InstrumentView::~mallets2InstrumentView()
{
}



void mallets2InstrumentView::setWidgetBackground( QWidget * _widget, const QString & _pic )
{
	_widget->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( _widget->backgroundRole(),
		PLUGIN_NAME::getIconPixmap( _pic.toLatin1().constData() ) );
	_widget->setPalette( pal );
}




QWidget * mallets2InstrumentView::setupModalBarControls( QWidget * _parent )
{
	QWidget * widget = new QWidget( _parent );
	widget->setFixedSize( 250, 250 );
	return( widget );
}





void mallets2InstrumentView::modelChanged()
{
	mallets2Instrument * inst = castModel<mallets2Instrument>();
	m_presetsCombo->setModel( &inst->m_presetsModel );
	m_hardnessKnob->setModel( &inst->m_hardnessModel );
	m_positionKnob->setModel( &inst->m_positionModel );
	m_stickKnob->setModel( &inst->m_stickModel );
	m_vibratoGainKnob->setModel( &inst->m_vibratoGainModel );
	m_vibratoFreqKnob->setModel( &inst->m_vibratoFreqModel );
	m_spreadKnob->setModel( &inst->m_spreadModel );
	m_randomKnob->setModel( &inst->m_randomModel );
}




void mallets2InstrumentView::changePreset()
{
	m_modalBarWidget->show();
}




// ModalBar
mallets2Synth::mallets2Synth( 	const StkFloat _pitch,
				const StkFloat _velocity,
				const int modalPresets,
				const StkFloat stickHardness,
				const StkFloat stickPosition,
				const StkFloat stickMix,
				const StkFloat vibratoGain,
				const StkFloat vibratoFrequency,
				const uint8_t delay,
				const StkFloat randomness,
				const sample_rate_t _sample_rate )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( QDir( ConfigManager::inst()->stkDir() ).absolutePath()
						.toLatin1().constData() );
		m_voice = new ModalBar();
		m_voice->controlChange( 16, modalPresets );
		m_voice->controlChange( 2, stickHardness );
		m_voice->controlChange( 4, stickPosition );
		m_voice->controlChange( 1, stickMix );
		m_voice->controlChange( 8, vibratoGain );
		m_voice->controlChange( 11, vibratoFrequency );
		m_voice->controlChange( 128, 128.0f ); // volume
		m_voice->noteOn( _pitch, _velocity );
	}
	catch( ... )
	{
		m_voice = NULL;
	}

	m_delay = new StkFloat[256];
	m_delayRead = 0;
	m_delayWrite = delay;
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
	return new mallets2Instrument( static_cast<InstrumentTrack *>( _data ) );
}


}





