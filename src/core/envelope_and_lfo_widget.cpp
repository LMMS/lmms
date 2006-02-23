#ifndef SINGLE_SOURCE_COMPILE

/*
 * envelope_and_lfo_widget.cpp - widget which is m_used by envelope/lfo/filter-
 *                               tab of channel-window
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QPaintEvent>
#include <QButtonGroup>
#include <QWhatsThis>
#include <Qt/QtXml>
#include <QLabel>

#else

#include <qbuttongroup.h>
#include <qwhatsthis.h>
#include <qpainter.h>
#include <qpen.h>
#include <qdom.h>
#include <qlabel.h>

#endif


#include "envelope_and_lfo_widget.h"
#include "song_editor.h"
#include "embed.h"
#include "knob.h"
#include "pixmap_button.h"
#include "oscillator.h"
#include "debug.h"
#include "tooltip.h"
#include "gui_templates.h"
#include "led_checkbox.h"
#include "tempo_sync_knob.h"
#include "string_pair_drag.h"
#include "mmp.h"
#include "text_float.h"



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

QMap<engine *, vvector<envelopeAndLFOWidget *> >
					envelopeAndLFOWidget::s_EaLWidgets;



envelopeAndLFOWidget::envelopeAndLFOWidget( float _value_for_zero_amount,
						QWidget * _parent,
						engine * _engine ) :
	QWidget( _parent ),
	settings(),
#ifdef QT4
	specialBgHandlingWidget( palette().color( backgroundRole() ) ),
#else
	specialBgHandlingWidget( paletteBackgroundColor() ),
#endif
	engineObject( _engine ),
	m_used( FALSE ),
	m_valueForZeroAmount( _value_for_zero_amount ),
	m_pahdEnv( NULL ),
	m_rEnv( NULL ),
	m_lfoFrame( 0 ),
	m_lfoAmountIsZero( FALSE ),
	m_lfoShapeData( NULL ),
	m_userWave( eng() ),
	m_lfoShape( SIN ),
	m_busy( FALSE )
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

	s_EaLWidgets[eng()].push_back( this );


	m_predelayKnob = new knob( knobBright_26, this, tr( "Predelay-time" ),
									eng() );
	m_predelayKnob->setLabel( tr( "DEL" ) );
	m_predelayKnob->setRange( 0.0, 1.0, 0.001 );
	m_predelayKnob->setInitValue( 0.0 );
	m_predelayKnob->move( PREDELAY_KNOB_X, ENV_KNOBS_Y );
	m_predelayKnob->setHintText( tr( "Predelay:" ) + " ", "" );
#ifdef QT4
	m_predelayKnob->setWhatsThis(
#else
	QWhatsThis::add( m_predelayKnob,
#endif
		tr( "Use this knob for setting predelay of the current "
			"envelope. The bigger this value the longer the time "
			"before start of actual envelope." ) );
	connect( m_predelayKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_attackKnob = new knob( knobBright_26, this, tr( "Attack-time" ),
									eng() );
	m_attackKnob->setLabel( tr( "ATT" ) );
	m_attackKnob->setRange( 0.0, 1.0, 0.001 );
	m_attackKnob->setInitValue( 0.0 );
	m_attackKnob->move( ATTACK_KNOB_X, ENV_KNOBS_Y );
	m_attackKnob->setHintText( tr( "Attack:" )+" ", "" );
#ifdef QT4
	m_attackKnob->setWhatsThis(
#else
	QWhatsThis::add( m_attackKnob,
#endif
		tr( "Use this knob for setting attack-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to increase to attack-level. "
			"Choose a small value for instruments like pianos "
			"and a big value for strings." ) );
	connect( m_attackKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_holdKnob = new knob( knobBright_26, this, tr( "Hold-time" ), eng() );
	m_holdKnob->setLabel( tr( "HOLD" ) );
	m_holdKnob->setRange( 0.0, 1.0, 0.001 );
	m_holdKnob->setInitValue( 0.5 );
	m_holdKnob->move( HOLD_KNOB_X, ENV_KNOBS_Y );
	m_holdKnob->setHintText( tr( "Hold:" ) + " ", "" );
#ifdef QT4
	m_holdKnob->setWhatsThis(
#else
	QWhatsThis::add( m_holdKnob,
#endif
		tr( "Use this knob for setting hold-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope holds attack-level before it begins to "
			"decrease to sustain-level." ) );
	connect( m_holdKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_decayKnob = new knob( knobBright_26, this, tr( "Decay-time" ),
									eng() );
	m_decayKnob->setLabel( tr( "DEC" ) );
	m_decayKnob->setRange( 0.0, 1.0, 0.001 );
	m_decayKnob->setInitValue( 0.5 );
	m_decayKnob->move( DECAY_KNOB_X, ENV_KNOBS_Y );
	m_decayKnob->setHintText( tr( "Decay:" ) + " ", "" );
#ifdef QT4
	m_decayKnob->setWhatsThis(
#else
	QWhatsThis::add( m_decayKnob,
#endif
		tr( "Use this knob for setting decay-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to decrease from attack-level to "
			"sustain-level. Choose a small value for instruments "
			"like pianos." ) );
	connect( m_decayKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_sustainKnob = new knob( knobBright_26, this, tr( "Sustain-level" ),
									eng() );
	m_sustainKnob->setLabel( tr( "SUST" ) );
	m_sustainKnob->setRange( 0.0, 1.0, 0.001 );
	m_sustainKnob->setInitValue( 0.5 );
	m_sustainKnob->move( SUSTAIN_KNOB_X, ENV_KNOBS_Y );
	m_sustainKnob->setHintText( tr( "Sustain:" ) + " ", "" );
#ifdef QT4
	m_sustainKnob->setWhatsThis(
#else
	QWhatsThis::add( m_sustainKnob,
#endif
		tr( "Use this knob for setting sustain-level of the current "
			"envelope. The bigger this value the higher the level "
			"on which the envelope stays before going down to "
			"zero." ) );
	connect( m_sustainKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_releaseKnob = new knob( knobBright_26, this, tr( "Release-time" ),
									eng() );
	m_releaseKnob->setLabel( tr( "REL" ) );
	m_releaseKnob->setRange( 0.0, 1.0, 0.001 );
	m_releaseKnob->setInitValue( 0.1 );
	m_releaseKnob->move( RELEASE_KNOB_X, ENV_KNOBS_Y );
	m_releaseKnob->setHintText( tr( "Release:" ) + " ", "" );
#ifdef QT4
	m_releaseKnob->setWhatsThis(
#else
	QWhatsThis::add( m_releaseKnob,
#endif
		tr( "Use this knob for setting release-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to decrease from sustain-level to "
			"zero. Choose a big value for soft instruments like "
			"strings." ) );
	connect( m_releaseKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_amountKnob = new knob( knobBright_26, this,
					tr( "Modulation amount" ), eng() );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->setRange( -1.0, 1.0, 0.005 );
	m_amountKnob->setInitValue( 0.0 );
	m_amountKnob->move( AMOUNT_KNOB_X, ENV_GRAPH_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
#ifdef QT4
	m_amountKnob->setWhatsThis(
#else
	QWhatsThis::add( m_amountKnob,
#endif
		tr( "Use this knob for setting modulation amount of the "
			"current envelope. The bigger this value the more the "
			"according size (e.g. volume or cutoff-frequency) "
			"will be influenced by this envelope." ) );
	connect( m_amountKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );



	m_lfoPredelayKnob = new knob( knobBright_26, this,
					tr( "LFO-predelay-time" ), eng() );
	m_lfoPredelayKnob->setLabel( tr( "DEL" ) );
	m_lfoPredelayKnob->setRange( 0.0, 1.0, 0.001 );
	m_lfoPredelayKnob->setInitValue( 0.0 );
	m_lfoPredelayKnob->move( LFO_PREDELAY_KNOB_X, LFO_KNOB_Y );
	m_lfoPredelayKnob->setHintText( tr( "LFO-predelay:" ) + " ", "" );
#ifdef QT4
	m_lfoPredelayKnob->setWhatsThis(
#else
	QWhatsThis::add( m_lfoPredelayKnob,
#endif
		tr( "Use this knob for setting predelay-time of the current "
			"LFO. The bigger this value the the time until the "
			"LFO starts to oscillate." ) );
	connect( m_lfoPredelayKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_lfoAttackKnob = new knob( knobBright_26, this,
					tr( "LFO-attack-time" ), eng() );
	m_lfoAttackKnob->setLabel( tr( "ATT" ) );
	m_lfoAttackKnob->setRange( 0.0, 1.0, 0.001 );
	m_lfoAttackKnob->setInitValue( 0.0 );
	m_lfoAttackKnob->move( LFO_ATTACK_KNOB_X, LFO_KNOB_Y );
	m_lfoAttackKnob->setHintText( tr( "LFO-attack:" ) + " ", "" );
#ifdef QT4
	m_lfoAttackKnob->setWhatsThis(
#else
	QWhatsThis::add( m_lfoAttackKnob,
#endif
		tr( "Use this knob for setting attack-time of the current LFO. "
			"The bigger this value the longer the LFO needs to "
			"increase its amplitude to maximum." ) );
	connect( m_lfoAttackKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_lfoSpeedKnob = new tempoSyncKnob( knobBright_26, this,
					tr( "LFO-speed" ), eng(), 20000.0 );
	m_lfoSpeedKnob->setLabel( tr( "SPD" ) );
	m_lfoSpeedKnob->setRange( 0.01, 1.0, 0.0001 );
	m_lfoSpeedKnob->setInitValue( 0.1 );
	m_lfoSpeedKnob->move( LFO_SPEED_KNOB_X, LFO_KNOB_Y );
	m_lfoSpeedKnob->setHintText( tr( "LFO-speed:" ) + " ", "" );
#ifdef QT4
	m_lfoSpeedKnob->setWhatsThis(
#else
	QWhatsThis::add( m_lfoSpeedKnob,
#endif
		tr( "Use this knob for setting speed of the current LFO. The "
			"bigger this value the faster the LFO oscillates and "
			"the faster will be your effect." ) );
	connect( m_lfoSpeedKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );

	m_lfoAmountKnob = new knob( knobBright_26, this,
					tr( "LFO-modulation-amount" ), eng() );
	m_lfoAmountKnob->setLabel( tr( "AMT" ) );
	m_lfoAmountKnob->setRange( -1.0, 1.0, 0.005 );
	m_lfoAmountKnob->setInitValue( 0.0 );
	m_lfoAmountKnob->move( LFO_AMOUNT_KNOB_X, LFO_KNOB_Y );
	m_lfoAmountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
#ifdef QT4
	m_lfoAmountKnob->setWhatsThis(
#else
	QWhatsThis::add( m_lfoAmountKnob,
#endif
		tr( "Use this knob for setting modulation amount of the "
			"current LFO. The bigger this value the more the "
			"selected size (e.g. volume or cutoff-frequency) will "
			"be influenced by this LFO." ) );
	connect( m_lfoAmountKnob, SIGNAL( valueChanged( float ) ), this,
				SLOT( updateAfterKnobChange( float ) ) );


	m_sinLfoBtn = new pixmapButton( this );
	m_sinLfoBtn->move( LFO_SHAPES_X, LFO_SHAPES_Y );
	m_sinLfoBtn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
	m_sinLfoBtn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );
	m_sinLfoBtn->setChecked( TRUE );
#ifdef QT4
	m_sinLfoBtn->setWhatsThis(
#else
	QWhatsThis::add( m_sinLfoBtn,
#endif
		tr( "Click here if you want a sine-wave for current "
							"oscillator." ) );

	m_triangleLfoBtn = new pixmapButton( this );
	m_triangleLfoBtn->move( LFO_SHAPES_X+15, LFO_SHAPES_Y );
	m_triangleLfoBtn->setActiveGraphic( embed::getIconPixmap(
						"triangle_wave_active" ) );
	m_triangleLfoBtn->setInactiveGraphic( embed::getIconPixmap(
						"triangle_wave_inactive" ) );
#ifdef QT4
	m_triangleLfoBtn->setWhatsThis(
#else
	QWhatsThis::add( m_triangleLfoBtn,
#endif
		tr( "Click here if you want a triangle-wave for current "
							"oscillator." ) );

	m_sawLfoBtn = new pixmapButton( this );
	m_sawLfoBtn->move( LFO_SHAPES_X+30, LFO_SHAPES_Y );
	m_sawLfoBtn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
	m_sawLfoBtn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );
#ifdef QT4
	m_sawLfoBtn->setWhatsThis(
#else
	QWhatsThis::add( m_sawLfoBtn,
#endif
		tr( "Click here if you want a saw-wave for current "
							"oscillator." ) );

	m_sqrLfoBtn = new pixmapButton( this );
	m_sqrLfoBtn->move( LFO_SHAPES_X+45, LFO_SHAPES_Y );
	m_sqrLfoBtn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
	m_sqrLfoBtn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );
#ifdef QT4
	m_sqrLfoBtn->setWhatsThis(
#else
	QWhatsThis::add( m_sqrLfoBtn,
#endif
		tr( "Click here if you want a square-wave for current "
							"oscillator." ) );

	m_usrLfoBtn = new pixmapButton( this );
	m_usrLfoBtn->move( LFO_SHAPES_X+60, LFO_SHAPES_Y );
	m_usrLfoBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
	m_usrLfoBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
#ifdef QT4
	m_usrLfoBtn->setWhatsThis(
#else
	QWhatsThis::add( m_usrLfoBtn,
#endif
		tr( "Click here if you want a user-defined wave for current "
			"oscillator. Afterwards drag an according sample-"
			"file into LFO-graph." ) );

	connect( m_sinLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoSinWaveCh( bool ) ) );
	connect( m_triangleLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoTriangleWaveCh( bool ) ) );
	connect( m_sawLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoSawWaveCh( bool ) ) );
	connect( m_sqrLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoSquareWaveCh( bool ) ) );
	connect( m_usrLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoUserWaveCh( bool ) ) );

	QButtonGroup * lfo_shapes_algo_group = new QButtonGroup( this );
	lfo_shapes_algo_group->addButton( m_sinLfoBtn );
	lfo_shapes_algo_group->addButton( m_triangleLfoBtn );
	lfo_shapes_algo_group->addButton( m_sawLfoBtn );
	lfo_shapes_algo_group->addButton( m_sqrLfoBtn );
	lfo_shapes_algo_group->addButton( m_usrLfoBtn );
	lfo_shapes_algo_group->setExclusive( TRUE );
#ifndef QT4
	lfo_shapes_algo_group->hide();
#endif

	m_x100Cb = new ledCheckBox( tr( "FREQ x 100" ), this );
	m_x100Cb->setFont( pointSize<6>( m_x100Cb->font() ) );
	m_x100Cb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 36 );
#ifdef QT4
	m_x100Cb->setWhatsThis(
#else
	QWhatsThis::add( m_x100Cb,
#endif
		tr( "Click here if the frequency of this LFO should be "
						"multiplied with 100." ) );
	toolTip::add( m_x100Cb, tr( "multiply LFO-frequency with 100" ) );
	connect( m_x100Cb, SIGNAL( toggled( bool ) ), this,
						SLOT( x100Toggled( bool ) ) );


	m_controlEnvAmountCb = new ledCheckBox( tr( "MODULATE ENV-AMOUNT" ),
									this );
	m_controlEnvAmountCb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 54 );
	m_controlEnvAmountCb->setFont( pointSize<6>(
					m_controlEnvAmountCb->font() ) );
#ifdef QT4
	m_controlEnvAmountCb ->setWhatsThis(
#else
	QWhatsThis::add( m_controlEnvAmountCb,
#endif
		tr( "Click here to make the envelope-amount controlled by this "
								"LFO." ) );
	toolTip::add( m_controlEnvAmountCb,
				tr( "control envelope-amount by this LFO" ) );


#ifndef QT4
	// set background-mode for flicker-free redraw
	setBackgroundMode( Qt::NoBackground );
#endif
	setAcceptDrops( TRUE );

	connect( eng()->getMixer(), SIGNAL( sampleRateChanged() ), this,
						SLOT( updateSampleVars() ) );

	updateSampleVars();
}




envelopeAndLFOWidget::~envelopeAndLFOWidget()
{
	delete[] m_pahdEnv;
	delete[] m_rEnv;
	delete[] m_lfoShapeData;

	vvector<envelopeAndLFOWidget *> & v = s_EaLWidgets[eng()];
	if( qFind( v.begin(), v.end(), this ) != v.end() )
	{
		v.erase( qFind( v.begin(), v.end(), this ) );
	}
}




void envelopeAndLFOWidget::triggerLFO( engine * _engine )
{
	vvector<envelopeAndLFOWidget *> & v = s_EaLWidgets[_engine];
	for( vvector<envelopeAndLFOWidget *>::iterator it = v.begin();
							it != v.end(); ++it )
	{
		( *it )->m_lfoFrame +=
				_engine->getMixer()->framesPerAudioBuffer();
	}
}




void envelopeAndLFOWidget::resetLFO( engine * _engine )
{
	vvector<envelopeAndLFOWidget *> & v = s_EaLWidgets[_engine];
	for( vvector<envelopeAndLFOWidget *>::iterator it = v.begin();
							it != v.end(); ++it )
	{
		( *it )->m_lfoFrame = 0;
	}
}




inline float FASTCALL envelopeAndLFOWidget::lfoLevel( f_cnt_t _frame,
					const f_cnt_t _frame_offset ) const
{
	if( m_lfoAmountIsZero == FALSE && _frame > m_lfoPredelayFrames )
	{
#ifdef LMMS_DEBUG
		assert( m_lfoShapeData != NULL );
#endif
		_frame -= m_lfoPredelayFrames;
		if( _frame > m_lfoAttackFrames )
		{
			return( m_lfoShapeData[( m_lfoFrame + _frame_offset ) %
						m_lfoOscillationFrames] );
		}
		return( m_lfoShapeData[( m_lfoFrame + _frame_offset ) %
						m_lfoOscillationFrames] *
						_frame / m_lfoAttackFrames );
	}
	return( 0.0f );
}




float FASTCALL envelopeAndLFOWidget::level( f_cnt_t _frame,
					const f_cnt_t _release_begin,
					const f_cnt_t _frame_offset ) const
{
	if( m_busy )
	{
		return( 0.0f );
	}
	if( _frame < m_pahdFrames )
	{
		if( m_controlEnvAmountCb->isChecked() )
		{
			return( m_pahdEnv[_frame] * ( 0.5f +
					lfoLevel( _frame, _frame_offset ) ) );
		}
		else
		{
			return( m_pahdEnv[_frame] +
					lfoLevel( _frame, _frame_offset ) );
		}
	}
	else if( _frame > _release_begin )
	{
		_frame -= _release_begin;
		if( _frame < m_rFrames )
		{
			if( m_controlEnvAmountCb->isChecked() )
			{
				return( m_rEnv[_frame] * ( 0.5f +
					lfoLevel( _frame, _frame_offset ) ) );
			}
			else
			{
				return( m_rEnv[_frame] +
					lfoLevel( _frame, _frame_offset ) );
			}
		}
		else
		{
			return( 0.0f );
		}
	}
	if( m_controlEnvAmountCb->isChecked() )
	{
		return( m_sustainLevel * ( 0.5f +
					lfoLevel( _frame, _frame_offset ) ) );
	}
	return( m_sustainLevel + lfoLevel( _frame, _frame_offset ) );
}




void envelopeAndLFOWidget::saveSettings( QDomDocument & ,
							QDomElement & _parent )
{
	_parent.setAttribute( "pdel", m_predelayKnob->value() );
	_parent.setAttribute( "att", m_attackKnob->value() );
	_parent.setAttribute( "hold", m_holdKnob->value() );
	_parent.setAttribute( "dec", m_decayKnob->value() );
	_parent.setAttribute( "sus", m_sustainKnob->value() );
	_parent.setAttribute( "rel", m_releaseKnob->value() );
	_parent.setAttribute( "amt", m_amountKnob->value() );
	_parent.setAttribute( "lshp", m_lfoShape );
	_parent.setAttribute( "lpdel", m_lfoPredelayKnob->value() );
	_parent.setAttribute( "latt", m_lfoAttackKnob->value() );
	_parent.setAttribute( "lspd", m_lfoSpeedKnob->value() );
	_parent.setAttribute( "lamt", m_lfoAmountKnob->value() );
	_parent.setAttribute( "x100", m_x100Cb->isChecked() );
	_parent.setAttribute( "ctlenvamt", m_controlEnvAmountCb->isChecked() );
	_parent.setAttribute( "lfosyncmode",
					( int ) m_lfoSpeedKnob->getSyncMode() );
	_parent.setAttribute( "userwavefile", m_userWave.audioFile() );
}




void envelopeAndLFOWidget::loadSettings( const QDomElement & _this )
{
	m_busy = TRUE;

	m_predelayKnob->setValue( _this.attribute( "pdel" ).toFloat() );
	m_attackKnob->setValue( _this.attribute( "att" ).toFloat() );
	m_holdKnob->setValue( _this.attribute( "hold" ).toFloat() );
	m_decayKnob->setValue( _this.attribute( "dec" ).toFloat() );
	m_sustainKnob->setValue( _this.attribute( "sus" ).toFloat() );
	m_releaseKnob->setValue( _this.attribute( "rel" ).toFloat() );
	m_amountKnob->setValue( _this.attribute( "amt" ).toFloat() );
	m_lfoShape = static_cast<lfoShapes>( _this.attribute(
							"lshp" ).toInt() );
	m_lfoPredelayKnob->setValue( _this.attribute( "lpdel" ).toFloat() );
	m_lfoAttackKnob->setValue( _this.attribute( "latt" ).toFloat() );
	m_lfoSpeedKnob->setValue( _this.attribute( "lspd" ).toFloat() );
	m_lfoAmountKnob->setValue( _this.attribute( "lamt" ).toFloat() );
	m_x100Cb->setChecked( _this.attribute( "x100" ).toInt() );
	m_controlEnvAmountCb->setChecked( _this.attribute(
							"ctlenvamt" ).toInt() );
	m_lfoSpeedKnob->setSyncMode(
		( tempoSyncKnob::tempoSyncMode ) _this.attribute(
						"lfosyncmode" ).toInt() );
	m_userWave.setAudioFile( _this.attribute( "userwavefile" ) );

	switch( m_lfoShape )
	{
		case SIN:
			m_sinLfoBtn->setChecked( TRUE );
			break;

		case TRIANGLE:
			m_triangleLfoBtn->setChecked( TRUE );
			break;

		case SAW:
			m_sawLfoBtn->setChecked( TRUE );
			break;

		case SQUARE:
			m_sqrLfoBtn->setChecked( TRUE );
			break;

		case USER:
			m_usrLfoBtn->setChecked( TRUE );
			break;
	}

	m_busy = FALSE;

	updateSampleVars();
	update();
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
		m_usrLfoBtn->setChecked( TRUE );
		lfoUserWaveCh( TRUE );
		_de->accept();
	}
	else if( type == QString( "tco_%1" ).arg( track::SAMPLE_TRACK ) )
	{
		multimediaProject mmp( value, FALSE );
		m_userWave.setAudioFile( mmp.content().firstChild().toElement().
							attribute( "src" ) );
		m_usrLfoBtn->setChecked( TRUE );
		lfoUserWaveCh( TRUE );
		_de->accept();
	}
}




void envelopeAndLFOWidget::paintEvent( QPaintEvent * )
{
	updateSampleVars();

#ifdef QT4
	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );
#else
	// create pixmap for whole widget
	QPixmap pm( size() );
	pm.fill( this, rect().topLeft() );

	// and a painter for it
	QPainter p( &pm, this );
#endif

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
					eng()->getMixer()->sampleRate() / 10;

	const float lfo_gray_amount = 1.0f - fabsf( m_lfoAmountKnob->value() );
	p.setPen( QPen( QColor( static_cast<int>( 96 * lfo_gray_amount ),
				static_cast<int>( 255 - 159 * lfo_gray_amount ),
				static_cast<int>( 128 - 32 *
							lfo_gray_amount ) ),
									2 ) );


	float osc_frames = m_lfoOscillationFrames;

	if( m_x100Cb->isChecked() )
	{
		osc_frames *= 100.0f;
	}

#ifdef QT4
	float old_y = 0;
#else
	int old_y = 0;
#endif
	for( int x = 0; x <= LFO_GRAPH_W; ++x )
	{
		float val = 0.0;
		float cur_sample = x * frames_for_graph / LFO_GRAPH_W;
		if( static_cast<f_cnt_t>( cur_sample ) > m_lfoPredelayFrames )
		{
			float phase = ( cur_sample -= m_lfoPredelayFrames ) /
								osc_frames;
			switch( m_lfoShape )
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
					val = oscillator::userWaveSample( phase,
							m_userWave.data(),
							m_userWave.frames() );
			}
			if( static_cast<f_cnt_t>( cur_sample ) <=
							m_lfoAttackFrames )
			{
				val *= cur_sample / m_lfoAttackFrames;
			}
		}
#ifdef QT4
		float cur_y = -LFO_GRAPH_H / 2.0f * val;
		p.drawLine( QLineF( graph_x_base + x - 1, graph_y_base + old_y,
						graph_x_base + x,
						graph_y_base + cur_y ) );
#else
		int cur_y = static_cast<int>( -LFO_GRAPH_H / 2.0f * val );
		p.drawLine( graph_x_base + x - 1, graph_y_base + old_y,
				graph_x_base + x, graph_y_base + cur_y );
#endif
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

#ifndef QT4
	// blit drawn pixmap to actual widget
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void envelopeAndLFOWidget::updateSampleVars( void )
{
	if( !m_busy )
	{
		m_busy = TRUE;

		const float frames_per_env_seg = SECS_PER_ENV_SEGMENT *
						eng()->getMixer()->sampleRate();
		const f_cnt_t predelay_frames = static_cast<f_cnt_t>(
							frames_per_env_seg *
					expKnobVal( m_predelayKnob->value() ) );

		const f_cnt_t attack_frames = static_cast<f_cnt_t>(
							frames_per_env_seg *
					expKnobVal( m_attackKnob->value() ) );

		const f_cnt_t hold_frames = static_cast<f_cnt_t>(
							frames_per_env_seg *
					expKnobVal( m_holdKnob->value() ) );

		const f_cnt_t decay_frames = static_cast<f_cnt_t>(
							frames_per_env_seg *
					expKnobVal( m_decayKnob->value() *
						m_sustainKnob->value() ) );

		m_sustainLevel = 1.0f - m_sustainKnob->value();
		m_amount = m_amountKnob->value();
		if( m_amount >= 0 )
		{
			m_amountAdd = ( 1.0f - m_amount ) *
							m_valueForZeroAmount;
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

		sample_t * new_pahd_env = new sample_t[m_pahdFrames];
		sample_t * new_r_env = new sample_t[m_rFrames];

		for( f_cnt_t i = 0; i < predelay_frames; ++i )
		{
			new_pahd_env[i] = m_amountAdd;
		}

		f_cnt_t add = predelay_frames;

		for( f_cnt_t i = 0; i < attack_frames; ++i )
		{
			new_pahd_env[add+i] = ( (float)i / attack_frames ) *
							m_amount + m_amountAdd;
		}

		add += attack_frames;
		for( f_cnt_t i = 0; i < hold_frames; ++i )
		{
			new_pahd_env[add+i] = m_amount + m_amountAdd;
		}

		add += hold_frames;
		for( f_cnt_t i = 0; i < decay_frames; ++i )
		{
			new_pahd_env[add+i] = ( m_sustainLevel + ( 1.0f -
						(float)i / decay_frames ) *
						( 1.0f - m_sustainLevel ) ) *
							m_amount + m_amountAdd;
		}

		delete[] m_pahdEnv;
		delete[] m_rEnv;

		m_pahdEnv = new_pahd_env;
		m_rEnv = new_r_env;

		for( f_cnt_t i = 0; i < m_rFrames; ++i )
		{
			new_r_env[i] = ( (float)( m_rFrames - i ) / m_rFrames *
						m_sustainLevel ) * m_amount;
		}

		// save this calculation in real-time-part
		m_sustainLevel = m_sustainLevel * m_amount + m_amountAdd;
		

		const float frames_per_lfo_oscillation =
						SECS_PER_LFO_OSCILLATION *
						eng()->getMixer()->sampleRate();
		m_lfoPredelayFrames = static_cast<f_cnt_t>(
						frames_per_lfo_oscillation *
				expKnobVal( m_lfoPredelayKnob->value() ) );
		m_lfoAttackFrames = static_cast<f_cnt_t>(
						frames_per_lfo_oscillation *
				expKnobVal( m_lfoAttackKnob->value() ) );
		m_lfoOscillationFrames = static_cast<f_cnt_t>(
						frames_per_lfo_oscillation *
						m_lfoSpeedKnob->value() );
		if( m_x100Cb->isChecked() )
		{
			m_lfoOscillationFrames /= 100;
		}
		m_lfoAmount = m_lfoAmountKnob->value() * 0.5f;

		m_used = TRUE;
		if( static_cast<int>( floorf( m_lfoAmount * 1000.0f ) ) == 0 )
		{
			m_lfoAmountIsZero = TRUE;
			if( static_cast<int>( floorf( m_amount * 1000.0f ) ) ==
									0 )
			{
				m_used = FALSE;
			}
		}
		else
		{
			m_lfoAmountIsZero = FALSE;
		}

		if( m_lfoAmountIsZero == FALSE )
		{
			delete[] m_lfoShapeData;
			m_lfoShapeData = new sample_t[m_lfoOscillationFrames];
			for( f_cnt_t frame = 0; frame < m_lfoOscillationFrames;
								++frame )
			{
				const float phase = frame / static_cast<float>(
						m_lfoOscillationFrames );
				// hope, compiler optimizes well and places
				// branches out of loop and generates one loop
				// for each branch...
				switch( m_lfoShape  )
				{
					case TRIANGLE:
						m_lfoShapeData[frame] =
				oscillator::triangleSample( phase );
						break;

					case SQUARE:
						m_lfoShapeData[frame] =
				oscillator::squareSample( phase );
						break;

					case SAW:
						m_lfoShapeData[frame] =
				oscillator::sawSample( phase );
						break;
					case USER:
						m_lfoShapeData[frame] =
				oscillator::userWaveSample( phase,
							m_userWave.data(),
							m_userWave.frames() );
						break;
					case SIN:
					default:
						m_lfoShapeData[frame] =
				oscillator::sinSample( phase );
						break;
				}
				m_lfoShapeData[frame] *= m_lfoAmount;
			}
		}
		m_busy = FALSE;
	}
}




void envelopeAndLFOWidget::x100Toggled( bool )
{
	eng()->getSongEditor()->setModified();
	updateSampleVars();
}




void envelopeAndLFOWidget::updateAfterKnobChange( float )
{
	update();
}




void envelopeAndLFOWidget::lfoSinWaveCh( bool _on )
{
	if( _on )
	{
		m_lfoShape = SIN;
	}
	eng()->getSongEditor()->setModified();

	update();
}




void envelopeAndLFOWidget::lfoTriangleWaveCh( bool _on )
{
	if( _on )
	{
		m_lfoShape = TRIANGLE;
	}
	eng()->getSongEditor()->setModified();

	update();
}




void envelopeAndLFOWidget::lfoSawWaveCh( bool _on )
{
	if( _on )
	{
		m_lfoShape = SAW;
	}
	eng()->getSongEditor()->setModified();

	update();
}




void envelopeAndLFOWidget::lfoSquareWaveCh( bool _on )
{
	if( _on )
	{
		m_lfoShape = SQUARE;
	}
	eng()->getSongEditor()->setModified();

	update();
}




void envelopeAndLFOWidget::lfoUserWaveCh( bool _on )
{
	if( _on )
	{
		if( m_userWave.frames() <= 1 )
		{
			textFloat::displayMessage( tr( "Hint" ),
				tr( "Drag a sample from somewhere and drop "
					"it in this window." ),
					embed::getIconPixmap( "hint" ), 3000 );
		}
		m_lfoShape = USER;
	}
	eng()->getSongEditor()->setModified();

	update();
}




#include "envelope_and_lfo_widget.moc"


#endif
