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

#include <stk/BandedWG.h>
#include <stk/ModalBar.h>
#include <stk/TubeBell.h>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "FontHelper.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"

#include "embed.h"
#include "lmms_math.h"
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
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	{},
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
	m_modulatorModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Modulator" )),
	m_crossfadeModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Crossfade" )),
	m_lfoSpeedModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "LFO speed" )),
	m_lfoDepthModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "LFO depth" )),
	m_adsrModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "ADSR" )),
	m_pressureModel(64.0f, 0.1f, 128.0f, 0.1f, this, tr( "Pressure" )),
	m_motionModel(64.0f, 0.0f, 128.0f, 0.1f, this, tr( "Motion" )),
//	TODO: m_vibratoModel
	m_velocityModel(64.0f, 0.1f, 128.0f, 0.1f, this, tr( "Speed" )),
	m_strikeModel( true, this, tr( "Bowed" ) ),
	m_presetsModel(this, tr("Instrument")),
	m_spreadModel(0, 0, 255, 1, this, tr( "Spread" )),
	m_randomModel(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Randomness")),
	m_versionModel( MALLETS_PRESET_VERSION, 0, MALLETS_PRESET_VERSION, this, "" ),
	m_isOldVersionModel( false, this, "" ),
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
	
	// TubeBell
	m_presetsModel.addItem( tr( "Tubular bells" ) );
	m_scalers.append(1.8f);
	
	// BandedWG
	m_presetsModel.addItem( tr( "Uniform bar" ) );
	m_scalers.append( 25.0 );
	m_presetsModel.addItem( tr( "Tuned bar" ) );
	m_scalers.append( 10.0 );
	m_presetsModel.addItem( tr( "Glass" ) );
	m_scalers.append( 16.0 );
	m_presetsModel.addItem( tr( "Tibetan bowl" ) );
	m_scalers.append( 7.0 );
}




void MalletsInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
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
//	m_motionModel.saveSettings( _doc, _this, "motion" );
//	m_vibratoModel.saveSettings( _doc, _this, "vibrato" );
	m_velocityModel.saveSettings( _doc, _this, "velocity" );
	m_strikeModel.saveSettings( _doc, _this, "strike" );

	m_presetsModel.saveSettings( _doc, _this, "preset" );
	m_spreadModel.saveSettings( _doc, _this, "spread" );
	m_randomModel.saveSettings(_doc, _this, "randomness");
	m_versionModel.saveSettings( _doc, _this, "version" );
	m_isOldVersionModel.saveSettings( _doc, _this, "oldversion" );
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

	// TubeBell
	m_modulatorModel.loadSettings( _this, "modulator" );
	m_crossfadeModel.loadSettings( _this, "crossfade" );
	m_lfoSpeedModel.loadSettings( _this, "lfo_speed" );
	m_lfoDepthModel.loadSettings( _this, "lfo_depth" );
	m_adsrModel.loadSettings( _this, "adsr" );
	
	// BandedWG
	m_pressureModel.loadSettings( _this, "pressure" );
