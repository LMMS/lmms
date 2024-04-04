/*
 * EnvelopeAndLfoView.cpp - widget which is m_used by envelope/lfo/filter-
 *                          tab of instrument track window
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "EnvelopeAndLfoView.h"

#include "EnvelopeGraph.h"
#include "LfoGraph.h"
#include "EnvelopeAndLfoParameters.h"
#include "SampleLoader.h"
#include "gui_templates.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "DataFile.h"
#include "PixmapButton.h"
#include "StringPairDrag.h"
#include "TempoSyncKnob.h"
#include "TextFloat.h"
#include "Track.h"

namespace lmms
{

namespace gui
{

const int ENV_GRAPH_X = 6;
const int ENV_GRAPH_Y = 6;

const int ENV_KNOBS_Y = 43;
const int ENV_KNOBS_LBL_Y = ENV_KNOBS_Y+35;
const int KNOB_X_SPACING = 32;
const int PREDELAY_KNOB_X = 6;
const int ATTACK_KNOB_X = PREDELAY_KNOB_X+KNOB_X_SPACING;
const int HOLD_KNOB_X = ATTACK_KNOB_X+KNOB_X_SPACING;
const int DECAY_KNOB_X = HOLD_KNOB_X+KNOB_X_SPACING;
const int SUSTAIN_KNOB_X = DECAY_KNOB_X+KNOB_X_SPACING;
const int RELEASE_KNOB_X = SUSTAIN_KNOB_X+KNOB_X_SPACING;
const int AMOUNT_KNOB_X = RELEASE_KNOB_X+KNOB_X_SPACING;

const int LFO_GRAPH_X = 6;
const int LFO_GRAPH_Y = ENV_KNOBS_LBL_Y+14;
const int LFO_KNOB_Y = LFO_GRAPH_Y-2;
const int LFO_PREDELAY_KNOB_X = LFO_GRAPH_X + 100;
const int LFO_ATTACK_KNOB_X = LFO_PREDELAY_KNOB_X+KNOB_X_SPACING;
const int LFO_SPEED_KNOB_X = LFO_ATTACK_KNOB_X+KNOB_X_SPACING;
const int LFO_AMOUNT_KNOB_X = LFO_SPEED_KNOB_X+KNOB_X_SPACING;
const int LFO_SHAPES_X = LFO_GRAPH_X;//PREDELAY_KNOB_X;
const int LFO_SHAPES_Y = LFO_GRAPH_Y + 50;

EnvelopeAndLfoView::EnvelopeAndLfoView( QWidget * _parent ) :
	QWidget( _parent ),
	ModelView( nullptr, this ),
	m_params( nullptr )
{
	m_envelopeGraph = new EnvelopeGraph(this);
	m_envelopeGraph->move(ENV_GRAPH_X, ENV_GRAPH_Y);

	m_predelayKnob = new Knob( KnobType::Bright26, this );
	m_predelayKnob->setLabel( tr( "DEL" ) );
	m_predelayKnob->move( PREDELAY_KNOB_X, ENV_KNOBS_Y );
	m_predelayKnob->setHintText( tr( "Pre-delay:" ), "" );


	m_attackKnob = new Knob( KnobType::Bright26, this );
	m_attackKnob->setLabel( tr( "ATT" ) );
	m_attackKnob->move( ATTACK_KNOB_X, ENV_KNOBS_Y );
	m_attackKnob->setHintText( tr( "Attack:" ), "" );


	m_holdKnob = new Knob( KnobType::Bright26, this );
	m_holdKnob->setLabel( tr( "HOLD" ) );
	m_holdKnob->move( HOLD_KNOB_X, ENV_KNOBS_Y );
	m_holdKnob->setHintText( tr( "Hold:" ), "" );


	m_decayKnob = new Knob( KnobType::Bright26, this );
	m_decayKnob->setLabel( tr( "DEC" ) );
	m_decayKnob->move( DECAY_KNOB_X, ENV_KNOBS_Y );
	m_decayKnob->setHintText( tr( "Decay:" ), "" );


	m_sustainKnob = new Knob( KnobType::Bright26, this );
	m_sustainKnob->setLabel( tr( "SUST" ) );
	m_sustainKnob->move( SUSTAIN_KNOB_X, ENV_KNOBS_Y );
	m_sustainKnob->setHintText( tr( "Sustain:" ), "" );


	m_releaseKnob = new Knob( KnobType::Bright26, this );
	m_releaseKnob->setLabel( tr( "REL" ) );
	m_releaseKnob->move( RELEASE_KNOB_X, ENV_KNOBS_Y );
    m_releaseKnob->setHintText( tr( "Release:" ), "" );


	m_amountKnob = new Knob( KnobType::Bright26, this );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->move( AMOUNT_KNOB_X, ENV_GRAPH_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ), "" );



	m_lfoGraph = new LfoGraph(this);
	m_lfoGraph->move(LFO_GRAPH_X, LFO_GRAPH_Y);

	m_lfoPredelayKnob = new Knob( KnobType::Bright26, this );
	m_lfoPredelayKnob->setLabel( tr( "DEL" ) );
	m_lfoPredelayKnob->move( LFO_PREDELAY_KNOB_X, LFO_KNOB_Y );
	m_lfoPredelayKnob->setHintText( tr( "Pre-delay:" ), "" );


	m_lfoAttackKnob = new Knob( KnobType::Bright26, this );
	m_lfoAttackKnob->setLabel( tr( "ATT" ) );
	m_lfoAttackKnob->move( LFO_ATTACK_KNOB_X, LFO_KNOB_Y );
	m_lfoAttackKnob->setHintText( tr( "Attack:" ), "" );


	m_lfoSpeedKnob = new TempoSyncKnob( KnobType::Bright26, this );
	m_lfoSpeedKnob->setLabel( tr( "SPD" ) );
	m_lfoSpeedKnob->move( LFO_SPEED_KNOB_X, LFO_KNOB_Y );
	m_lfoSpeedKnob->setHintText( tr( "Frequency:" ), "" );


	m_lfoAmountKnob = new Knob( KnobType::Bright26, this );
	m_lfoAmountKnob->setLabel( tr( "AMT" ) );
	m_lfoAmountKnob->move( LFO_AMOUNT_KNOB_X, LFO_KNOB_Y );
	m_lfoAmountKnob->setHintText( tr( "Modulation amount:" ), "" );

	auto sin_lfo_btn = new PixmapButton(this, nullptr);
	sin_lfo_btn->move( LFO_SHAPES_X, LFO_SHAPES_Y );
	sin_lfo_btn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
	sin_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );

	auto triangle_lfo_btn = new PixmapButton(this, nullptr);
	triangle_lfo_btn->move( LFO_SHAPES_X+15, LFO_SHAPES_Y );
	triangle_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"triangle_wave_active" ) );
	triangle_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"triangle_wave_inactive" ) );

	auto saw_lfo_btn = new PixmapButton(this, nullptr);
	saw_lfo_btn->move( LFO_SHAPES_X+30, LFO_SHAPES_Y );
	saw_lfo_btn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
	saw_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );

	auto sqr_lfo_btn = new PixmapButton(this, nullptr);
	sqr_lfo_btn->move( LFO_SHAPES_X+45, LFO_SHAPES_Y );
	sqr_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
	sqr_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );

	m_userLfoBtn = new PixmapButton( this, nullptr );
	m_userLfoBtn->move( LFO_SHAPES_X+75, LFO_SHAPES_Y );
	m_userLfoBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
	m_userLfoBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );

	connect( m_userLfoBtn, SIGNAL(toggled(bool)),
				this, SLOT(lfoUserWaveChanged()));

	auto random_lfo_btn = new PixmapButton(this, nullptr);
	random_lfo_btn->move( LFO_SHAPES_X+60, LFO_SHAPES_Y );
	random_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"random_wave_active" ) );
	random_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"random_wave_inactive" ) );

	m_lfoWaveBtnGrp = new automatableButtonGroup( this );
	m_lfoWaveBtnGrp->addButton( sin_lfo_btn );
	m_lfoWaveBtnGrp->addButton( triangle_lfo_btn );
	m_lfoWaveBtnGrp->addButton( saw_lfo_btn );
	m_lfoWaveBtnGrp->addButton( sqr_lfo_btn );
	m_lfoWaveBtnGrp->addButton( m_userLfoBtn );
	m_lfoWaveBtnGrp->addButton( random_lfo_btn );

	m_x100Cb = new LedCheckBox( tr( "FREQ x 100" ), this );
	m_x100Cb->setFont(pointSize(m_x100Cb->font(), 6.5));
	m_x100Cb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 36 );
	m_x100Cb->setToolTip(tr("Multiply LFO frequency by 100"));


	m_controlEnvAmountCb = new LedCheckBox( tr( "MODULATE ENV AMOUNT" ),
			this );
	m_controlEnvAmountCb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 54 );
	m_controlEnvAmountCb->setFont(pointSize(m_controlEnvAmountCb->font(), 6.5));
	m_controlEnvAmountCb->setToolTip(
				tr( "Control envelope amount by this LFO" ) );


	setAcceptDrops( true );
}




EnvelopeAndLfoView::~EnvelopeAndLfoView()
{
	delete m_lfoWaveBtnGrp;
}




void EnvelopeAndLfoView::modelChanged()
{
	m_params = castModel<EnvelopeAndLfoParameters>();
	m_envelopeGraph->setModel(m_params);
	m_predelayKnob->setModel( &m_params->m_predelayModel );
	m_attackKnob->setModel( &m_params->m_attackModel );
	m_holdKnob->setModel( &m_params->m_holdModel );
	m_decayKnob->setModel( &m_params->m_decayModel );
	m_sustainKnob->setModel( &m_params->m_sustainModel );
	m_releaseKnob->setModel( &m_params->m_releaseModel );
	m_amountKnob->setModel( &m_params->m_amountModel );

	m_lfoGraph->setModel(m_params);
	m_lfoPredelayKnob->setModel( &m_params->m_lfoPredelayModel );
	m_lfoAttackKnob->setModel( &m_params->m_lfoAttackModel );
	m_lfoSpeedKnob->setModel( &m_params->m_lfoSpeedModel );
	m_lfoAmountKnob->setModel( &m_params->m_lfoAmountModel );
	m_lfoWaveBtnGrp->setModel( &m_params->m_lfoWaveModel );
	m_x100Cb->setModel( &m_params->m_x100Model );
	m_controlEnvAmountCb->setModel( &m_params->m_controlEnvAmountModel );
}




void EnvelopeAndLfoView::dragEnterEvent( QDragEnterEvent * _dee )
{
	StringPairDrag::processDragEnterEvent( _dee,
					QString( "samplefile,clip_%1" ).arg(
							static_cast<int>(Track::Type::Sample) ) );
}




void EnvelopeAndLfoView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		m_params->m_userWave = SampleLoader::createBufferFromFile(value);
		m_userLfoBtn->model()->setValue( true );
		m_params->m_lfoWaveModel.setValue(static_cast<int>(EnvelopeAndLfoParameters::LfoShape::UserDefinedWave));
		_de->accept();
		update();
	}
	else if( type == QString( "clip_%1" ).arg( static_cast<int>(Track::Type::Sample) ) )
	{
		DataFile dataFile( value.toUtf8() );
		auto file = dataFile.content().
					firstChildElement().firstChildElement().
					firstChildElement().attribute("src");
		m_params->m_userWave = SampleLoader::createBufferFromFile(file);
		m_userLfoBtn->model()->setValue( true );
		m_params->m_lfoWaveModel.setValue(static_cast<int>(EnvelopeAndLfoParameters::LfoShape::UserDefinedWave));
		_de->accept();
		update();
	}
}




void EnvelopeAndLfoView::lfoUserWaveChanged()
{
	if( static_cast<EnvelopeAndLfoParameters::LfoShape>(m_params->m_lfoWaveModel.value()) ==
				EnvelopeAndLfoParameters::LfoShape::UserDefinedWave )
	{
		if (m_params->m_userWave->size() <= 1)
		{
			TextFloat::displayMessage( tr( "Hint" ),
				tr( "Drag and drop a sample into this window." ),
					embed::getIconPixmap( "hint" ), 3000 );
		}
	}
}

} // namespace gui

} // namespace lmms
