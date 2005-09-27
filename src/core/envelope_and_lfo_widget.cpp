/*
 * envelope_widget.cpp - widget which is m_used by envelope/lfo/filter-tab of
 *                       channel-window
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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

#define isChecked isOn
#define setChecked setOn

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


// how long should be each envelope-segment maximal (e.g. attack)?
const float SECS_PER_ENV_SEGMENT = 5.0f;
// how long should be one LFO-oscillation maximal?
const float SECS_PER_LFO_OSCILLATION = 20.0f;


const int env_graph_x = 8;
const int env_graph_y = 8;

const int env_knobs_y = 43;
const int env_knobs_lbl_y = env_knobs_y+35;
const int knob_x_spacing = 32;
const int predelay_knob_x = 6;
const int attack_knob_x = predelay_knob_x+knob_x_spacing;
const int hold_knob_x = attack_knob_x+knob_x_spacing;
const int decay_knob_x = hold_knob_x+knob_x_spacing;
const int sustain_knob_x = decay_knob_x+knob_x_spacing;
const int release_knob_x = sustain_knob_x+knob_x_spacing;
const int amount_knob_x = release_knob_x+knob_x_spacing;

const float time_unit_width = 36.0;


const int lfo_graph_x = 8;
const int lfo_graph_y = env_knobs_lbl_y+14;
const int lfo_knob_y = lfo_graph_y-2;
const int lfo_knobs_lbl_y = lfo_knob_y+35;
const int lfo_predelay_knob_x = lfo_graph_x + 100;
const int lfo_attack_knob_x = lfo_predelay_knob_x+knob_x_spacing;
const int lfo_speed_knob_x = lfo_attack_knob_x+knob_x_spacing;
const int lfo_amount_knob_x = lfo_speed_knob_x+knob_x_spacing;
const int lfo_shapes_x = lfo_graph_x;//predelay_knob_x;
const int lfo_shapes_y = lfo_graph_y + 50;


QPixmap * envelopeAndLFOWidget::s_envGraph = NULL;
QPixmap * envelopeAndLFOWidget::s_lfoGraph = NULL;
Uint32 envelopeAndLFOWidget::s_lfoFrame = 0;



envelopeAndLFOWidget::envelopeAndLFOWidget( float _value_for_zero_amount,
							QWidget * _parent ) :
	QWidget( _parent ),
	settings(),
#ifdef QT4
	specialBgHandlingWidget( palette().color( backgroundRole() ) ),
#else
	specialBgHandlingWidget( paletteBackgroundColor() ),
#endif
	m_used( FALSE ),
	m_valueForZeroAmount( _value_for_zero_amount ),
	m_pahdEnv( NULL ),
	m_rEnv( NULL ),
	m_lfoAmountIsZero( FALSE ),
	m_lfoShapeData( NULL ),
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

	m_predelayKnob = new knob( knobBright_26, this, tr( "Predelay-time" ) );
	m_predelayKnob->setLabel( tr( "DEL" ) );
	m_predelayKnob->setRange( 0.0, 1.0, 0.001 );
	m_predelayKnob->setValue( 0.0, TRUE );
	m_predelayKnob->move( predelay_knob_x, env_knobs_y );
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

	m_attackKnob = new knob( knobBright_26, this, tr( "Attack-time" ) );
	m_attackKnob->setLabel( tr( "ATT" ) );
	m_attackKnob->setRange( 0.0, 1.0, 0.001 );
	m_attackKnob->setValue( 0.0, TRUE );
	m_attackKnob->move( attack_knob_x, env_knobs_y );
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

	m_holdKnob = new knob( knobBright_26, this, tr( "Hold-time" ) );
	m_holdKnob->setLabel( tr( "HOLD" ) );
	m_holdKnob->setRange( 0.0, 1.0, 0.001 );
	m_holdKnob->setValue( 0.5, TRUE );
	m_holdKnob->move( hold_knob_x, env_knobs_y );
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

	m_decayKnob = new knob( knobBright_26, this, tr( "Decay-time" ) );
	m_decayKnob->setLabel( tr( "DEC" ) );
	m_decayKnob->setRange( 0.0, 1.0, 0.001 );
	m_decayKnob->setValue( 0.5, TRUE );
	m_decayKnob->move( decay_knob_x, env_knobs_y );
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

	m_sustainKnob = new knob( knobBright_26, this, tr( "Sustain-level" ) );
	m_sustainKnob->setLabel( tr( "SUST" ) );
	m_sustainKnob->setRange( 0.0, 1.0, 0.001 );
	m_sustainKnob->setValue( 0.5, TRUE );
	m_sustainKnob->move( sustain_knob_x, env_knobs_y );
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

	m_releaseKnob = new knob( knobBright_26, this, tr( "Release-time" ) );
	m_releaseKnob->setLabel( tr( "REL" ) );
	m_releaseKnob->setRange( 0.0, 1.0, 0.001 );
	m_releaseKnob->setValue( 0.1, TRUE );
	m_releaseKnob->move( release_knob_x, env_knobs_y );
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
						tr( "Modulation amount" ) );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->setRange( -1.0, 1.0, 0.005 );
	m_amountKnob->setValue( 0.0, TRUE );
	m_amountKnob->move( amount_knob_x, env_graph_y );
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
						tr( "LFO-predelay-time" ) );
	m_lfoPredelayKnob->setLabel( tr( "DEL" ) );
	m_lfoPredelayKnob->setRange( 0.0, 1.0, 0.001 );
	m_lfoPredelayKnob->setValue( 0.0, TRUE );
	m_lfoPredelayKnob->move( lfo_predelay_knob_x, lfo_knob_y );
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
						tr( "LFO-attack-time" ) );
	m_lfoAttackKnob->setLabel( tr( "ATT" ) );
	m_lfoAttackKnob->setRange( 0.0, 1.0, 0.001 );
	m_lfoAttackKnob->setValue( 0.0, TRUE );
	m_lfoAttackKnob->move( lfo_attack_knob_x, lfo_knob_y );
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

	m_lfoSpeedKnob = new knob( knobBright_26, this, tr( "LFO-speed" ) );
	m_lfoSpeedKnob->setLabel( tr( "SPD" ) );
	m_lfoSpeedKnob->setRange( 0.01, 1.0, 0.0001 );
	m_lfoSpeedKnob->setValue( 0.1, TRUE );
	m_lfoSpeedKnob->move( lfo_speed_knob_x, lfo_knob_y );
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
						tr( "LFO-modulation-amount" ) );
	m_lfoAmountKnob->setLabel( tr( "AMT" ) );
	m_lfoAmountKnob->setRange( -1.0, 1.0, 0.005 );
	m_lfoAmountKnob->setValue( 0.0, TRUE );
	m_lfoAmountKnob->move( lfo_amount_knob_x, lfo_knob_y );
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
	m_sinLfoBtn->move( lfo_shapes_x, lfo_shapes_y );
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
	m_triangleLfoBtn->move( lfo_shapes_x+15, lfo_shapes_y );
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
	m_sawLfoBtn->move( lfo_shapes_x+30, lfo_shapes_y );
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
	m_sqrLfoBtn->move( lfo_shapes_x+45, lfo_shapes_y );
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

	connect( m_sinLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoSinWaveCh( bool ) ) );
	connect( m_triangleLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoTriangleWaveCh( bool ) ) );
	connect( m_sawLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoSawWaveCh( bool ) ) );
	connect( m_sqrLfoBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( lfoSquareWaveCh( bool ) ) );

	QButtonGroup * lfo_shapes_algo_group = new QButtonGroup( this );
	lfo_shapes_algo_group->addButton( m_sinLfoBtn );
	lfo_shapes_algo_group->addButton( m_triangleLfoBtn );
	lfo_shapes_algo_group->addButton( m_sawLfoBtn );
	lfo_shapes_algo_group->addButton( m_sqrLfoBtn );
	lfo_shapes_algo_group->setExclusive( TRUE );
#ifndef QT4
	lfo_shapes_algo_group->hide();
#endif

	m_x100Btn = new pixmapButton( this );
	m_x100Btn->move( lfo_predelay_knob_x, lfo_graph_y + 36 );
	m_x100Btn->setBgGraphic( specialBgHandlingWidget::getBackground(
								m_x100Btn ) );

/*	m_x100Btn->setActiveGraphic( embed::getIconPixmap( "x100_active" ) );
	m_x100Btn->setInactiveGraphic( embed::getIconPixmap(
							"x100_inactive" ) );*/
	m_x100Btn->setBgGraphic(
			specialBgHandlingWidget::getBackground( m_x100Btn ) );
