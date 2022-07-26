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
#include <QDebug>
#include <QDir>
#include <QDomElement>
#include <QLabel>
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
	m_f0fixedModel(false, this, tr( "Fixed 0" )),
	m_f1fixedModel(false, this, tr( "Fixed 1" )),
	m_f2fixedModel(false, this, tr( "Fixed 2" )),
	m_f3fixedModel(false, this, tr( "Fixed 3" )),
	m_freq0Model(0.0f, 0.0f, 20.0f, 0.01f, this, tr( "Freq 0" )),
	m_freq1Model(0.0f, 0.0f, 20.0f, 0.01f, this, tr( "Freq 1" )),
	m_freq2Model(0.0f, 0.0f, 20.0f, 0.01f, this, tr( "Freq 2" )),
	m_freq3Model(0.0f, 0.0f, 20.0f, 0.01f, this, tr( "Freq 3" )),
	m_res0Model(0.9999f, 0.999f, 1.0f, 0.00001f, this, tr( "Res 0" )),
	m_res1Model(0.9999f, 0.999f, 1.0f, 0.00001f, this, tr( "Res 1" )),
	m_res2Model(0.9999f, 0.999f, 1.0f, 0.00001f, this, tr( "Res 2" )),
	m_res3Model(0.9999f, 0.999f, 1.0f, 0.00001f, this, tr( "Res 3" )),
	m_vol0Model(0.0f, 0.0f, 0.1f, 0.00001, this, tr( "Vol 0" )),
	m_vol1Model(0.0f, 0.0f, 0.1f, 0.00001, this, tr( "Vol 1" )),
	m_vol2Model(0.0f, 0.0f, 0.1f, 0.00001, this, tr( "Vol 2" )),
	m_vol3Model(0.0f, 0.0f, 0.1f, 0.00001, this, tr( "Vol 3" )),
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
	m_presetsModel.addItem( tr( "Vibraphone" ) );
	m_presetsModel.addItem( tr( "Agogo" ) );
	m_presetsModel.addItem( tr( "Wood 1" ) );
	m_presetsModel.addItem( tr( "Reso" ) );
	m_presetsModel.addItem( tr( "Wood 2" ) );
	m_presetsModel.addItem( tr( "Beats" ) );
	m_presetsModel.addItem( tr( "Two fixed" ) );
	m_presetsModel.addItem( tr( "Clump" ) );
}




void MalletsInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// ModalBar
	m_f0fixedModel.saveSettings( _doc, _this, "fixed0");
	m_f1fixedModel.saveSettings( _doc, _this, "fixed1");
	m_f2fixedModel.saveSettings( _doc, _this, "fixed2");
	m_f3fixedModel.saveSettings( _doc, _this, "fixed3");
	m_freq0Model.saveSettings( _doc, _this, "freq0" );
	m_freq1Model.saveSettings( _doc, _this, "freq1" );
	m_freq2Model.saveSettings( _doc, _this, "freq2" );
	m_freq3Model.saveSettings( _doc, _this, "freq3" );
	m_res0Model.saveSettings( _doc, _this, "res0" );
	m_res1Model.saveSettings( _doc, _this, "res1" );
	m_res2Model.saveSettings( _doc, _this, "res2" );
	m_res3Model.saveSettings( _doc, _this, "res3" );
	m_vol0Model.saveSettings( _doc, _this, "vol0" );
	m_vol1Model.saveSettings( _doc, _this, "vol1" );
	m_vol2Model.saveSettings( _doc, _this, "vol2" );
	m_vol3Model.saveSettings( _doc, _this, "vol3" );
	m_hardnessModel.saveSettings( _doc, _this, "hardness" );
	m_positionModel.saveSettings( _doc, _this, "position" );
	m_vibratoGainModel.saveSettings( _doc, _this, "vib_gain" );
	m_vibratoFreqModel.saveSettings( _doc, _this, "vib_freq" );
	m_stickModel.saveSettings( _doc, _this, "stick_mix" );

	m_presetsModel.saveSettings( _doc, _this, "preset" );
	m_spreadModel.saveSettings( _doc, _this, "spread" );
	m_randomModel.saveSettings(_doc, _this, "randomness");
}