//	m_motionModel.loadSettings( _this, "motion" );
//	m_vibratoModel.loadSettings( _this, "vibrato" );
	m_velocityModel.loadSettings( _this, "velocity" );
	m_strikeModel.loadSettings( _this, "strike" );

	m_presetsModel.loadSettings( _this, "preset" );
	m_spreadModel.loadSettings( _this, "spread" );
	m_randomModel.loadSettings(_this, "randomness");
	m_isOldVersionModel.loadSettings( _this, "oldversion" );

	// To maintain backward compatibility
	if( !_this.hasAttribute( "version" ) )
	{
		m_isOldVersionModel.setValue( true );
		m_vibratoGainModel.setValue( 0.0f );
		if( m_presetsModel.value() == 1 )
		{
		/* 	Earlier mallets used the stk internal
			default of 0.2. 0.2 * 128.0 = 25.6 	*/
			m_vibratoGainModel.setValue( 25.6f );
		}
		if( m_presetsModel.value() != 1 )
		{
			// Frequency actually worked for Vibraphone!
			m_vibratoFreqModel.setValue( 0.0f );
		}
		/*	Modalbar preset values, see stk, ModalBar.cpp
			void ModalBar :: setPreset( int preset )
			Stick Mix * 128.0
			m_positionModel values over 64 is formatted to the
			new knob by 128 - x	*/

		switch( m_presetsModel.value() )
		{
			case 0:
				m_hardnessModel.setValue( 55.0f );
				m_positionModel.setValue( 57.0f );
				m_stickModel.setValue( 12.0f );
				break;
			case 1:
				m_hardnessModel.setValue( 50.0f );
				m_positionModel.setValue( 55.0f );// 128 - 73!
				m_stickModel.setValue( 10.0f );
				break;
			case 2:
				m_hardnessModel.setValue( 78.0f );
				m_positionModel.setValue( 46.0f );
				m_stickModel.setValue( 18.0f );
				break;
			case 3:
				m_hardnessModel.setValue( 59.0f );
				m_positionModel.setValue( 48.0f );
				m_stickModel.setValue( 6.0f );
				break;
			case 4:
				m_hardnessModel.setValue( 58.0f );
				m_positionModel.setValue( 32.0f );
				m_stickModel.setValue( 13.0f );
				break;
			case 5:
				m_hardnessModel.setValue( 40.0f );
				m_positionModel.setValue( 57.0f );
				m_stickModel.setValue( 14.0f );
				break;
			case 6:
				m_hardnessModel.setValue( 51.0f );
				m_positionModel.setValue( 38.0f );
				m_stickModel.setValue( 9.0f );
				break;
			case 7:
				m_hardnessModel.setValue( 58.0f );
				m_positionModel.setValue( 58.0f );
				m_stickModel.setValue( 9.0f );
				break;
			case 8:
				m_hardnessModel.setValue( 50.0f );
				m_positionModel.setValue( 55.0f );// 128 - 73!
				m_stickModel.setValue( 10.0f );
				break;
		}
	}
}




QString MalletsInstrument::nodeName() const
{
	return( malletsstk_plugin_descriptor.name );
}