#ifdef QT4
	m_x100Btn->setWhatsThis(
#else
	QWhatsThis::add( m_x100Btn,
#endif
		tr( "Click here if the frequency of this LFO should be "
						"multiplied with 100." ) );
	toolTip::add( m_x100Btn, tr( "multiply LFO-frequency with 100" ) );
	connect( m_x100Btn, SIGNAL( toggled( bool ) ), this,
						SLOT( x100Toggled( bool ) ) );

	QLabel * x100_lbl = new QLabel( tr( "FREQ x 100" ), this );
	x100_lbl->setFont( pointSize<6>( x100_lbl->font() ) );
	x100_lbl->move( m_x100Btn->x() + 16, m_x100Btn->y() );
	x100_lbl->setFixedHeight( 10 );

	m_controlEnvAmountBtn = new pixmapButton( this );
	m_controlEnvAmountBtn->move( lfo_predelay_knob_x, lfo_graph_y + 54 );
	m_controlEnvAmountBtn->setBgGraphic(
			specialBgHandlingWidget::getBackground(
						m_controlEnvAmountBtn ) );
/*	m_controlEnvAmountBtn->setActiveGraphic( embed::getIconPixmap(
						"control_env_amount_active" ) );
	m_controlEnvAmountBtn->setInactiveGraphic( embed::getIconPixmap(
					"control_env_amount_inactive" ) );*/
#ifdef QT4
	m_controlEnvAmountBtn ->setWhatsThis(
#else
	QWhatsThis::add( m_controlEnvAmountBtn,
#endif
		tr( "Click here to make the envelope-amount controlled by this "
								"LFO." ) );
	QLabel * cea_lbl = new QLabel( tr( "MODULATE ENV-AMOUNT" ), this );
	cea_lbl->setFont( pointSize<6>( cea_lbl->font() ) );
	cea_lbl->move( m_controlEnvAmountBtn->x() + 16,
						m_controlEnvAmountBtn->y() );
	cea_lbl->setFixedSize( 110, 10 );
	toolTip::add( m_controlEnvAmountBtn,
				tr( "control envelope-amount by this LFO" ) );


#ifndef QT4
	// set background-mode for flicker-free redraw
	setBackgroundMode( Qt::NoBackground );
#endif

	connect( mixer::inst(), SIGNAL( sampleRateChanged() ), this,
						SLOT( updateSampleVars() ) );

	updateSampleVars();
}




envelopeAndLFOWidget::~envelopeAndLFOWidget()
{
	delete[] m_pahdEnv;
	delete[] m_rEnv;
	delete[] m_lfoShapeData;
}




void envelopeAndLFOWidget::triggerLFO( void )
{
	s_lfoFrame += mixer::inst()->framesPerAudioBuffer();
}




void envelopeAndLFOWidget::resetLFO( void )
{
	s_lfoFrame = 0;
}




inline float FASTCALL envelopeAndLFOWidget::lfoLevel( Uint32 _frame,
						Uint32 _frame_offset ) const
{
	if( m_lfoAmountIsZero == FALSE && _frame > m_lfoPredelayFrames )
	{
#ifdef LMMS_DEBUG
		assert( m_lfoShapeData != NULL );
#endif
		_frame -= m_lfoPredelayFrames;
		if( _frame > m_lfoAttackFrames )
		{
			return( m_lfoShapeData[( s_lfoFrame + _frame_offset ) %
						m_lfoOscillationFrames] );
		}
		return( m_lfoShapeData[( s_lfoFrame + _frame_offset ) %
						m_lfoOscillationFrames] *
						_frame / m_lfoAttackFrames );
	}
	return( 0.0f );
}




float FASTCALL envelopeAndLFOWidget::level( Uint32 _frame,
						Uint32 _release_begin,
						Uint32 _frame_offset ) const
{
	if( m_busy )
	{
		return( 0.0f );
	}
	if( _frame < m_pahdFrames )
	{
		if( m_controlEnvAmountBtn->isChecked() )
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
			if( m_controlEnvAmountBtn->isChecked() )
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
	if( m_controlEnvAmountBtn->isChecked() )
	{
		return( m_sustainLevel * ( 0.5f +
					lfoLevel( _frame, _frame_offset ) ) );
	}
	return( m_sustainLevel + lfoLevel( _frame, _frame_offset ) );
}




void envelopeAndLFOWidget::saveSettings( QDomDocument & ,
							QDomElement & _parent )
{
	_parent.setAttribute( "pdel", QString::number(
						m_predelayKnob->value() ) );
	_parent.setAttribute( "att", QString::number( m_attackKnob->value() ) );
	_parent.setAttribute( "hold", QString::number( m_holdKnob->value() ) );
	_parent.setAttribute( "dec", QString::number( m_decayKnob->value() ) );
	_parent.setAttribute( "sus", QString::number(
						m_sustainKnob->value() ) );
	_parent.setAttribute( "rel", QString::number(
						m_releaseKnob->value() ) );
	_parent.setAttribute( "amt", QString::number( m_amountKnob->value() ) );
	_parent.setAttribute( "lshp", QString::number( m_lfoShape ) );
	_parent.setAttribute( "lpdel", QString::number(
						m_lfoPredelayKnob->value() ) );
	_parent.setAttribute( "latt", QString::number(
						m_lfoAttackKnob->value() ) );
	_parent.setAttribute( "lspd", QString::number(
						m_lfoSpeedKnob->value() ) );
	_parent.setAttribute( "lamt", QString::number(
						m_lfoAmountKnob->value() ) );
	_parent.setAttribute( "x100", QString::number(
						m_x100Btn->isChecked() ) );
	_parent.setAttribute( "ctlenvamt", QString::number(
					m_controlEnvAmountBtn->isChecked() ) );
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
	m_x100Btn->setChecked( _this.attribute( "x100" ).toInt() );
	m_controlEnvAmountBtn->setChecked( _this.attribute(
							"ctlenvamt" ).toInt() );

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
	}

	m_busy = FALSE;

	updateSampleVars();
	update();
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
	p.drawPixmap( env_graph_x, env_graph_y, *s_envGraph );
	// draw LFO-graph
	p.drawPixmap( lfo_graph_x, lfo_graph_y, *s_lfoGraph );


	p.setFont( pointSize<8>( p.font() ) );

	const float gray_amount = 1.0f - fabsf( m_amountKnob->value() );

	p.setPen( QPen( QColor( static_cast<int>( 96 * gray_amount ),
				static_cast<int>( 255 - 159 * gray_amount ),
				static_cast<int>( 128 - 32 * gray_amount ) ),
									2 ) );

	const QColor end_points_color( 0xFF, 0xBF, 0x22 );
	const QColor end_points_bg_color( 0, 0, 2 );

	const int y_base = env_graph_y + s_envGraph->height() - 3;
	const int avail_height = s_envGraph->height() - 6;
	
	int x1 = env_graph_x + 2 + static_cast<int>( m_predelayKnob->value() *
							time_unit_width );
	int x2 = x1 + static_cast<int>( m_attackKnob->value() *
							time_unit_width );

	p.drawLine( x1, y_base, x2, y_base - avail_height );
	p.fillRect( x1 - 1, y_base - 2, 4, 4, end_points_bg_color );
	p.fillRect( x1, y_base - 1, 2, 2, end_points_color );
	x1 = x2;
	x2 = x1 + static_cast<int>( m_holdKnob->value() * time_unit_width );

	p.drawLine( x1, y_base - avail_height, x2, y_base - avail_height );
	p.fillRect( x1 - 1, y_base - 2 - avail_height, 4, 4,
							end_points_bg_color );
	p.fillRect( x1, y_base-1-avail_height, 2, 2, end_points_color );
	x1 = x2;
	x2 = x1 + static_cast<int>( ( m_decayKnob->value() *
						m_sustainKnob->value() ) *
							time_unit_width );

	p.drawLine( x1, y_base-avail_height, x2, static_cast<int>( y_base -
								avail_height +
				m_sustainKnob->value() * avail_height ) );
	p.fillRect( x1 - 1, y_base - 2 - avail_height, 4, 4,
							end_points_bg_color );
	p.fillRect( x1, y_base - 1 - avail_height, 2, 2, end_points_color );
	x1 = x2;
	x2 = x1 + static_cast<int>( m_releaseKnob->value() * time_unit_width );

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


	int lfo_graph_w = s_lfoGraph->width() - 6;	// substract border
	int lfo_graph_h = s_lfoGraph->height() - 6;	// substract border
	int graph_x_base = lfo_graph_x + 3;
	int graph_y_base = lfo_graph_y + 3 + lfo_graph_h / 2;

	const float frames_for_graph = SECS_PER_LFO_OSCILLATION *
					mixer::inst()->sampleRate() / 10;

	const float lfo_gray_amount = 1.0f - fabsf( m_lfoAmountKnob->value() );
	p.setPen( QPen( QColor( static_cast<int>( 96 * lfo_gray_amount ),
				static_cast<int>( 255 - 159 * lfo_gray_amount ),
				static_cast<int>( 128 - 32 *
							lfo_gray_amount ) ),
									2 ) );


	float osc_frames = m_lfoOscillationFrames;

	if( m_x100Btn->isChecked() )
	{
		osc_frames *= 100.0f;
	}

#ifdef QT4
	float old_y = 0;
#else
	int old_y = 0;
#endif
	for( int x = 0; x <= lfo_graph_w; ++x )
	{
		float val = 0.0;
		float cur_sample = x*frames_for_graph/lfo_graph_w;
		if( static_cast<Uint32>( cur_sample ) > m_lfoPredelayFrames )
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
			}
			if( (Uint32) cur_sample <= m_lfoAttackFrames )
			{
				val *= cur_sample / m_lfoAttackFrames;
			}
		}
#ifdef QT4
		float cur_y = -lfo_graph_h / 2.0f * val;
		p.drawLine( QLineF( graph_x_base + x - 1, graph_y_base + old_y,
						graph_x_base + x,
						graph_y_base + cur_y ) );
#else
		int cur_y = static_cast<int>( -lfo_graph_h / 2.0f * val );
		p.drawLine( graph_x_base + x - 1, graph_y_base + old_y,
				graph_x_base + x, graph_y_base + cur_y );
#endif
		old_y = cur_y;
	}

	p.setPen( QColor( 255, 192, 0 ) );
	int ms_per_osc = static_cast<int>( SECS_PER_LFO_OSCILLATION *
						m_lfoSpeedKnob->value() *
								1000.0f );
	p.drawText( lfo_graph_x + 4, lfo_graph_y + s_lfoGraph->height() - 6,
							tr( "ms/LFO:" ) );
	p.drawText( lfo_graph_x + 52, lfo_graph_y + s_lfoGraph->height() - 6,
						QString::number( ms_per_osc ) );