void MalletsInstrument::loadSettings( const QDomElement & _this )
{
	// ModalBar
	m_f0fixedModel.loadSettings( _this, "fixed0");
	m_f1fixedModel.loadSettings( _this, "fixed1");
	m_f2fixedModel.loadSettings( _this, "fixed2");
	m_f3fixedModel.loadSettings( _this, "fixed3");
	m_freq0Model.loadSettings( _this, "freq0" );
	m_freq1Model.loadSettings( _this, "freq1" );
	m_freq2Model.loadSettings( _this, "freq2" );
	m_freq3Model.loadSettings( _this, "freq3" );
	m_res0Model.loadSettings( _this, "res0" );
	m_res1Model.loadSettings( _this, "res1" );
	m_res2Model.loadSettings( _this, "res2" );
	m_res3Model.loadSettings( _this, "res3" );
	m_vol0Model.loadSettings( _this, "vol0" );
	m_vol1Model.loadSettings( _this, "vol1" );
	m_vol2Model.loadSettings( _this, "vol2" );
	m_vol3Model.loadSettings( _this, "vol3" );
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
	p = ps->presetIndex();
	ps->setFrequency( freq );
	ps->setFixed( 0, m_freq0Model.value(), m_res0Model.value(), m_vol0Model.value());
	ps->setFixed( 1, m_freq1Model.value(), m_res1Model.value(), m_vol1Model.value());
	ps->setFixed( 2, m_freq2Model.value(), m_res2Model.value(), m_vol2Model.value());
	ps->setFixed( 3, m_freq3Model.value(), m_res3Model.value(), m_vol3Model.value());

	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		_working_buffer[frame][0] = ps->nextSampleLeft() * 20.0f;
		_working_buffer[frame][1] = ps->nextSampleRight() * 20.0f;
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
	m_presetsCombo->setGeometry( 140, 0, 99, ComboBox::DEFAULT_HEIGHT );
	m_presetsCombo->setFont( pointSize<8>( m_presetsCombo->font() ) );
	
	connect( &_instrument->m_presetsModel, SIGNAL( dataChanged() ),
		 this, SLOT( changePreset() ) );
	connect( &_instrument->m_f0fixedModel, SIGNAL( dataChanged() ),
			this, SLOT( changeFreqModel() ));
	connect( &_instrument->m_f1fixedModel, SIGNAL( dataChanged() ),
			this, SLOT( changeFreqModel() ));
	connect( &_instrument->m_f2fixedModel, SIGNAL( dataChanged() ),
			this, SLOT( changeFreqModel() ));
	connect( &_instrument->m_f3fixedModel, SIGNAL( dataChanged() ),
			this, SLOT( changeFreqModel() ));
	connect( &_instrument->m_presetsModel, SIGNAL( dataChanged() ),
			this, SLOT( changeFreqModel() ));
	
	m_spreadKnob = new Knob( knobBright_26, this );
	m_spreadKnob->setLabel( tr( "Spread" ) );
	m_spreadKnob->move( 180, 155 );
	m_spreadKnob->setHintText( tr( "Spread:" ), "" );

	m_randomKnob = new Knob(knobBright_26, this);
	m_randomKnob->setLabel(tr("Rand"));
	m_randomKnob->move(180, 195);
	m_randomKnob->setHintText(tr("Random:"),"");

	modelChanged();

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

	m_f0fixedLED = new LedCheckBox( tr( "F0" ), widget );
	m_f0fixedLED->move( 10, 20 );
	m_f1fixedLED = new LedCheckBox( tr( "F1" ), widget );
	m_f1fixedLED->move( 60, 20 );
	m_f2fixedLED = new LedCheckBox( tr( "F2" ), widget );
	m_f2fixedLED->move( 110, 20 );
	m_f3fixedLED = new LedCheckBox( tr( "F3" ), widget );
	m_f3fixedLED->move( 160, 20 );
	m_freq0Knob	= new Knob( knobBright_26, widget );
	m_freq0Knob->setLabel( tr( "Freq 0" ) );
	m_freq0Knob->move( 30, 35 );
	m_freq0Knob->setHintText( tr( "Freq 0:" ), "" );

	m_freq1Knob	= new Knob( knobBright_26, widget );
	m_freq1Knob->setLabel( tr( "Freq 1" ) );
	m_freq1Knob->move( 80, 35 );
	m_freq1Knob->setHintText( tr( "Freq 1:" ), "" );

	m_freq2Knob	= new Knob( knobBright_26, widget );
	m_freq2Knob->setLabel( tr( "Freq 2" ) );
	m_freq2Knob->move( 130, 35 );
	m_freq2Knob->setHintText( tr( "Freq 2:" ), "" );

	m_freq3Knob	= new Knob( knobBright_26, widget );
	m_freq3Knob->setLabel( tr( "Freq 3" ) );
	m_freq3Knob->move( 180, 35 );
	m_freq3Knob->setHintText( tr( "Freq3:" ), "" );

	m_res0Knob	= new Knob( knobBright_26, widget );
	m_res0Knob->setLabel( tr( "Res 0" ) );
	m_res0Knob->move( 30, 75 );
	m_res0Knob->setHintText( tr( "Res 0:" ), "" );

	m_res1Knob	= new Knob( knobBright_26, widget );
	m_res1Knob->setLabel( tr( "Res 1" ) );
	m_res1Knob->move( 80, 75 );
	m_res1Knob->setHintText( tr( "Res 1:" ), "" );

	m_res2Knob	= new Knob( knobBright_26, widget );
	m_res2Knob->setLabel( tr( "Res 2" ) );
	m_res2Knob->move( 130, 75 );
	m_res2Knob->setHintText( tr( "Res 2:" ), "" );

	m_res3Knob	= new Knob( knobBright_26, widget );
	m_res3Knob->setLabel( tr( "Res 3" ) );
	m_res3Knob->move( 180, 75 );
	m_res3Knob->setHintText( tr( "Res 3:" ), "" );
		
	m_vol0Knob	= new Knob( knobBright_26, widget );
	m_vol0Knob->setLabel( tr( "Vol 0" ) );
	m_vol0Knob->move( 30, 115 );
	m_vol0Knob->setHintText( tr( "Vol 0:" ), "" );

	m_vol1Knob	= new Knob( knobBright_26, widget );
	m_vol1Knob->setLabel( tr( "Vol 1" ) );
	m_vol1Knob->move( 80, 115 );
	m_vol1Knob->setHintText( tr( "Vol 1:" ), "" );

	m_vol2Knob	= new Knob( knobBright_26, widget );
	m_vol2Knob->setLabel( tr( "Vol 2" ) );
	m_vol2Knob->move( 130, 115 );
	m_vol2Knob->setHintText( tr( "Vol 2:" ), "" );

	m_vol3Knob	= new Knob( knobBright_26, widget );
	m_vol3Knob->setLabel( tr( "Vol 3" ) );
	m_vol3Knob->move( 180, 115 );
	m_vol3Knob->setHintText( tr( "Vol 3:" ), "" );

	m_hardnessKnob = new Knob( knobBright_26, widget );
	m_hardnessKnob->setLabel( tr( "Hard" ) );
	m_hardnessKnob->move( 30, 155 );
	m_hardnessKnob->setHintText( tr( "Hardness:" ), "" );

	m_positionKnob = new Knob( knobBright_26, widget );
	m_positionKnob->setLabel( tr( "Pos" ) );
	m_positionKnob->move( 80, 155 );
	m_positionKnob->setHintText( tr( "Position:" ), "" );

	m_stickKnob = new Knob( knobBright_26, widget );
	m_stickKnob->setLabel( tr( "Stick" ) );
	m_stickKnob->move( 130, 155 );
	m_stickKnob->setHintText( tr( "Stick mix:" ), "" );

	m_vibratoGainKnob = new Knob( knobBright_26, widget );
	m_vibratoGainKnob->setLabel( tr( "Gain" ) );
	m_vibratoGainKnob->move( 30, 195 );
	m_vibratoGainKnob->setHintText( tr( "Vibrato gain:" ), "" );

	m_vibratoFreqKnob = new Knob( knobBright_26, widget );
	m_vibratoFreqKnob->setLabel( tr( "Freq" ) );
	m_vibratoFreqKnob->move( 80, 195 );
	m_vibratoFreqKnob->setHintText( tr( "Vibrato frequency:" ), "" );

	QLabel* vibratoLabel = new QLabel( tr( "Vibrato" ), widget );
	vibratoLabel->setFont( pointSize<8>( vibratoLabel->font() ) );
	vibratoLabel->move(50, 230);

	return( widget );
}




