#ifndef SINGLE_SOURCE_COMPILE

/*
 * envelope_and_lfo_widget.cpp - widget which is m_used by envelope/lfo/filter-
 *                               tab of channel-window
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "envelope_and_lfo_widget.h"


#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtXml/QDomElement>


#include "debug.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "knob.h"
#include "led_checkbox.h"
#include "mmp.h"
#include "oscillator.h"
#include "pixmap_button.h"
#include "song_editor.h"
#include "string_pair_drag.h"
#include "tempo_sync_knob.h"
#include "text_float.h"
#include "tooltip.h"
#include "automatable_model_templates.h"



// how long should be each envelope-segment maximal (e.g. attack)?
const float SECS_PER_ENV_SEGMENT = 5.0f;
// how long should be one LFO-oscillation maximal?
const float SECS_PER_LFO_OSCILLATION = 20.0f;


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

const float TIME_UNIT_WIDTH = 36.0;


const int LFO_GRAPH_X = 6;
const int LFO_GRAPH_Y = ENV_KNOBS_LBL_Y+14;
const int LFO_KNOB_Y = LFO_GRAPH_Y-2;
const int LFO_PREDELAY_KNOB_X = LFO_GRAPH_X + 100;
const int LFO_ATTACK_KNOB_X = LFO_PREDELAY_KNOB_X+KNOB_X_SPACING;
const int LFO_SPEED_KNOB_X = LFO_ATTACK_KNOB_X+KNOB_X_SPACING;
const int LFO_AMOUNT_KNOB_X = LFO_SPEED_KNOB_X+KNOB_X_SPACING;
const int LFO_SHAPES_X = LFO_GRAPH_X;//PREDELAY_KNOB_X;
const int LFO_SHAPES_Y = LFO_GRAPH_Y + 50;


QPixmap * envelopeAndLFOWidget::s_envGraph = NULL;
QPixmap * envelopeAndLFOWidget::s_lfoGraph = NULL;

QVector<envelopeAndLFOWidget *> envelopeAndLFOWidget::s_EaLWidgets;



envelopeAndLFOWidget::envelopeAndLFOWidget( float _value_for_zero_amount,
							QWidget * _parent,
							track * _track ) :
	QWidget( _parent ),
	m_used( FALSE ),
	m_predelayModel(),
	m_attackModel(),
	m_holdModel(),
	m_decayModel(),
	m_sustainModel(),
	m_releaseModel(),
	m_amountModel(),
	m_lfoPredelayModel(),
	m_lfoAttackModel(),
	m_lfoSpeedModel(),
	m_lfoAmountModel(),
	m_lfoWaveModel(),
	m_x100Model( FALSE, FALSE, TRUE ),
	m_controlEnvAmountModel( FALSE, FALSE, TRUE ),
	m_valueForZeroAmount( _value_for_zero_amount ),
	m_pahdEnv( NULL ),
	m_rEnv( NULL ),
	m_lfoFrame( 0 ),
	m_lfoAmountIsZero( FALSE ),
	m_lfoShapeData( NULL )
{
	if( s_envGraph == NULL )
	{
		s_envGraph = new QPixmap( embed::getIconPixmap(
							"envelope_graph" ) );
	}
	if( s_lfoGraph == NULL )
	{
		s_lfoGraph = new QPixmap( embed::getIconPixmap( "lfo_graph" ) );
	}

	s_EaLWidgets.push_back( this );

	m_predelayModel.setTrack( _track );
	m_predelayModel.setRange( 0.0, 1.0, 0.001 );
	m_predelayModel.setInitValue( 0.0 );
	m_predelayKnob = new knob( knobBright_26, this, tr( "Predelay-time" ) );
	m_predelayKnob->setModel( &m_predelayModel );
	m_predelayKnob->setLabel( tr( "DEL" ) );
	m_predelayKnob->move( PREDELAY_KNOB_X, ENV_KNOBS_Y );
	m_predelayKnob->setHintText( tr( "Predelay:" ) + " ", "" );
	m_predelayKnob->setWhatsThis(
		tr( "Use this knob for setting predelay of the current "
			"envelope. The bigger this value the longer the time "
			"before start of actual envelope." ) );
	connect( &m_predelayModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );


	m_attackModel.setTrack( _track );
	m_attackModel.setRange( 0.0, 1.0, 0.001 );
	m_attackModel.setInitValue( 0.0 );
	m_attackKnob = new knob( knobBright_26, this, tr( "Attack-time" ) );
	m_attackKnob->setModel( &m_attackModel );
	m_attackKnob->setLabel( tr( "ATT" ) );
	m_attackKnob->move( ATTACK_KNOB_X, ENV_KNOBS_Y );
	m_attackKnob->setHintText( tr( "Attack:" )+" ", "" );
	m_attackKnob->setWhatsThis(
		tr( "Use this knob for setting attack-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to increase to attack-level. "
			"Choose a small value for instruments like pianos "
			"and a big value for strings." ) );
	connect( &m_attackModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );

	m_holdModel.setTrack( _track );
	m_holdModel.setRange( 0.0, 1.0, 0.001 );
	m_holdModel.setInitValue( 0.5 );
	m_holdKnob = new knob( knobBright_26, this, tr( "Hold-time" ) );
	m_holdKnob->setModel( &m_holdModel );
	m_holdKnob->setLabel( tr( "HOLD" ) );
	m_holdKnob->move( HOLD_KNOB_X, ENV_KNOBS_Y );
	m_holdKnob->setHintText( tr( "Hold:" ) + " ", "" );
	m_holdKnob->setWhatsThis(
		tr( "Use this knob for setting hold-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope holds attack-level before it begins to "
			"decrease to sustain-level." ) );
	connect( &m_holdModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );


	m_decayModel.setTrack( _track );
	m_decayModel.setRange( 0.0, 1.0, 0.001 );
	m_decayModel.setInitValue( 0.5 );
	m_decayKnob = new knob( knobBright_26, this, tr( "Decay-time" ) );
	m_decayKnob->setModel( &m_decayModel );
	m_decayKnob->setLabel( tr( "DEC" ) );
	m_decayKnob->move( DECAY_KNOB_X, ENV_KNOBS_Y );
	m_decayKnob->setHintText( tr( "Decay:" ) + " ", "" );
	m_decayKnob->setWhatsThis(
		tr( "Use this knob for setting decay-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to decrease from attack-level to "
			"sustain-level. Choose a small value for instruments "
			"like pianos." ) );
	connect( &m_decayModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );


	m_sustainModel.setTrack( _track );
	m_sustainModel.setRange( 0.0, 1.0, 0.001 );
	m_sustainModel.setInitValue( 0.5 );
	m_sustainKnob = new knob( knobBright_26, this, tr( "Sustain-level" ) );
	m_sustainKnob->setModel( &m_sustainModel );
	m_sustainKnob->setLabel( tr( "SUST" ) );
	m_sustainKnob->move( SUSTAIN_KNOB_X, ENV_KNOBS_Y );
	m_sustainKnob->setHintText( tr( "Sustain:" ) + " ", "" );
	m_sustainKnob->setWhatsThis(
		tr( "Use this knob for setting sustain-level of the current "
			"envelope. The bigger this value the higher the level "
			"on which the envelope stays before going down to "
			"zero." ) );
	connect( &m_sustainModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );



	m_releaseModel.setTrack( _track );
	m_releaseModel.setRange( 0.0, 1.0, 0.001 );
	m_releaseModel.setInitValue( 0.1 );
	m_releaseKnob = new knob( knobBright_26, this, tr( "Release-time" ) );
	m_releaseKnob->setModel( &m_releaseModel );
	m_releaseKnob->setLabel( tr( "REL" ) );
	m_releaseKnob->move( RELEASE_KNOB_X, ENV_KNOBS_Y );
	m_releaseKnob->setHintText( tr( "Release:" ) + " ", "" );
	m_releaseKnob->setWhatsThis(
		tr( "Use this knob for setting release-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to decrease from sustain-level to "
			"zero. Choose a big value for soft instruments like "
			"strings." ) );
	connect( &m_releaseModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );


	m_amountModel.setTrack( _track );
	m_amountModel.setRange( -1.0, 1.0, 0.005 );
	m_amountModel.setInitValue( 0.0 );
	m_amountKnob = new knob( knobBright_26, this,
						tr( "Modulation amount" ) );
	m_amountKnob->setModel( &m_amountModel );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->move( AMOUNT_KNOB_X, ENV_GRAPH_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
	m_amountKnob->setWhatsThis(
		tr( "Use this knob for setting modulation amount of the "
			"current envelope. The bigger this value the more the "
			"according size (e.g. volume or cutoff-frequency) "
			"will be influenced by this envelope." ) );
	connect( &m_amountModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );




	m_lfoPredelayModel.setTrack( _track );
	m_lfoPredelayModel.setRange( 0.0, 1.0, 0.001 );
	m_lfoPredelayModel.setInitValue( 0.0 );
	m_lfoPredelayKnob = new knob( knobBright_26, this,
						tr( "LFO-predelay-time" ) );
	m_lfoPredelayKnob->setModel( &m_lfoPredelayModel );
	m_lfoPredelayKnob->setLabel( tr( "DEL" ) );
	m_lfoPredelayKnob->move( LFO_PREDELAY_KNOB_X, LFO_KNOB_Y );
	m_lfoPredelayKnob->setHintText( tr( "LFO-predelay:" ) + " ", "" );
	m_lfoPredelayKnob->setWhatsThis(
		tr( "Use this knob for setting predelay-time of the current "
			"LFO. The bigger this value the the time until the "
			"LFO starts to oscillate." ) );
	connect( &m_lfoPredelayModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );


	m_lfoAttackModel.setTrack( _track );
	m_lfoAttackModel.setRange( 0.0, 1.0, 0.001 );
	m_lfoAttackModel.setInitValue( 0.0 );
	m_lfoAttackKnob = new knob( knobBright_26, this,
						tr( "LFO-attack-time" ) );
	m_lfoAttackKnob->setModel( &m_lfoAttackModel );
	m_lfoAttackKnob->setLabel( tr( "ATT" ) );
	m_lfoAttackKnob->move( LFO_ATTACK_KNOB_X, LFO_KNOB_Y );
	m_lfoAttackKnob->setHintText( tr( "LFO-attack:" ) + " ", "" );
	m_lfoAttackKnob->setWhatsThis(
		tr( "Use this knob for setting attack-time of the current LFO. "
			"The bigger this value the longer the LFO needs to "
			"increase its amplitude to maximum." ) );
	connect( &m_lfoAttackModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );


	m_lfoSpeedModel.setTrack( _track );
	m_lfoSpeedModel.setRange( 0.01, 1.0, 0.0001 );
	m_lfoSpeedModel.setInitValue( 0.1 );
	m_lfoSpeedKnob = new tempoSyncKnob( knobBright_26, this,
						tr( "LFO-speed" ), 20000.0 );
	m_lfoSpeedKnob->setModel( &m_lfoSpeedModel );
	m_lfoSpeedKnob->setLabel( tr( "SPD" ) );
	m_lfoSpeedKnob->move( LFO_SPEED_KNOB_X, LFO_KNOB_Y );
	m_lfoSpeedKnob->setHintText( tr( "LFO-speed:" ) + " ", "" );
	m_lfoSpeedKnob->setWhatsThis(
		tr( "Use this knob for setting speed of the current LFO. The "
			"bigger this value the faster the LFO oscillates and "
			"the faster will be your effect." ) );
	connect( &m_lfoSpeedModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );


	m_lfoAmountModel.setTrack( _track );
	m_lfoAmountModel.setRange( -1.0, 1.0, 0.005 );
	m_lfoAmountModel.setInitValue( 0.0 );
	m_lfoAmountKnob = new knob( knobBright_26, this,
						tr( "LFO-modulation-amount" ) );
	m_lfoAmountKnob->setModel( &m_lfoAmountModel );
	m_lfoAmountKnob->setLabel( tr( "AMT" ) );
	m_lfoAmountKnob->move( LFO_AMOUNT_KNOB_X, LFO_KNOB_Y );
	m_lfoAmountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
	m_lfoAmountKnob->setWhatsThis(
		tr( "Use this knob for setting modulation amount of the "
			"current LFO. The bigger this value the more the "
			"selected size (e.g. volume or cutoff-frequency) will "
			"be influenced by this LFO." ) );
	connect( &m_lfoAmountModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );


	pixmapButton * sin_lfo_btn = new pixmapButton( this, NULL );
	sin_lfo_btn->move( LFO_SHAPES_X, LFO_SHAPES_Y );
	sin_lfo_btn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
	sin_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );
	sin_lfo_btn->setWhatsThis(
		tr( "Click here if you want a sine-wave for current "
							"oscillator." ) );

	pixmapButton * triangle_lfo_btn = new pixmapButton( this, NULL );
	triangle_lfo_btn->move( LFO_SHAPES_X+15, LFO_SHAPES_Y );
	triangle_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"triangle_wave_active" ) );
	triangle_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"triangle_wave_inactive" ) );
	triangle_lfo_btn->setWhatsThis(
		tr( "Click here if you want a triangle-wave for current "
							"oscillator." ) );

	pixmapButton * saw_lfo_btn = new pixmapButton( this, NULL );
	saw_lfo_btn->move( LFO_SHAPES_X+30, LFO_SHAPES_Y );
	saw_lfo_btn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
	saw_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );
	saw_lfo_btn->setWhatsThis(
		tr( "Click here if you want a saw-wave for current "
							"oscillator." ) );

	pixmapButton * sqr_lfo_btn = new pixmapButton( this, NULL );
	sqr_lfo_btn->move( LFO_SHAPES_X+45, LFO_SHAPES_Y );
	sqr_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
	sqr_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );
	sqr_lfo_btn->setWhatsThis(
		tr( "Click here if you want a square-wave for current "
							"oscillator." ) );

	m_userLfoBtn = new pixmapButton( this, NULL );
	m_userLfoBtn->move( LFO_SHAPES_X+60, LFO_SHAPES_Y );
	m_userLfoBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
	m_userLfoBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
	m_userLfoBtn->setWhatsThis(
		tr( "Click here if you want a user-defined wave for current "
			"oscillator. Afterwards drag an according sample-"
			"file into LFO-graph." ) );

	connect( m_userLfoBtn, SIGNAL( toggled( bool ) ),
				this, SLOT( lfoUserWaveChanged() ) );

	m_lfoWaveBtnGrp = new automatableButtonGroup( this,
						tr( "LFO wave shape" ) );
	m_lfoWaveBtnGrp->setModel( &m_lfoWaveModel );
	m_lfoWaveBtnGrp->addButton( sin_lfo_btn );
	m_lfoWaveBtnGrp->addButton( triangle_lfo_btn );
	m_lfoWaveBtnGrp->addButton( saw_lfo_btn );
	m_lfoWaveBtnGrp->addButton( sqr_lfo_btn );
	m_lfoWaveBtnGrp->addButton( m_userLfoBtn );

	m_lfoWaveModel.setTrack( _track );
	m_lfoWaveModel.setInitValue( SIN );

	connect( &m_lfoWaveModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );

	m_x100Model.setTrack( _track );
	m_x100Cb = new ledCheckBox( tr( "FREQ x 100" ), this,
							tr( "Freq x 100" ) );
	m_x100Cb->setModel( &m_x100Model );
	m_x100Cb->setFont( pointSize<6>( m_x100Cb->font() ) );
	m_x100Cb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 36 );
	m_x100Cb->setWhatsThis(
		tr( "Click here if the frequency of this LFO should be "
						"multiplied with 100." ) );
	toolTip::add( m_x100Cb, tr( "multiply LFO-frequency with 100" ) );
	connect( &m_x100Model, SIGNAL( dataChanged() ),
				this, SLOT( updateSampleVars() ) );


	m_controlEnvAmountModel.setTrack( _track );
	m_controlEnvAmountCb = new ledCheckBox( tr( "MODULATE ENV-AMOUNT" ),
					this, tr( "Modulate Env-Amount" ) );
	m_controlEnvAmountCb->setModel( &m_controlEnvAmountModel );
	m_controlEnvAmountCb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 54 );
	m_controlEnvAmountCb->setFont( pointSize<6>(
					m_controlEnvAmountCb->font() ) );
	m_controlEnvAmountCb ->setWhatsThis(
		tr( "Click here to make the envelope-amount controlled by this "
								"LFO." ) );
	toolTip::add( m_controlEnvAmountCb,
				tr( "control envelope-amount by this LFO" ) );


	setAcceptDrops( TRUE );

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ), this,
						SLOT( updateSampleVars() ) );

	m_lfoShapeData =
		new sample_t[engine::getMixer()->framesPerPeriod()];
	updateSampleVars();
}




envelopeAndLFOWidget::~envelopeAndLFOWidget()
{
	m_predelayModel.disconnect( this );
	m_attackModel.disconnect( this );
	m_holdModel.disconnect( this );
	m_decayModel.disconnect( this );
	m_sustainModel.disconnect( this );
	m_releaseModel.disconnect( this );
	m_amountModel.disconnect( this );
	m_lfoPredelayModel.disconnect( this );
	m_lfoAttackModel.disconnect( this );
	m_lfoSpeedModel.disconnect( this );
	m_lfoAmountModel.disconnect( this );
	m_lfoWaveModel.disconnect( this );
	m_x100Model.disconnect( this );

	delete[] m_pahdEnv;
	delete[] m_rEnv;
	delete[] m_lfoShapeData;

	QVector<envelopeAndLFOWidget *> & v = s_EaLWidgets;
	if( qFind( v.begin(), v.end(), this ) != v.end() )
	{
		v.erase( qFind( v.begin(), v.end(), this ) );
	}

	delete m_lfoWaveBtnGrp;
}




inline sample_t envelopeAndLFOWidget::lfoShapeSample( fpp_t _frame_offset )
{
	f_cnt_t frame = ( m_lfoFrame + _frame_offset ) % m_lfoOscillationFrames;
	const float phase = frame / static_cast<float>(
						m_lfoOscillationFrames );
	sample_t shape_sample;
	switch( m_lfoWaveModel.value()  )
	{
		case TRIANGLE:
			shape_sample = oscillator::triangleSample( phase );
			break;
		case SQUARE:
			shape_sample = oscillator::squareSample( phase );
			break;
		case SAW:
			shape_sample = oscillator::sawSample( phase );
			break;
		case USER:
			shape_sample = m_userWave.userWaveSample( phase );
			break;
		case SIN:
		default:
			shape_sample = oscillator::sinSample( phase );
			break;
	}
	return( shape_sample * m_lfoAmount );
}




void envelopeAndLFOWidget::updateLFOShapeData( void )
{
	const fpp_t frames = engine::getMixer()->framesPerPeriod();
	for( fpp_t offset = 0; offset < frames; ++offset )
	{
		m_lfoShapeData[offset] = lfoShapeSample( offset );
	}
	m_bad_lfoShapeData = FALSE;
}




void envelopeAndLFOWidget::triggerLFO( void )
{
	QVector<envelopeAndLFOWidget *> & v = s_EaLWidgets;
	for( QVector<envelopeAndLFOWidget *>::iterator it = v.begin();
							it != v.end(); ++it )
	{
		( *it )->m_lfoFrame +=
				engine::getMixer()->framesPerPeriod();
		( *it )->m_bad_lfoShapeData = TRUE;
	}
}




void envelopeAndLFOWidget::resetLFO( void )
{
	QVector<envelopeAndLFOWidget *> & v = s_EaLWidgets;
	for( QVector<envelopeAndLFOWidget *>::iterator it = v.begin();
							it != v.end(); ++it )
	{
		( *it )->m_lfoFrame = 0;
		( *it )->m_bad_lfoShapeData = TRUE;
	}
}




inline void FASTCALL envelopeAndLFOWidget::fillLFOLevel( float * _buf,
							f_cnt_t _frame,
							const fpp_t _frames )
{
	if( m_lfoAmountIsZero || _frame <= m_lfoPredelayFrames )
	{
		for( fpp_t offset = 0; offset < _frames; ++offset )
		{
			*_buf++ = 0.0f;
		}
		return;
	}
	_frame -= m_lfoPredelayFrames;

	if( m_bad_lfoShapeData )
	{
		updateLFOShapeData();
	}

	fpp_t offset = 0;
	for( ; offset < _frames && _frame < m_lfoAttackFrames; ++offset,
								++_frame )
	{
		*_buf++ = m_lfoShapeData[offset] * _frame / m_lfoAttackFrames;
	}
	for( ; offset < _frames; ++offset )
	{
		*_buf++ = m_lfoShapeData[offset];
	}
}




void FASTCALL envelopeAndLFOWidget::fillLevel( float * _buf, f_cnt_t _frame,
					const f_cnt_t _release_begin,
					const fpp_t _frames )
{
	if( _frame < 0 || _release_begin < 0 )
	{
		return;
	}

	fillLFOLevel( _buf, _frame, _frames );

	for( fpp_t offset = 0; offset < _frames; ++offset, ++_buf, ++_frame )
	{
		float env_level;
		if( _frame < _release_begin )
		{
			if( _frame < m_pahdFrames )
			{
				env_level = m_pahdEnv[_frame];
			}
			else
			{
				env_level = m_sustainLevel;
			}
		}
		else if( ( _frame - _release_begin ) < m_rFrames )
		{
			env_level = m_rEnv[_frame - _release_begin] *
				( ( _release_begin < m_pahdFrames ) ?
				m_pahdEnv[_release_begin] : m_sustainLevel );
		}
		else
		{
			env_level = 0.0f;
		}

		// at this point, *_buf is LFO level
		*_buf = m_controlEnvAmountModel.value() ?
			env_level * ( 0.5f + *_buf ) :
			env_level + *_buf;
	}
}




void envelopeAndLFOWidget::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	m_predelayModel.saveSettings( _doc, _parent, "pdel" );
	m_attackModel.saveSettings( _doc, _parent, "att" );
	m_holdModel.saveSettings( _doc, _parent, "hold" );
	m_decayModel.saveSettings( _doc, _parent, "dec" );
	m_sustainModel.saveSettings( _doc, _parent, "sus" );
	m_releaseModel.saveSettings( _doc, _parent, "rel" );
	m_amountModel.saveSettings( _doc, _parent, "amt" );
	m_lfoWaveModel.saveSettings( _doc, _parent, "lshp" );
	m_lfoPredelayModel.saveSettings( _doc, _parent, "lpdel" );
	m_lfoAttackModel.saveSettings( _doc, _parent, "latt" );
	m_lfoSpeedModel.saveSettings( _doc, _parent, "lspd" );
	m_lfoAmountModel.saveSettings( _doc, _parent, "lamt" );
	m_x100Model.saveSettings( _doc, _parent, "x100" );
	m_controlEnvAmountModel.saveSettings( _doc, _parent, "ctlenvamt" );
	_parent.setAttribute( "userwavefile", m_userWave.audioFile() );
}




void envelopeAndLFOWidget::loadSettings( const QDomElement & _this )
{
	m_predelayModel.loadSettings( _this, "pdel" );
	m_attackModel.loadSettings( _this, "att" );
	m_holdModel.loadSettings( _this, "hold" );
	m_decayModel.loadSettings( _this, "dec" );
	m_sustainModel.loadSettings( _this, "sus" );
	m_releaseModel.loadSettings( _this, "rel" );
	m_amountModel.loadSettings( _this, "amt" );
	m_lfoWaveModel.loadSettings( _this, "lshp" );
	m_lfoPredelayModel.loadSettings( _this, "lpdel" );
	m_lfoAttackModel.loadSettings( _this, "latt" );
	m_lfoSpeedModel.loadSettings( _this, "lspd" );
	m_lfoAmountModel.loadSettings( _this, "lamt" );
	m_x100Model.loadSettings( _this, "x100" );
	m_controlEnvAmountModel.loadSettings( _this, "ctlenvamt" );
	
	// Keep compatibility with version 2.1 file format
	if( _this.hasAttribute( "lfosyncmode" ) )
	{
		m_lfoSpeedKnob->setSyncMode(
		( tempoSyncKnob::tempoSyncMode ) _this.attribute(
						"lfosyncmode" ).toInt() );
	}
	
	m_userWave.setAudioFile( _this.attribute( "userwavefile" ) );

	updateSampleVars();
}




void envelopeAndLFOWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() != Qt::LeftButton )
	{
		return;
	}

	if( QRect( ENV_GRAPH_X, ENV_GRAPH_Y, s_envGraph->width(),
			s_envGraph->height() ).contains( _me->pos() ) == TRUE )
	{
		if( m_amountKnob->value() < 1.0f )
		{
			m_amountKnob->setValue( 1.0f );
		}
		else
		{
			m_amountKnob->setValue( 0.0f );
		}
		updateSampleVars();
	}
	else if( QRect( LFO_GRAPH_X, LFO_GRAPH_Y, s_lfoGraph->width(),
			s_lfoGraph->height() ).contains( _me->pos() ) == TRUE )
	{
		if( m_lfoAmountKnob->value() < 1.0f )
		{
			m_lfoAmountKnob->setValue( 1.0f );
		}
		else
		{
			m_lfoAmountKnob->setValue( 0.0f );
		}
		updateSampleVars();
	}
}




void envelopeAndLFOWidget::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee,
					QString( "samplefile,tco_%1" ).arg(
							track::SAMPLE_TRACK ) );
}




void envelopeAndLFOWidget::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		m_userWave.setAudioFile( stringPairDrag::decodeValue( _de ) );
		m_userLfoBtn->model()->setValue( TRUE );
		_de->accept();
	}
	else if( type == QString( "tco_%1" ).arg( track::SAMPLE_TRACK ) )
	{
		multimediaProject mmp( value, FALSE );
		m_userWave.setAudioFile( mmp.content().firstChild().toElement().
							attribute( "src" ) );
		m_userLfoBtn->model()->setValue( TRUE );
		_de->accept();
	}
}




void envelopeAndLFOWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );

	// set smaller font
	p.setFont( pointSize<6>( p.font() ) );

	// draw envelope-graph
	p.drawPixmap( ENV_GRAPH_X, ENV_GRAPH_Y, *s_envGraph );
	// draw LFO-graph
	p.drawPixmap( LFO_GRAPH_X, LFO_GRAPH_Y, *s_lfoGraph );


	p.setFont( pointSize<8>( p.font() ) );

	const float gray_amount = 1.0f - fabsf( m_amountKnob->value() );

	p.setPen( QPen( QColor( static_cast<int>( 96 * gray_amount ),
				static_cast<int>( 255 - 159 * gray_amount ),
				static_cast<int>( 128 - 32 * gray_amount ) ),
									2 ) );

	const QColor end_points_color( 0xFF, 0xBF, 0x22 );
	const QColor end_points_bg_color( 0, 0, 2 );

	const int y_base = ENV_GRAPH_Y + s_envGraph->height() - 3;
	const int avail_height = s_envGraph->height() - 6;
	
	int x1 = ENV_GRAPH_X + 2 + static_cast<int>( m_predelayKnob->value() *
							TIME_UNIT_WIDTH );
	int x2 = x1 + static_cast<int>( m_attackKnob->value() *
							TIME_UNIT_WIDTH );

	p.drawLine( x1, y_base, x2, y_base - avail_height );
	p.fillRect( x1 - 1, y_base - 2, 4, 4, end_points_bg_color );
	p.fillRect( x1, y_base - 1, 2, 2, end_points_color );
	x1 = x2;
	x2 = x1 + static_cast<int>( m_holdKnob->value() * TIME_UNIT_WIDTH );

	p.drawLine( x1, y_base - avail_height, x2, y_base - avail_height );
	p.fillRect( x1 - 1, y_base - 2 - avail_height, 4, 4,
							end_points_bg_color );
	p.fillRect( x1, y_base-1-avail_height, 2, 2, end_points_color );
	x1 = x2;
	x2 = x1 + static_cast<int>( ( m_decayKnob->value() *
						m_sustainKnob->value() ) *
							TIME_UNIT_WIDTH );

	p.drawLine( x1, y_base-avail_height, x2, static_cast<int>( y_base -
								avail_height +
				m_sustainKnob->value() * avail_height ) );
	p.fillRect( x1 - 1, y_base - 2 - avail_height, 4, 4,
							end_points_bg_color );
	p.fillRect( x1, y_base - 1 - avail_height, 2, 2, end_points_color );
	x1 = x2;
	x2 = x1 + static_cast<int>( m_releaseKnob->value() * TIME_UNIT_WIDTH );

	p.drawLine( x1, static_cast<int>( y_base - avail_height +
						m_sustainKnob->value() *
						avail_height ), x2, y_base );
	p.fillRect( x1-1, static_cast<int>( y_base - avail_height +
						m_sustainKnob->value() *
						avail_height ) - 2, 4, 4,
							end_points_bg_color );
	p.fillRect( x1, static_cast<int>( y_base - avail_height +
						m_sustainKnob->value() *
						avail_height ) - 1, 2, 2,
							end_points_color );
	p.fillRect( x2 - 1, y_base - 2, 4, 4, end_points_bg_color );
	p.fillRect( x2, y_base - 1, 2, 2, end_points_color );


	int LFO_GRAPH_W = s_lfoGraph->width() - 6;	// substract border
	int LFO_GRAPH_H = s_lfoGraph->height() - 6;	// substract border
	int graph_x_base = LFO_GRAPH_X + 3;
	int graph_y_base = LFO_GRAPH_Y + 3 + LFO_GRAPH_H / 2;

	const float frames_for_graph = SECS_PER_LFO_OSCILLATION *
					engine::getMixer()->sampleRate() / 10;

	const float lfo_gray_amount = 1.0f - fabsf( m_lfoAmountKnob->value() );
	p.setPen( QPen( QColor( static_cast<int>( 96 * lfo_gray_amount ),
				static_cast<int>( 255 - 159 * lfo_gray_amount ),
				static_cast<int>( 128 - 32 *
							lfo_gray_amount ) ),
									1.5 ) );


	float osc_frames = m_lfoOscillationFrames;

	if( m_x100Model.value() )
	{
		osc_frames *= 100.0f;
	}

	float old_y = 0;
	for( int x = 0; x <= LFO_GRAPH_W; ++x )
	{
		float val = 0.0;
		float cur_sample = x * frames_for_graph / LFO_GRAPH_W;
		if( static_cast<f_cnt_t>( cur_sample ) > m_lfoPredelayFrames )
		{
			float phase = ( cur_sample -= m_lfoPredelayFrames ) /
								osc_frames;
			switch( m_lfoWaveModel.value() )
			{
				case SIN:
					val = oscillator::sinSample( phase );
					break;
				case TRIANGLE:
					val = oscillator::triangleSample(
								phase );
					break;
				case SAW:
					val = oscillator::sawSample( phase );
					break;
				case SQUARE:
					val = oscillator::squareSample( phase );
					break;
				case USER:
					val = m_userWave.userWaveSample(
								phase );
			}
			if( static_cast<f_cnt_t>( cur_sample ) <=
							m_lfoAttackFrames )
			{
				val *= cur_sample / m_lfoAttackFrames;
			}
		}
		float cur_y = -LFO_GRAPH_H / 2.0f * val;
		p.drawLine( QLineF( graph_x_base + x - 1, graph_y_base + old_y,
						graph_x_base + x,
						graph_y_base + cur_y ) );
		old_y = cur_y;
	}

	p.setPen( QColor( 255, 192, 0 ) );
	int ms_per_osc = static_cast<int>( SECS_PER_LFO_OSCILLATION *
						m_lfoSpeedKnob->value() *
								1000.0f );
	p.drawText( LFO_GRAPH_X + 4, LFO_GRAPH_Y + s_lfoGraph->height() - 6,
							tr( "ms/LFO:" ) );
	p.drawText( LFO_GRAPH_X + 52, LFO_GRAPH_Y + s_lfoGraph->height() - 6,
						QString::number( ms_per_osc ) );

}




void envelopeAndLFOWidget::updateSampleVars( void )
{
	engine::getMixer()->lock();

	const float frames_per_env_seg = SECS_PER_ENV_SEGMENT *
					engine::getMixer()->sampleRate();
	// TODO: Remove the expKnobVals, time should be linear
	const f_cnt_t predelay_frames = static_cast<f_cnt_t>(
							frames_per_env_seg *
					expKnobVal( m_predelayKnob->value() ) );

	const f_cnt_t attack_frames = static_cast<f_cnt_t>( frames_per_env_seg *
					expKnobVal( m_attackKnob->value() ) );

	const f_cnt_t hold_frames = static_cast<f_cnt_t>( frames_per_env_seg *
					expKnobVal( m_holdKnob->value() ) );

	const f_cnt_t decay_frames = static_cast<f_cnt_t>( frames_per_env_seg *
					expKnobVal( m_decayKnob->value() *
						m_sustainKnob->value() ) );

	m_sustainLevel = 1.0f - m_sustainKnob->value();
	m_amount = m_amountKnob->value();
	if( m_amount >= 0 )
	{
		m_amountAdd = ( 1.0f - m_amount ) * m_valueForZeroAmount;
	}
	else
	{
		m_amountAdd = m_valueForZeroAmount;
	}

	m_pahdFrames = predelay_frames + attack_frames + hold_frames +
								decay_frames;
	m_rFrames = static_cast<f_cnt_t>( frames_per_env_seg *
					expKnobVal( m_releaseKnob->value() ) );

	if( static_cast<int>( floorf( m_amount * 1000.0f ) ) == 0 )
	{
		//m_pahdFrames = 0;
		m_rFrames = 0;
	}

	delete[] m_pahdEnv;
	delete[] m_rEnv;

	m_pahdEnv = new sample_t[m_pahdFrames];
	m_rEnv = new sample_t[m_rFrames];

	for( f_cnt_t i = 0; i < predelay_frames; ++i )
	{
		m_pahdEnv[i] = m_amountAdd;
	}

	f_cnt_t add = predelay_frames;

	for( f_cnt_t i = 0; i < attack_frames; ++i )
	{
		m_pahdEnv[add + i] = ( (float)i / attack_frames ) *
							m_amount + m_amountAdd;
	}

	add += attack_frames;
	for( f_cnt_t i = 0; i < hold_frames; ++i )
	{
		m_pahdEnv[add + i] = m_amount + m_amountAdd;
	}

	add += hold_frames;
	for( f_cnt_t i = 0; i < decay_frames; ++i )
	{
		m_pahdEnv[add + i] = ( m_sustainLevel + ( 1.0f -
						(float)i / decay_frames ) *
						( 1.0f - m_sustainLevel ) ) *
							m_amount + m_amountAdd;
	}

	for( f_cnt_t i = 0; i < m_rFrames; ++i )
	{
		m_rEnv[i] = ( (float)( m_rFrames - i ) / m_rFrames
					// * m_sustainLevel
					 		) * m_amount;
	}

	// save this calculation in real-time-part
	m_sustainLevel = m_sustainLevel * m_amount + m_amountAdd;


	const float frames_per_lfo_oscillation = SECS_PER_LFO_OSCILLATION *
					engine::getMixer()->sampleRate();
	m_lfoPredelayFrames = static_cast<f_cnt_t>( frames_per_lfo_oscillation *
				expKnobVal( m_lfoPredelayKnob->value() ) );
	m_lfoAttackFrames = static_cast<f_cnt_t>( frames_per_lfo_oscillation *
				expKnobVal( m_lfoAttackKnob->value() ) );
	m_lfoOscillationFrames = static_cast<f_cnt_t>(
						frames_per_lfo_oscillation *
						m_lfoSpeedKnob->value() );
	if( m_x100Model.value() )
	{
		m_lfoOscillationFrames /= 100;
	}
	m_lfoAmount = m_lfoAmountKnob->value() * 0.5f;

	m_used = TRUE;
	if( static_cast<int>( floorf( m_lfoAmount * 1000.0f ) ) == 0 )
	{
		m_lfoAmountIsZero = TRUE;
		if( static_cast<int>( floorf( m_amount * 1000.0f ) ) == 0 )
		{
			m_used = FALSE;
		}
	}
	else
	{
		m_lfoAmountIsZero = FALSE;
	}

	m_bad_lfoShapeData = TRUE;

	update();

	engine::getMixer()->unlock();
}




void envelopeAndLFOWidget::lfoUserWaveChanged( void )
{
	if( m_lfoWaveModel.value() == USER )
	{
		if( m_userWave.frames() <= 1 )
		{
			textFloat::displayMessage( tr( "Hint" ),
				tr( "Drag a sample from somewhere and drop "
					"it in this window." ),
					embed::getIconPixmap( "hint" ), 3000 );
		}
	}
}




#include "envelope_and_lfo_widget.moc"


#endif