#ifndef QT4
	// blit drawn pixmap to actual widget
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void envelopeAndLFOWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() != Qt::LeftButton )
	{
		return;
	}

	if( QRect( env_graph_x, env_graph_y, s_envGraph->width(),
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
	else if( QRect( lfo_graph_x, lfo_graph_y, s_lfoGraph->width(),
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




void envelopeAndLFOWidget::updateSampleVars( void )
{
	if( !m_busy )
	{
		m_busy = TRUE;

		const float frames_per_env_seg = SECS_PER_ENV_SEGMENT *
						mixer::inst()->sampleRate();
		Uint32 predelay_frames = static_cast<Uint32>(
							frames_per_env_seg *
					expKnobVal( m_predelayKnob->value() ) );

		Uint32 attack_frames = static_cast<Uint32>( frames_per_env_seg *
					expKnobVal( m_attackKnob->value() ) );

		Uint32 hold_frames = static_cast<Uint32>( frames_per_env_seg *
					expKnobVal( m_holdKnob->value() ) );

		Uint32 decay_frames = static_cast<Uint32>( frames_per_env_seg *
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
		m_rFrames = static_cast<Uint32>( frames_per_env_seg *
					expKnobVal( m_releaseKnob->value() ) );

		if( static_cast<int>( floorf( m_amount * 1000.0f ) ) == 0 )
		{
			//m_pahdFrames = 0;
			m_rFrames = 0;
		}

		float * new_pahd_env = new float[m_pahdFrames];
		float * new_r_env = new float[m_rFrames];

		for( Uint32 i = 0; i < predelay_frames; ++i )
		{
			new_pahd_env[i] = m_amountAdd;
		}

		Uint32 add = predelay_frames;

		for( Uint32 i = 0; i < attack_frames; ++i )
		{
			new_pahd_env[add+i] = ( (float)i / attack_frames ) *
							m_amount + m_amountAdd;
		}

		add += attack_frames;
		for( Uint32 i = 0; i < hold_frames; ++i )
		{
			new_pahd_env[add+i] = m_amount + m_amountAdd;
		}

		add += hold_frames;
		for( Uint32 i = 0; i < decay_frames; ++i )
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

		for( Uint32 i = 0; i < m_rFrames; ++i )
		{
			new_r_env[i] = ( (float)( m_rFrames - i ) / m_rFrames *
						m_sustainLevel ) * m_amount;
		}

		// save this calculation in real-time-part
		m_sustainLevel = m_sustainLevel * m_amount + m_amountAdd;
		

		const float frames_per_lfo_oscillation =
						SECS_PER_LFO_OSCILLATION *
						mixer::inst()->sampleRate();
		m_lfoPredelayFrames = static_cast<Uint32>(
						frames_per_lfo_oscillation *
				expKnobVal( m_lfoPredelayKnob->value() ) );
		m_lfoAttackFrames = static_cast<Uint32>(
						frames_per_lfo_oscillation *
				expKnobVal( m_lfoAttackKnob->value() ) );
		m_lfoOscillationFrames = static_cast<Uint32>(
						frames_per_lfo_oscillation *
						m_lfoSpeedKnob->value() );
		if( m_x100Btn->isChecked() )
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
			m_lfoShapeData = new float[m_lfoOscillationFrames];
			for( Uint32 frame = 0; frame < m_lfoOscillationFrames;
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
	songEditor::inst()->setModified();
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
	songEditor::inst()->setModified();

	update();
}




void envelopeAndLFOWidget::lfoTriangleWaveCh( bool _on )
{
	if( _on )
	{
		m_lfoShape = TRIANGLE;
	}
	songEditor::inst()->setModified();

	update();
}




void envelopeAndLFOWidget::lfoSawWaveCh( bool _on )
{
	if( _on )
	{
		m_lfoShape = SAW;
	}
	songEditor::inst()->setModified();

	update();
}




void envelopeAndLFOWidget::lfoSquareWaveCh( bool _on )
{
	if( _on )
	{
		m_lfoShape = SQUARE;
	}
	songEditor::inst()->setModified();

	update();
}




#undef isChecked
#undef setChecked


#include "envelope_and_lfo_widget.moc"

