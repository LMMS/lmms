/*
 * Mallets.cpp - tuned instruments that one would bang upon
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009-2015 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2016 Oskar Wallgren <oskarwallgren13/at/gmail.com>
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

#include "Mallets.h"

#include <QDir>
#include <QDomElement>
#include <QMessageBox>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "lmms_math.h"
#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT malletsstk_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Mallets",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Tuneful things to bang on" ),
	"Danny McRae <khjklujn/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
} ;

}


MalletsInstrument::MalletsInstrument( InstrumentTrack * _instrument_track ):
	Instrument( _instrument_track, &malletsstk_plugin_descriptor ),
	m_hardnessModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Hardness" )),
	m_positionModel(64.0f, 0.0f, 64.0f, 0.1f, this, tr( "Position" )),
	m_vibratoGainModel(0.0f, 0.0f, 128.0f, 0.1f, this, tr( "Vibrato gain" )),
	m_vibratoFreqModel(0.0f, 0.0f, 128.0f, 0.1f, this, tr( "Vibrato frequency" )),
	m_stickModel(0.0f, 0.0f, 128.0f, 0.1f, this, tr( "Stick mix" )),
	m_presetsModel(this),
	m_spreadModel(0, 0, 255, 1, this, tr( "Spread" )),
	m_randomModel(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Randomness")),
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
	m_presetsModel.addItem( tr( "Wood 1" ) );
	m_scalers.append( 4.0 );
	m_presetsModel.addItem( tr( "Reso" ) );
	m_scalers.append( 2.5 );
	m_presetsModel.addItem( tr( "Wood 2" ) );
	m_scalers.append( 5.0 );
	m_presetsModel.addItem( tr( "Beats" ) );
	m_scalers.append( 20.0 );
	m_presetsModel.addItem( tr( "Two fixed" ) );
	m_scalers.append( 5.0 );
	m_presetsModel.addItem( tr( "Clump" ) );
	m_scalers.append( 4.0 );
}




void MalletsInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// ModalBar
	m_hardnessModel.saveSettings( _doc, _this, "hardness" );
	m_positionModel.saveSettings( _doc, _this, "position" );
	m_vibratoGainModel.saveSettings( _doc, _this, "vib_gain" );
	m_vibratoFreqModel.saveSettings( _doc, _this, "vib_freq" );
	m_stickModel.saveSettings( _doc, _this, "stick_mix" );

	m_presetsModel.saveSettings( _doc, _this, "preset" );
	m_spreadModel.saveSettings( _doc, _this, "spread" );
	m_randomModel.saveSettings(_doc, _this, "randomness");
	m_versionModel.saveSettings( _doc, _this, "version" );
}




void MalletsInstrument::loadSettings( const QDomElement & _this )
{
	m_versionModel.loadSettings( _this, "version" );

	// ModalBar
	m_hardnessModel.loadSettings( _this, "hardness" );
	m_positionModel.loadSettings( _this, "position" );
	m_vibratoGainModel.loadSettings( _this, "vib_gain" );
	m_vibratoFreqModel.loadSettings( _this, "vib_freq" );
	m_stickModel.loadSettings( _this, "stick_mix" );

	m_presetsModel.loadSettings( _this, "preset" );
	m_spreadModel.loadSettings( _this, "spread" );
	m_randomModel.loadSettings(_this, "randomness");
}




QString MalletsInstrument::nodeName() const
{
	return( malletsstk_plugin_descriptor.name );
}




void MalletsInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if( m_filesMissing )
	{
		return;
	}

	int p = m_presetsModel.value();

	const float freq = _n->frequency();
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == nullptr )
	{
		const float vel = _n->getVolume() / 200.0;

		const float random = m_randomModel.value();
		float hardness = m_hardnessModel.value();
		float position = m_positionModel.value();

		hardness += random * static_cast<float>(rand() % 128) - 64.0;
		hardness = std::clamp(0.0f, hardness, 128.0f);

		position += random * static_cast<float>(rand() % 64) - 32.0;
		position = std::clamp(0.0f, position, 64.0f);

		// critical section as STK is not thread-safe
		static QMutex m;
		m.lock();
			_n->m_pluginData = new MalletsSynth( freq,
						vel,
						m_stickModel.value(),
						hardness,
						position,
						m_vibratoGainModel.value(),
						m_vibratoFreqModel.value(),
						p,
						(uint8_t) m_spreadModel.value(),
				Engine::audioEngine()->processingSampleRate() );
		m.unlock();
		static_cast<MalletsSynth *>(_n->m_pluginData)->setPresetIndex(p);
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	MalletsSynth * ps = static_cast<MalletsSynth *>( _n->m_pluginData );
	ps->setFrequency( freq );
	p = ps->presetIndex();

	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		_working_buffer[frame][0] = ps->nextSampleLeft() * m_scalers[p];
		_working_buffer[frame][1] = ps->nextSampleRight() * m_scalers[p];
	}

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}




void MalletsInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<MalletsSynth *>( _n->m_pluginData );
}




gui::PluginView * MalletsInstrument::instantiateView( QWidget * _parent )
{
	return( new gui::MalletsInstrumentView( this, _parent ) );
}


namespace gui
{


MalletsInstrumentView::MalletsInstrumentView( MalletsInstrument * _instrument,
							QWidget * _parent ) :
        InstrumentViewFixedSize( _instrument, _parent )
{
	m_modalBarWidget = setupModalBarControls( this );
	setWidgetBackground( m_modalBarWidget, "artwork" );
	m_modalBarWidget->move( 0,0 );

	changePreset(); // Show widget

	m_presetsCombo = new ComboBox( this, tr( "Instrument" ) );
	m_presetsCombo->setGeometry( 140, 50, 99, ComboBox::DEFAULT_HEIGHT );
	m_presetsCombo->setFont( pointSize<8>( m_presetsCombo->font() ) );
	
	connect( &_instrument->m_presetsModel, SIGNAL( dataChanged() ),
		 this, SLOT( changePreset() ) );
	
	m_spreadKnob = new Knob( knobVintage_32, this );
	m_spreadKnob->setLabel( tr( "Spread" ) );
	m_spreadKnob->move( 190, 140 );
	m_spreadKnob->setHintText( tr( "Spread:" ), "" );

	m_randomKnob = new Knob(knobVintage_32, this);
	m_randomKnob->setLabel(tr("Random"));
	m_randomKnob->move(190, 190);
	m_randomKnob->setHintText(tr("Random:"),"");

	// try to inform user about missing Stk-installation
	if( _instrument->m_filesMissing && getGUI() != nullptr )
	{
		QMessageBox::information( 0, tr( "Missing files" ),
				tr( "Your Stk-installation seems to be "
					"incomplete. Please make sure "
					"the full Stk-package is installed!" ),
				QMessageBox::Ok );
	}
}




void MalletsInstrumentView::setWidgetBackground( QWidget * _widget, const QString & _pic )
{
	_widget->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( _widget->backgroundRole(),
		PLUGIN_NAME::getIconPixmap( _pic.toLatin1().constData() ) );
	_widget->setPalette( pal );
}




QWidget * MalletsInstrumentView::setupModalBarControls( QWidget * _parent )
{
	QWidget * widget = new QWidget( _parent );
	widget->setFixedSize( 250, 250 );
		
	m_hardnessKnob = new Knob( knobVintage_32, widget );
	m_hardnessKnob->setLabel( tr( "Hardness" ) );
	m_hardnessKnob->move( 30, 90 );
	m_hardnessKnob->setHintText( tr( "Hardness:" ), "" );

	m_positionKnob = new Knob( knobVintage_32, widget );
	m_positionKnob->setLabel( tr( "Position" ) );
	m_positionKnob->move( 110, 90 );
	m_positionKnob->setHintText( tr( "Position:" ), "" );

	m_vibratoGainKnob = new Knob( knobVintage_32, widget );
	m_vibratoGainKnob->setLabel( tr( "Vibrato gain" ) );
	m_vibratoGainKnob->move( 30, 140 );
	m_vibratoGainKnob->setHintText( tr( "Vibrato gain:" ), "" );

	m_vibratoFreqKnob = new Knob( knobVintage_32, widget );
	m_vibratoFreqKnob->setLabel( tr( "Vibrato frequency" ) );
	m_vibratoFreqKnob->move( 110, 140 );
	m_vibratoFreqKnob->setHintText( tr( "Vibrato frequency:" ), "" );

	m_stickKnob = new Knob( knobVintage_32, widget );
	m_stickKnob->setLabel( tr( "Stick mix" ) );
	m_stickKnob->move( 190, 90 );
	m_stickKnob->setHintText( tr( "Stick mix:" ), "" );

	return( widget );
}




void MalletsInstrumentView::modelChanged()
{
	MalletsInstrument * inst = castModel<MalletsInstrument>();
	m_hardnessKnob->setModel( &inst->m_hardnessModel );
	m_positionKnob->setModel( &inst->m_positionModel );
	m_vibratoGainKnob->setModel( &inst->m_vibratoGainModel );
	m_vibratoFreqKnob->setModel( &inst->m_vibratoFreqModel );
	m_stickKnob->setModel( &inst->m_stickModel );
	m_presetsCombo->setModel( &inst->m_presetsModel );
	m_spreadKnob->setModel( &inst->m_spreadModel );
	m_randomKnob->setModel(&inst->m_randomModel);
}




void MalletsInstrumentView::changePreset()
{
	m_modalBarWidget->show();
}


} // namespace gui


// ModalBar
MalletsSynth::MalletsSynth( const StkFloat _pitch,
				const StkFloat _velocity,
				const StkFloat _control1,
				const StkFloat _control2,
				const StkFloat _control4,
				const StkFloat _control8,
				const StkFloat _control11,
				const int _control16,
				const uint8_t _delay,
				const sample_rate_t _sample_rate ) :
	m_presetIndex(0)
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( QDir( ConfigManager::inst()->stkDir() ).absolutePath()
						.toLocal8Bit().constData() );
#ifndef LMMS_DEBUG
		Stk::showWarnings( false );
#endif

		m_voice = new ModalBar();
	
		m_voice->controlChange( 16, _control16 );
		m_voice->controlChange( 1, _control1 );
		m_voice->controlChange( 2, _control2 );
		m_voice->controlChange( 4, _control4 );
		m_voice->controlChange( 8, _control8 );
		m_voice->controlChange( 11, _control11 );
		m_voice->controlChange( 128, 128.0f );
		
		m_voice->noteOn( _pitch, _velocity );
	}
	catch( ... )
	{
		m_voice = nullptr;
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
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * m, void * )
{
	return new MalletsInstrument( static_cast<InstrumentTrack *>( m ) );
}


}


} // namespace lmms