void MalletsInstrumentView::modelChanged()
{
	MalletsInstrument * inst = castModel<MalletsInstrument>();
	m_f0fixedLED->setModel( &inst->m_f0fixedModel );
	m_f1fixedLED->setModel( &inst->m_f1fixedModel );
	m_f2fixedLED->setModel( &inst->m_f2fixedModel );
	m_f3fixedLED->setModel( &inst->m_f3fixedModel );
/*	m_freq0Knob->setModel( &inst->m_freq0Model );
	m_freq1Knob->setModel( &inst->m_freq1Model );
	m_freq2Knob->setModel( &inst->m_freq2Model );
	m_freq3Knob->setModel( &inst->m_freq3Model );*/

	m_freq0Knob->setModel( &inst->m_freq0Model );
	m_freq1Knob->setModel( &inst->m_freq1Model );
	m_freq2Knob->setModel( &inst->m_freq2Model );
	m_freq3Knob->setModel( &inst->m_freq3Model );

	m_res0Knob->setModel( &inst->m_res0Model );
	m_res1Knob->setModel( &inst->m_res1Model );
	m_res2Knob->setModel( &inst->m_res2Model );
	m_res3Knob->setModel( &inst->m_res3Model );
	m_vol0Knob->setModel( &inst->m_vol0Model );
	m_vol1Knob->setModel( &inst->m_vol1Model );
	m_vol2Knob->setModel( &inst->m_vol2Model );
	m_vol3Knob->setModel( &inst->m_vol3Model );
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
	MalletsInstrument * inst = castModel<MalletsInstrument>();

	int p = inst->m_presetsModel.value();
	if(MalletsInstrument::m_presets[p][0][0] < 0.0f)
	{
		inst->m_f0fixedModel.setValue(true);
		inst->m_freq0Model.setRange(0.0f, 10000.0f, 0.1f);
	}
	else
	{
		inst->m_f0fixedModel.setValue(false);
		inst->m_freq0Model.setRange(0.0f, 20.0f, 0.01f);
	}

	if(MalletsInstrument::m_presets[p][0][1] < 0.0f)
	{
		inst->m_f1fixedModel.setValue(true);
		inst->m_freq1Model.setRange(0.0f, 10000.0f, 0.1f);
	}
	else
	{
		inst->m_f1fixedModel.setValue(false);
		inst->m_freq1Model.setRange(0.0f, 20.0f, 0.01f);
	}

	if(MalletsInstrument::m_presets[p][0][2] < 0.0f)
	{
		inst->m_f2fixedModel.setValue(true);
		inst->m_freq2Model.setRange(0.0f, 10000.0f, 0.1f);
	}
	else
	{
		inst->m_f2fixedModel.setValue(false);
		inst->m_freq2Model.setRange(0.0f, 20.0f, 0.01f);
	}

	if(MalletsInstrument::m_presets[p][0][3] < 0.0f)
	{
		inst->m_f3fixedModel.setValue(true);
		inst->m_freq3Model.setRange(0.0f, 10000.0f, 0.1f);
	}
	else
	{
		inst->m_f3fixedModel.setValue(false);
		inst->m_freq3Model.setRange(0.0f, 20.0f, 0.01f);
	}

	inst->m_freq0Model.setValue(std::fabs(MalletsInstrument::m_presets[p][0][0]));
	inst->m_freq1Model.setValue(std::fabs(MalletsInstrument::m_presets[p][0][1]));
	inst->m_freq2Model.setValue(std::fabs(MalletsInstrument::m_presets[p][0][2]));
	inst->m_freq3Model.setValue(std::fabs(MalletsInstrument::m_presets[p][0][3]));
	inst->m_res0Model.setValue(MalletsInstrument::m_presets[p][1][0]);
	inst->m_res1Model.setValue(MalletsInstrument::m_presets[p][1][1]);
	inst->m_res2Model.setValue(MalletsInstrument::m_presets[p][1][2]);
	inst->m_res3Model.setValue(MalletsInstrument::m_presets[p][1][3]);
	inst->m_vol0Model.setValue(MalletsInstrument::m_presets[p][2][0]);
	inst->m_vol1Model.setValue(MalletsInstrument::m_presets[p][2][1]);
	inst->m_vol2Model.setValue(MalletsInstrument::m_presets[p][2][2]);
	inst->m_vol3Model.setValue(MalletsInstrument::m_presets[p][2][3]);
	inst->m_hardnessModel.setValue(MalletsInstrument::m_presets[p][3][0]);
	inst->m_positionModel.setValue(MalletsInstrument::m_presets[p][3][1]);
	inst->m_stickModel.setValue(MalletsInstrument::m_presets[p][3][2]);
	m_modalBarWidget->show();
}




void MalletsInstrumentView::changeFreqModel()
{
	MalletsInstrument * inst = castModel<MalletsInstrument>();
	inst->m_f0fixedModel.value() ? inst->m_freq0Model.setRange(0.0f, 10000.0f, 0.1f) : inst->m_freq0Model.setRange(0.0f, 20.0f, 0.01f);
	inst->m_f1fixedModel.value() ? inst->m_freq1Model.setRange(0.0f, 10000.0f, 0.1f) : inst->m_freq1Model.setRange(0.0f, 20.0f, 0.01f);
	inst->m_f2fixedModel.value() ? inst->m_freq2Model.setRange(0.0f, 10000.0f, 0.1f) : inst->m_freq2Model.setRange(0.0f, 20.0f, 0.01f);
	inst->m_f3fixedModel.value() ? inst->m_freq3Model.setRange(0.0f, 10000.0f, 0.1f) : inst->m_freq3Model.setRange(0.0f, 20.0f, 0.01f);
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
		//m_voice->controlChange( 128, 128.0f );

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