void MalletsInstrument::playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer )
{
	if( m_filesMissing )
	{
		return;
	}

	int p = m_presetsModel.value();

	const float freq = _n->frequency();
	if (!_n->m_pluginData)
	{
		// If newer projects, adjust velocity to within stk's limits
		float velocityAdjust =
			m_isOldVersionModel.value() ? 100.0 : 200.0;
		const float vel = _n->getVolume() / velocityAdjust;

		const float random = m_randomModel.value();
		float hardness = m_hardnessModel.value();
		float position = m_positionModel.value();
		float modulator = m_modulatorModel.value();
		float crossfade = m_crossfadeModel.value();
		float pressure = m_pressureModel.value();
		float speed = m_velocityModel.value();

		if (p < 9)
		{
			hardness += random * fastRand(-64.f, +64.f);
			hardness = std::clamp(hardness, 0.0f, 128.0f);

			position += random * fastRand(-32.f, +32.f);
			position = std::clamp(position, 0.0f, 64.0f);
		}
		else if (p == 9)
		{
			modulator += random * fastRand(-64.f, +64.f);
			modulator = std::clamp(modulator, 0.0f, 128.0f);

			crossfade += random * fastRand(-64.f, +64.f);
			crossfade = std::clamp(crossfade, 0.0f, 128.0f);
		}
		else
		{
			pressure += random * fastRand(-64.f, +64.f);
			pressure = std::clamp(pressure, 0.0f, 128.0f);

			speed += random * fastRand(-64.f, +64.f);
			speed = std::clamp(speed, 0.0f, 128.0f);
		}

		// critical section as STK is not thread-safe
		static QMutex m;
		m.lock();
		if( p < 9 )
		{
			_n->m_pluginData = new MalletsSynth( freq,
						vel,
						m_stickModel.value(),
						hardness,
						position,
						m_vibratoGainModel.value(),
						m_vibratoFreqModel.value(),
						p,
						(uint8_t) m_spreadModel.value(),
				Engine::audioEngine()->outputSampleRate() );
		}
		else if( p == 9 )
		{
			_n->m_pluginData = new MalletsSynth( freq,
						vel,
						p,
						m_lfoDepthModel.value(),
						modulator,
						crossfade,
						m_lfoSpeedModel.value(),
						m_adsrModel.value(),
						(uint8_t) m_spreadModel.value(),
				Engine::audioEngine()->outputSampleRate() );
		}
		else
		{
			_n->m_pluginData = new MalletsSynth( freq,
						vel,
						pressure,
						m_motionModel.value(),
						m_vibratoModel.value(),
						p - 10,
						m_strikeModel.value() * 128.0,
						speed,
						(uint8_t) m_spreadModel.value(),
				Engine::audioEngine()->outputSampleRate() );
		}
		m.unlock();
		static_cast<MalletsSynth *>(_n->m_pluginData)->setPresetIndex(p);
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	auto ps = static_cast<MalletsSynth*>(_n->m_pluginData);
	ps->setFrequency(freq);

	p = ps->presetIndex();
	if (p < 9) // ModalBar updates
	{
		ps->setVibratoGain(m_vibratoGainModel.value());
		ps->setVibratoFreq(m_vibratoFreqModel.value());
	}
	else if (p == 9) // Tubular Bells updates
	{
		ps->setADSR(m_adsrModel.value());
		ps->setLFODepth(m_lfoDepthModel.value());
		ps->setLFOSpeed(m_lfoSpeedModel.value());
	}

	sample_t add_scale = 0.0f;
	if( p == 10 && m_isOldVersionModel.value() == true )
	{
		add_scale = static_cast<sample_t>( m_strikeModel.value() ) * freq * 2.5f;
	}

	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		_working_buffer[frame][0] = ps->nextSampleLeft() *
				( m_scalers[p] + add_scale );
		_working_buffer[frame][1] = ps->nextSampleRight() *
				( m_scalers[p] + add_scale );
	}
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
	
	m_tubeBellWidget = setupTubeBellControls( this );
	setWidgetBackground( m_tubeBellWidget, "artwork" );
	m_tubeBellWidget->move( 0,0 );
	
	m_bandedWGWidget = setupBandedWGControls( this );
	setWidgetBackground( m_bandedWGWidget, "artwork" );
	m_bandedWGWidget->move( 0,0 );

	changePreset(); // Show widget

	m_presetsCombo = new ComboBox( this, tr( "Instrument" ) );
	m_presetsCombo->setGeometry( 140, 50, 99, ComboBox::DEFAULT_HEIGHT );
	
	connect( &_instrument->m_presetsModel, SIGNAL( dataChanged() ),
		 this, SLOT( changePreset() ) );
	
	m_spreadKnob = new Knob(KnobType::Vintage32, tr("Spread"), SMALL_FONT_SIZE, this);
	m_spreadKnob->move( 190, 140 );
	m_spreadKnob->setHintText( tr( "Spread:" ), "" );

	m_randomKnob = new Knob(KnobType::Vintage32, tr("Random"), SMALL_FONT_SIZE, this);
	m_randomKnob->move(190, 190);
	m_randomKnob->setHintText(tr("Random:"), "");

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
	auto widget = new QWidget(_parent);
	widget->setFixedSize( 250, 250 );
		
	m_hardnessKnob = new Knob(KnobType::Vintage32, tr("Hardness"), SMALL_FONT_SIZE, widget);
	m_hardnessKnob->move( 30, 90 );
	m_hardnessKnob->setHintText( tr( "Hardness:" ), "" );

	m_positionKnob = new Knob(KnobType::Vintage32, tr("Position"), SMALL_FONT_SIZE, widget);
	m_positionKnob->move( 110, 90 );
	m_positionKnob->setHintText( tr( "Position:" ), "" );

	m_vibratoGainKnob = new Knob(KnobType::Vintage32, tr("Vibrato gain"), SMALL_FONT_SIZE, widget);
	m_vibratoGainKnob->move( 30, 140 );
	m_vibratoGainKnob->setHintText( tr( "Vibrato gain:" ), "" );

	m_vibratoFreqKnob = new Knob(KnobType::Vintage32, tr("Vibrato frequency"), SMALL_FONT_SIZE, widget);
	m_vibratoFreqKnob->move( 110, 140 );
	m_vibratoFreqKnob->setHintText( tr( "Vibrato frequency:" ), "" );

	m_stickKnob = new Knob(KnobType::Vintage32, tr("Stick mix"), SMALL_FONT_SIZE, widget);
	m_stickKnob->move( 190, 90 );
	m_stickKnob->setHintText( tr( "Stick mix:" ), "" );

	return( widget );
}




