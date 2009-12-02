/*
 * EnvelopeAndLfoView.cpp - widget which is m_used by envelope/lfo/filter-
 *                          tab of instrument track window
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include "EnvelopeAndLfoView.h"
#include "EnvelopeAndLfoParameters.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "knob.h"
#include "led_checkbox.h"
#include "Mixer.h"
#include "mmp.h"
#include "Oscillator.h"
#include "pixmap_button.h"
#include "string_pair_drag.h"
#include "TempoSyncKnob.h"
#include "text_float.h"
#include "tooltip.h"
#include "track.h"


extern const float SECS_PER_ENV_SEGMENT;
extern const float SECS_PER_LFO_OSCILLATION;


const int ENV_GRAPH_X = 6;
const int ENV_GRAPH_Y = 6;

const int ENV_KNOBS_Y = 43;
const int ENV_KNOBS_LBL_Y = ENV_KNOBS_Y + 35;
const int KNOB_X_SPACING = 32;
const int PREDELAY_KNOB_X = 6;
const int ATTACK_KNOB_X = PREDELAY_KNOB_X + KNOB_X_SPACING;
const int HOLD_KNOB_X = ATTACK_KNOB_X + KNOB_X_SPACING;
const int DECAY_KNOB_X = HOLD_KNOB_X + KNOB_X_SPACING;
const int SUSTAIN_KNOB_X = DECAY_KNOB_X + KNOB_X_SPACING;
const int RELEASE_KNOB_X = SUSTAIN_KNOB_X + KNOB_X_SPACING;
const int AMOUNT_KNOB_X = RELEASE_KNOB_X + KNOB_X_SPACING;

const float TIME_UNIT_WIDTH = 36.0;

const int LFO_GRAPH_X = 6;
const int LFO_GRAPH_Y = ENV_KNOBS_LBL_Y + 14;
const int LFO_KNOB_Y = LFO_GRAPH_Y - 2;
const int LFO_PREDELAY_KNOB_X = LFO_GRAPH_X + 100;
const int LFO_ATTACK_KNOB_X = LFO_PREDELAY_KNOB_X + KNOB_X_SPACING;
const int LFO_SPEED_KNOB_X = LFO_ATTACK_KNOB_X + KNOB_X_SPACING;
const int LFO_AMOUNT_KNOB_X = LFO_SPEED_KNOB_X + KNOB_X_SPACING;
const int LFO_SHAPES_X = LFO_GRAPH_X;//PREDELAY_KNOB_X;
const int LFO_SHAPES_Y = LFO_GRAPH_Y + 50;


QPixmap * EnvelopeAndLfoView::s_envGraph = NULL;
QPixmap * EnvelopeAndLfoView::s_lfoGraph = NULL;



EnvelopeAndLfoView::EnvelopeAndLfoView( QWidget * _parent ) :
	QWidget( _parent ),
	ModelView( NULL, this ),
	m_params( NULL )
{
	if( s_envGraph == NULL )
	{
		s_envGraph = new QPixmap( embed::getIconPixmap( "envelope_graph" ) );
	}
	if( s_lfoGraph == NULL )
	{
		s_lfoGraph = new QPixmap( embed::getIconPixmap( "lfo_graph" ) );
	}

	m_predelayKnob = new knob( knobBright_26, this );
	m_predelayKnob->setLabel( tr( "DEL" ) );
	m_predelayKnob->move( PREDELAY_KNOB_X, ENV_KNOBS_Y );
	m_predelayKnob->setHintText( tr( "Predelay:" ) + " ", "" );
	m_predelayKnob->setWhatsThis(
		tr( "Use this knob for setting predelay of the current "
			"envelope. The bigger this value the longer the time "
			"before start of actual envelope." ) );


	m_attackKnob = new knob( knobBright_26, this );
	m_attackKnob->setLabel( tr( "ATT" ) );
	m_attackKnob->move( ATTACK_KNOB_X, ENV_KNOBS_Y );
	m_attackKnob->setHintText( tr( "Attack:" )+" ", "" );
	m_attackKnob->setWhatsThis(
		tr( "Use this knob for setting attack-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to increase to attack-level. "
			"Choose a small value for instruments like pianos "
			"and a big value for strings." ) );

	m_holdKnob = new knob( knobBright_26, this );
	m_holdKnob->setLabel( tr( "HOLD" ) );
	m_holdKnob->move( HOLD_KNOB_X, ENV_KNOBS_Y );
	m_holdKnob->setHintText( tr( "Hold:" ) + " ", "" );
	m_holdKnob->setWhatsThis(
		tr( "Use this knob for setting hold-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope holds attack-level before it begins to "
			"decrease to sustain-level." ) );

	m_decayKnob = new knob( knobBright_26, this );
	m_decayKnob->setLabel( tr( "DEC" ) );
	m_decayKnob->move( DECAY_KNOB_X, ENV_KNOBS_Y );
	m_decayKnob->setHintText( tr( "Decay:" ) + " ", "" );
	m_decayKnob->setWhatsThis(
		tr( "Use this knob for setting decay-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to decrease from attack-level to "
			"sustain-level. Choose a small value for instruments "
			"like pianos." ) );


	m_sustainKnob = new knob( knobBright_26, this );
	m_sustainKnob->setLabel( tr( "SUST" ) );
	m_sustainKnob->move( SUSTAIN_KNOB_X, ENV_KNOBS_Y );
	m_sustainKnob->setHintText( tr( "Sustain:" ) + " ", "" );
	m_sustainKnob->setWhatsThis(
		tr( "Use this knob for setting sustain-level of the current "
			"envelope. The bigger this value the higher the level "
			"on which the envelope stays before going down to "
			"zero." ) );


	m_releaseKnob = new knob( knobBright_26, this );
	m_releaseKnob->setLabel( tr( "REL" ) );
	m_releaseKnob->move( RELEASE_KNOB_X, ENV_KNOBS_Y );
	m_releaseKnob->setHintText( tr( "Release:" ) + " ", "" );
	m_releaseKnob->setWhatsThis(
		tr( "Use this knob for setting release-time of the current "
			"envelope. The bigger this value the longer the "
			"envelope needs to decrease from sustain-level to "
			"zero. Choose a big value for soft instruments like "
			"strings." ) );


	m_amountKnob = new knob( knobBright_26, this );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->move( AMOUNT_KNOB_X, ENV_GRAPH_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
	m_amountKnob->setWhatsThis(
		tr( "Use this knob for setting modulation amount of the "
			"current envelope. The bigger this value the more the "
			"according size (e.g. volume or cutoff-frequency) "
			"will be influenced by this envelope." ) );




	m_lfoPredelayKnob = new knob( knobBright_26, this );
	m_lfoPredelayKnob->setLabel( tr( "DEL" ) );
	m_lfoPredelayKnob->move( LFO_PREDELAY_KNOB_X, LFO_KNOB_Y );
	m_lfoPredelayKnob->setHintText( tr( "LFO predelay:" ) + " ", "" );
	m_lfoPredelayKnob->setWhatsThis(
		tr( "Use this knob for setting predelay-time of the current "
			"LFO. The bigger this value the the time until the "
			"LFO starts to oscillate." ) );


	m_lfoAttackKnob = new knob( knobBright_26, this );
	m_lfoAttackKnob->setLabel( tr( "ATT" ) );
	m_lfoAttackKnob->move( LFO_ATTACK_KNOB_X, LFO_KNOB_Y );
	m_lfoAttackKnob->setHintText( tr( "LFO- attack:" ) + " ", "" );
	m_lfoAttackKnob->setWhatsThis(
		tr( "Use this knob for setting attack-time of the current LFO. "
			"The bigger this value the longer the LFO needs to "
			"increase its amplitude to maximum." ) );


	m_lfoSpeedKnob = new TempoSyncKnob( knobBright_26, this );
	m_lfoSpeedKnob->setLabel( tr( "SPD" ) );
	m_lfoSpeedKnob->move( LFO_SPEED_KNOB_X, LFO_KNOB_Y );
	m_lfoSpeedKnob->setHintText( tr( "LFO speed:" ) + " ", "" );
	m_lfoSpeedKnob->setWhatsThis(
		tr( "Use this knob for setting speed of the current LFO. The "
			"bigger this value the faster the LFO oscillates and "
			"the faster will be your effect." ) );


	m_lfoAmountKnob = new knob( knobBright_26, this );
	m_lfoAmountKnob->setLabel( tr( "AMT" ) );
	m_lfoAmountKnob->move( LFO_AMOUNT_KNOB_X, LFO_KNOB_Y );
	m_lfoAmountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
	m_lfoAmountKnob->setWhatsThis(
		tr( "Use this knob for setting modulation amount of the "
			"current LFO. The bigger this value the more the "
			"selected size (e.g. volume or cutoff-frequency) will "
			"be influenced by this LFO." ) );


	pixmapButton * sin_lfo_btn = new pixmapButton( this, NULL );
	sin_lfo_btn->move( LFO_SHAPES_X, LFO_SHAPES_Y );
	sin_lfo_btn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
	sin_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );
	sin_lfo_btn->setWhatsThis(
		tr( "Click here for a sine-wave." ) );

	pixmapButton * triangle_lfo_btn = new pixmapButton( this, NULL );
	triangle_lfo_btn->move( LFO_SHAPES_X+15, LFO_SHAPES_Y );
	triangle_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"triangle_wave_active" ) );
	triangle_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"triangle_wave_inactive" ) );
	triangle_lfo_btn->setWhatsThis(
		tr( "Click here for a triangle-wave." ) );

	pixmapButton * saw_lfo_btn = new pixmapButton( this, NULL );
	saw_lfo_btn->move( LFO_SHAPES_X+30, LFO_SHAPES_Y );
	saw_lfo_btn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
	saw_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );
	saw_lfo_btn->setWhatsThis(
		tr( "Click here for a saw-wave for current." ) );

	pixmapButton * sqr_lfo_btn = new pixmapButton( this, NULL );
	sqr_lfo_btn->move( LFO_SHAPES_X+45, LFO_SHAPES_Y );
	sqr_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
	sqr_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );
	sqr_lfo_btn->setWhatsThis(
		tr( "Click here for a square-wave." ) );

	m_userLfoBtn = new pixmapButton( this, NULL );
	m_userLfoBtn->move( LFO_SHAPES_X+60, LFO_SHAPES_Y );
	m_userLfoBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
	m_userLfoBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
	m_userLfoBtn->setWhatsThis(
		tr( "Click here for a user-defined wave. "
			"Afterwards, drag an according sample-"
			"file onto the LFO graph." ) );

	connect( m_userLfoBtn, SIGNAL( toggled( bool ) ),
				this, SLOT( lfoUserWaveChanged() ) );

	m_lfoWaveBtnGrp = new automatableButtonGroup( this );
	m_lfoWaveBtnGrp->addButton( sin_lfo_btn );
	m_lfoWaveBtnGrp->addButton( triangle_lfo_btn );
	m_lfoWaveBtnGrp->addButton( saw_lfo_btn );
	m_lfoWaveBtnGrp->addButton( sqr_lfo_btn );
	m_lfoWaveBtnGrp->addButton( m_userLfoBtn );


	m_x100Cb = new ledCheckBox( tr( "FREQ x 100" ), this );
	m_x100Cb->setFont( pointSize<6>( m_x100Cb->font() ) );
	m_x100Cb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 36 );
	m_x100Cb->setWhatsThis(
		tr( "Click here if the frequency of this LFO should be "
						"multiplied by 100." ) );
	toolTip::add( m_x100Cb, tr( "multiply LFO-frequency by 100" ) );


	m_controlEnvAmountCb = new ledCheckBox( tr( "MODULATE ENV-AMOUNT" ),
			this );
	m_controlEnvAmountCb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 54 );
	m_controlEnvAmountCb->setFont( pointSize<6>(
					m_controlEnvAmountCb->font() ) );
	m_controlEnvAmountCb ->setWhatsThis(
		tr( "Click here to make the envelope-amount controlled by this "
								"LFO." ) );
	toolTip::add( m_controlEnvAmountCb,
				tr( "control envelope-amount by this LFO" ) );


	setAcceptDrops( true );

}




EnvelopeAndLfoView::~EnvelopeAndLfoView()
{
	delete m_lfoWaveBtnGrp;
}




void EnvelopeAndLfoView::modelChanged()
{
	m_params = castModel<EnvelopeAndLfoParameters>();
	m_predelayKnob->setModel( &m_params->m_predelayModel );
	m_attackKnob->setModel( &m_params->m_attackModel );
	m_holdKnob->setModel( &m_params->m_holdModel );
	m_decayKnob->setModel( &m_params->m_decayModel );
	m_sustainKnob->setModel( &m_params->m_sustainModel );
	m_releaseKnob->setModel( &m_params->m_releaseModel );
	m_amountKnob->setModel( &m_params->m_amountModel );
	m_lfoPredelayKnob->setModel( &m_params->m_lfoPredelayModel );
	m_lfoAttackKnob->setModel( &m_params->m_lfoAttackModel );
	m_lfoSpeedKnob->setModel( &m_params->m_lfoSpeedModel );
	m_lfoAmountKnob->setModel( &m_params->m_lfoAmountModel );
	m_lfoWaveBtnGrp->setModel( &m_params->m_lfoWaveModel );
	m_x100Cb->setModel( &m_params->m_x100Model );
	m_controlEnvAmountCb->setModel( &m_params->m_controlEnvAmountModel );
}




void EnvelopeAndLfoView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() != Qt::LeftButton )
	{
		return;
	}

	if( QRect( ENV_GRAPH_X, ENV_GRAPH_Y, s_envGraph->width(),
			s_envGraph->height() ).contains( _me->pos() ) == true )
	{
		if( m_amountKnob->value<float>() < 1.0f )
		{
			m_amountKnob->setValue( 1.0f );
		}
		else
		{
			m_amountKnob->setValue( 0.0f );
		}
	}
	else if( QRect( LFO_GRAPH_X, LFO_GRAPH_Y, s_lfoGraph->width(),
			s_lfoGraph->height() ).contains( _me->pos() ) == true )
	{
		if( m_lfoAmountKnob->value<float>() < 1.0f )
		{
			m_lfoAmountKnob->setValue( 1.0f );
		}
		else
		{
			m_lfoAmountKnob->setValue( 0.0f );
		}
	}
}




void EnvelopeAndLfoView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee,
					QString( "samplefile,tco_%1" ).arg(
							track::SampleTrack ) );
}




void EnvelopeAndLfoView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		m_params->m_userWave.setAudioFile(
					stringPairDrag::decodeValue( _de ) );
		m_userLfoBtn->model()->setValue( true );
		_de->accept();
	}
	else if( type == QString( "tco_%1" ).arg( track::SampleTrack ) )
	{
		multimediaProject mmp( value.toUtf8() );
		m_params->m_userWave.setAudioFile(
				mmp.content().firstChild().toElement().
							attribute( "src" ) );
		m_userLfoBtn->model()->setValue( true );
		_de->accept();
	}
}




void EnvelopeAndLfoView::paintEvent( QPaintEvent * )
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

	LmmsStyle * style = engine::getLmmsStyle();
	QColor lineColor = style->color(LmmsStyle::StandardGraphLine);
	const float gray_amount = fabsf( m_amountKnob->value<float>() );

	p.setPen( QPen( QColor::fromHsvF(
			lineColor.hueF(),
			lineColor.saturationF() * gray_amount,
			lineColor.valueF() * (0.5+gray_amount*0.5) ), 2 ) );

	const QColor endPointsColor = style->color( LmmsStyle::StandardGraphHandle );
	const QColor endPointsBorderColor = style->color(LmmsStyle::StandardGraphHandleBorder );

	const int yBase = ENV_GRAPH_Y + s_envGraph->height() - 3;
	const int availHeight = s_envGraph->height() - 6;

	int x1 = ENV_GRAPH_X + 2 + static_cast<int>( m_predelayKnob->value<float>() *
							TIME_UNIT_WIDTH );
	int x2 = x1 + static_cast<int>( m_attackKnob->value<float>() *
							TIME_UNIT_WIDTH );

	p.drawLine( x1, yBase, x2, yBase - availHeight );
	p.fillRect( x1 - 1, yBase - 2, 4, 4, endPointsBorderColor );
	p.fillRect( x1, yBase - 1, 2, 2, endPointsColor );
	x1 = x2;
	x2 = x1 + static_cast<int>( m_holdKnob->value<float>() * TIME_UNIT_WIDTH );

	p.drawLine( x1, yBase - availHeight, x2, yBase - availHeight );
	p.fillRect( x1 - 1, yBase - 2 - availHeight, 4, 4,
							endPointsBorderColor );
	p.fillRect( x1, yBase-1-availHeight, 2, 2, endPointsColor );
	x1 = x2;
	x2 = x1 + static_cast<int>( ( m_decayKnob->value<float>() *
						m_sustainKnob->value<float>() ) *
							TIME_UNIT_WIDTH );

	p.drawLine( x1, yBase-availHeight, x2, static_cast<int>( yBase -
								availHeight +
				m_sustainKnob->value<float>() * availHeight ) );
	p.fillRect( x1 - 1, yBase - 2 - availHeight, 4, 4,
							endPointsBorderColor );
	p.fillRect( x1, yBase - 1 - availHeight, 2, 2, endPointsColor );
	x1 = x2;
	x2 = x1 + static_cast<int>( m_releaseKnob->value<float>() * TIME_UNIT_WIDTH );

	p.drawLine( x1, static_cast<int>( yBase - availHeight +
						m_sustainKnob->value<float>() *
						availHeight ), x2, yBase );
	p.fillRect( x1-1, static_cast<int>( yBase - availHeight +
						m_sustainKnob->value<float>() *
						availHeight ) - 2, 4, 4,
							endPointsBorderColor );
	p.fillRect( x1, static_cast<int>( yBase - availHeight +
						m_sustainKnob->value<float>() *
						availHeight ) - 1, 2, 2,
							endPointsColor );
	p.fillRect( x2 - 1, yBase - 2, 4, 4, endPointsBorderColor );
	p.fillRect( x2, yBase - 1, 2, 2, endPointsColor );


	int lfoGraphWidth = s_lfoGraph->width() - 6;	// substract border
	int lfoGraphHeight = s_lfoGraph->height() - 6;	// substract border
	int graphXBase = LFO_GRAPH_X + 3;
	int graphYBase = LFO_GRAPH_Y + 3 + lfoGraphHeight / 2;

	const float framesForGraph = SECS_PER_LFO_OSCILLATION *
				engine::mixer()->baseSampleRate() / 10;

	const float lfoGrayAmount = fabsf( m_lfoAmountKnob->value<float>() );
	p.setPen( QPen( QColor::fromHsvF(
			lineColor.hueF(),
			lineColor.saturationF() * lfoGrayAmount,
			lineColor.valueF() * (0.5+lfoGrayAmount*0.5) ), 1.5 ) );


	float oscFrames = m_params->m_lfoOscillationFrames;

	if( m_params->m_x100Model.value() )
	{
		oscFrames *= 100.0f;
	}

	float oldY = 0;
	for( int x = 0; x <= lfoGraphWidth; ++x )
	{
		float val = 0.0;
		float curSample = x * framesForGraph / lfoGraphWidth;
		if( static_cast<f_cnt_t>( curSample ) >
						m_params->m_lfoPredelayFrames )
		{
			float phase = ( curSample -=
					m_params->m_lfoPredelayFrames ) /
								oscFrames;
			switch( m_params->m_lfoWaveModel.value() )
			{
				case EnvelopeAndLfoParameters::SineWave:
					val = Oscillator::sinSample( phase );
					break;
				case EnvelopeAndLfoParameters::TriangleWave:
					val = Oscillator::triangleSample(
								phase );
					break;
				case EnvelopeAndLfoParameters::SawWave:
					val = Oscillator::sawSample( phase );
					break;
				case EnvelopeAndLfoParameters::SquareWave:
					val = Oscillator::squareSample( phase );
					break;
				case EnvelopeAndLfoParameters::UserDefinedWave:
					val = m_params->m_userWave.
							userWaveSample( phase );
			}
			if( static_cast<f_cnt_t>( curSample ) <=
						m_params->m_lfoAttackFrames )
			{
				val *= curSample / m_params->m_lfoAttackFrames;
			}
		}
		float curY = -lfoGraphHeight / 2.0f * val;
		p.drawLine( QLineF( graphXBase + x - 1, graphYBase + oldY,
						graphXBase + x,
						graphYBase + curY ) );
		oldY = curY;
	}

	p.setPen( endPointsColor );
	int msPerOsc = static_cast<int>( SECS_PER_LFO_OSCILLATION *
						m_lfoSpeedKnob->value<float>() *
								1000.0f );
	p.drawText( LFO_GRAPH_X + 4, LFO_GRAPH_Y + s_lfoGraph->height() - 6,
							tr( "ms/LFO:" ) );
	p.drawText( LFO_GRAPH_X + 52, LFO_GRAPH_Y + s_lfoGraph->height() - 6,
						QString::number( msPerOsc ) );

}




void EnvelopeAndLfoView::lfoUserWaveChanged()
{
	if( m_params->m_lfoWaveModel.value() ==
				EnvelopeAndLfoParameters::UserDefinedWave )
	{
		if( m_params->m_userWave.frames() <= 1 )
		{
			textFloat::displayMessage( tr( "Hint" ),
				tr( "Drag a sample from somewhere and drop "
					"it in this window." ),
					embed::getIconPixmap( "hint" ), 3000 );
		}
	}
}




#include "moc_EnvelopeAndLfoView.cxx"