QWidget * MalletsInstrumentView::setupTubeBellControls( QWidget * _parent )
{
	auto widget = new QWidget(_parent);
	widget->setFixedSize( 250, 250 );
	
	m_modulatorKnob = new Knob(KnobType::Vintage32, tr("Modulator"), SMALL_FONT_SIZE, widget);
	m_modulatorKnob->move( 30, 90 );
	m_modulatorKnob->setHintText( tr( "Modulator:" ), "" );

	m_crossfadeKnob = new Knob(KnobType::Vintage32, tr("Crossfade"), SMALL_FONT_SIZE, widget);
	m_crossfadeKnob->move( 110, 90 );
	m_crossfadeKnob->setHintText( tr( "Crossfade:" ), "" );
	
	m_lfoSpeedKnob = new Knob(KnobType::Vintage32, tr("LFO speed"), SMALL_FONT_SIZE, widget);
	m_lfoSpeedKnob->move( 30, 140 );
	m_lfoSpeedKnob->setHintText( tr( "LFO speed:" ), "" );
	
	m_lfoDepthKnob = new Knob(KnobType::Vintage32, tr("LFO depth"), SMALL_FONT_SIZE, widget);
	m_lfoDepthKnob->move( 110, 140 );
	m_lfoDepthKnob->setHintText( tr( "LFO depth:" ), "" );
	
	m_adsrKnob = new Knob(KnobType::Vintage32, tr("ADSR"), SMALL_FONT_SIZE, widget);
	m_adsrKnob->move( 190, 90 );
	m_adsrKnob->setHintText( tr( "ADSR:" ), "" );

	return( widget );
}




QWidget * MalletsInstrumentView::setupBandedWGControls( QWidget * _parent )
{
	// BandedWG
	auto widget = new QWidget(_parent);
	widget->setFixedSize( 250, 250 );
	
/*	m_strikeLED = new LedCheckBox( tr( "Bowed" ), widget );
	m_strikeLED->move( 138, 25 );*/

	m_pressureKnob = new Knob(KnobType::Vintage32, tr("Pressure"), SMALL_FONT_SIZE, widget);
	m_pressureKnob->move( 30, 90 );
	m_pressureKnob->setHintText( tr( "Pressure:" ), "" );

/*	m_motionKnob = new Knob(KnobType::Vintage32, tr("Motion"), SMALL_FONT_SIZE, widget);
	m_motionKnob->move( 110, 90 );
	m_motionKnob->setHintText( tr( "Motion:" ), "" );*/

	m_velocityKnob = new Knob(KnobType::Vintage32, tr("Speed"), SMALL_FONT_SIZE, widget);
	m_velocityKnob->move( 30, 140 );
	m_velocityKnob->setHintText( tr( "Speed:" ), "" );
	
/*	m_vibratoKnob = new Knob(KnobType::Vintage32, tr("Vibrato"), SMALL_FONT_SIZE, widget);
	m_vibratoKnob->move( 110, 140 );
	m_vibratoKnob->setHintText( tr( "Vibrato:" ), "" );*/
	
	return( widget );
}




void MalletsInstrumentView::modelChanged()
{
	auto inst = castModel<MalletsInstrument>();
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
//	m_motionKnob->setModel( &inst->m_motionModel );
//	m_vibratoKnob->setModel( &inst->m_vibratoModel );
	m_velocityKnob->setModel( &inst->m_velocityModel );
//	m_strikeLED->setModel( &inst->m_strikeModel );
	m_presetsCombo->setModel( &inst->m_presetsModel );
	m_spreadKnob->setModel( &inst->m_spreadModel );
	m_randomKnob->setModel(&inst->m_randomModel);
}




void MalletsInstrumentView::changePreset()
{
	auto inst = castModel<MalletsInstrument>();
	int _preset = inst->m_presetsModel.value();

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




// TubeBell
MalletsSynth::MalletsSynth( const StkFloat _pitch,
				const StkFloat _velocity,
				const int _preset,
				const StkFloat _control1,
				const StkFloat _control2,
				const StkFloat _control4,
				const StkFloat _control11,
				const StkFloat _control128,
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




// BandedWG
MalletsSynth::MalletsSynth( const StkFloat _pitch,
				const StkFloat _velocity,
				const StkFloat _control2,
				const StkFloat _control4,
				const StkFloat _control11,
				const int _control16,
				const StkFloat _control64,
				const StkFloat _control128,
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
