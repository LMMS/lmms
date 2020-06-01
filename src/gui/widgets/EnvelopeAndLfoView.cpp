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

#include <QMouseEvent>
#include <QPainter>

#include "EnvelopeAndLfoView.h"
#include "EnvelopeAndLfoParameters.h"
#include "embed.h"
#include "Engine.h"
#include "gui_templates.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "Mixer.h"
#include "DataFile.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "StringPairDrag.h"
#include "TempoSyncKnob.h"
#include "TextFloat.h"
#include "ToolTip.h"
#include "Track.h"


extern const float SECS_PER_ENV_SEGMENT;
extern const float SECS_PER_LFO_OSCILLATION;


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

const int TIME_UNIT_WIDTH = 40;

const int LFO_GRAPH_X = 6;
const int LFO_GRAPH_Y = ENV_KNOBS_LBL_Y+14;
const int LFO_KNOB_Y = LFO_GRAPH_Y-2;
const int LFO_PREDELAY_KNOB_X = LFO_GRAPH_X + 100;
const int LFO_ATTACK_KNOB_X = LFO_PREDELAY_KNOB_X+KNOB_X_SPACING;
const int LFO_SPEED_KNOB_X = LFO_ATTACK_KNOB_X+KNOB_X_SPACING;
const int LFO_AMOUNT_KNOB_X = LFO_SPEED_KNOB_X+KNOB_X_SPACING;
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

	m_predelayKnob = new Knob( knobBright_26, this );
	m_predelayKnob->setLabel( tr( "DEL" ) );
	m_predelayKnob->move( PREDELAY_KNOB_X, ENV_KNOBS_Y );
	m_predelayKnob->setHintText( tr( "Pre-delay:" ), "" );


	m_attackKnob = new Knob( knobBright_26, this );
	m_attackKnob->setLabel( tr( "ATT" ) );
	m_attackKnob->move( ATTACK_KNOB_X, ENV_KNOBS_Y );
	m_attackKnob->setHintText( tr( "Attack:" ), "" );


	m_holdKnob = new Knob( knobBright_26, this );
	m_holdKnob->setLabel( tr( "HOLD" ) );
	m_holdKnob->move( HOLD_KNOB_X, ENV_KNOBS_Y );
	m_holdKnob->setHintText( tr( "Hold:" ), "" );


	m_decayKnob = new Knob( knobBright_26, this );
	m_decayKnob->setLabel( tr( "DEC" ) );
	m_decayKnob->move( DECAY_KNOB_X, ENV_KNOBS_Y );
	m_decayKnob->setHintText( tr( "Decay:" ), "" );


	m_sustainKnob = new Knob( knobBright_26, this );
	m_sustainKnob->setLabel( tr( "SUST" ) );
	m_sustainKnob->move( SUSTAIN_KNOB_X, ENV_KNOBS_Y );
	m_sustainKnob->setHintText( tr( "Sustain:" ), "" );


	m_releaseKnob = new Knob( knobBright_26, this );
	m_releaseKnob->setLabel( tr( "REL" ) );
	m_releaseKnob->move( RELEASE_KNOB_X, ENV_KNOBS_Y );
    m_releaseKnob->setHintText( tr( "Release:" ), "" );


	m_amountKnob = new Knob( knobBright_26, this );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->move( AMOUNT_KNOB_X, ENV_GRAPH_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ), "" );




	m_lfoPredelayKnob = new Knob( knobBright_26, this );
	m_lfoPredelayKnob->setLabel( tr( "DEL" ) );
	m_lfoPredelayKnob->move( LFO_PREDELAY_KNOB_X, LFO_KNOB_Y );
	m_lfoPredelayKnob->setHintText( tr( "Pre-delay:" ), "" );


	m_lfoAttackKnob = new Knob( knobBright_26, this );
	m_lfoAttackKnob->setLabel( tr( "ATT" ) );
	m_lfoAttackKnob->move( LFO_ATTACK_KNOB_X, LFO_KNOB_Y );
	m_lfoAttackKnob->setHintText( tr( "Attack:" ), "" );


	m_lfoSpeedKnob = new TempoSyncKnob( knobBright_26, this );
	m_lfoSpeedKnob->setLabel( tr( "SPD" ) );
	m_lfoSpeedKnob->move( LFO_SPEED_KNOB_X, LFO_KNOB_Y );
	m_lfoSpeedKnob->setHintText( tr( "Frequency:" ), "" );


	m_lfoAmountKnob = new Knob( knobBright_26, this );
	m_lfoAmountKnob->setLabel( tr( "AMT" ) );
	m_lfoAmountKnob->move( LFO_AMOUNT_KNOB_X, LFO_KNOB_Y );
	m_lfoAmountKnob->setHintText( tr( "Modulation amount:" ), "" );


	PixmapButton * sin_lfo_btn = new PixmapButton( this, NULL );
	sin_lfo_btn->move( LFO_SHAPES_X, LFO_SHAPES_Y );
	sin_lfo_btn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
	sin_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );

	PixmapButton * triangle_lfo_btn = new PixmapButton( this, NULL );
	triangle_lfo_btn->move( LFO_SHAPES_X+15, LFO_SHAPES_Y );
	triangle_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"triangle_wave_active" ) );
	triangle_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"triangle_wave_inactive" ) );

	PixmapButton * saw_lfo_btn = new PixmapButton( this, NULL );
	saw_lfo_btn->move( LFO_SHAPES_X+30, LFO_SHAPES_Y );
	saw_lfo_btn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
	saw_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );

	PixmapButton * sqr_lfo_btn = new PixmapButton( this, NULL );
	sqr_lfo_btn->move( LFO_SHAPES_X+45, LFO_SHAPES_Y );
	sqr_lfo_btn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
	sqr_lfo_btn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );

	m_userLfoBtn = new PixmapButton( this, NULL );
	m_userLfoBtn->move( LFO_SHAPES_X+75, LFO_SHAPES_Y );
	m_userLfoBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
	m_userLfoBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );

	connect( m_userLfoBtn, SIGNAL( toggled( bool ) ),
				this, SLOT( lfoUserWaveChanged() ) );

	PixmapButton * random_lfo_btn = new PixmapButton( this, NULL );
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
	m_x100Cb->setFont( pointSizeF( m_x100Cb->font(), 6.5 ) );
	m_x100Cb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 36 );
	ToolTip::add( m_x100Cb, tr( "Multiply LFO frequency by 100" ) );


	m_controlEnvAmountCb = new LedCheckBox( tr( "MODULATE ENV AMOUNT" ),
			this );
	m_controlEnvAmountCb->move( LFO_PREDELAY_KNOB_X, LFO_GRAPH_Y + 54 );
	m_controlEnvAmountCb->setFont( pointSizeF( m_controlEnvAmountCb->font(), 6.5 ) );
	ToolTip::add( m_controlEnvAmountCb,
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
		if( m_params->m_amountModel.value() < 1.0f )
		{
			m_params->m_amountModel.setValue( 1.0f );
		}
		else
		{
			m_params->m_amountModel.setValue( 0.0f );
		}
	}
	else if( QRect( LFO_GRAPH_X, LFO_GRAPH_Y, s_lfoGraph->width(),
			s_lfoGraph->height() ).contains( _me->pos() ) == true )
	{
		if( m_params->m_lfoAmountModel.value() < 1.0f )
		{
			m_params->m_lfoAmountModel.setValue( 1.0f );
		}
		else
		{
			m_params->m_lfoAmountModel.setValue( 0.0f );
		}
	}
}




void EnvelopeAndLfoView::dragEnterEvent( QDragEnterEvent * _dee )
{
	StringPairDrag::processDragEnterEvent( _dee,
					QString( "samplefile,tco_%1" ).arg(
							Track::SampleTrack ) );
}




void EnvelopeAndLfoView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		m_params->m_userWave.setAudioFile(
					StringPairDrag::decodeValue( _de ) );
		m_userLfoBtn->model()->setValue( true );
		m_params->m_lfoWaveModel.setValue(EnvelopeAndLfoParameters::UserDefinedWave);
		_de->accept();
		update();
	}
	else if( type == QString( "tco_%1" ).arg( Track::SampleTrack ) )
	{
		DataFile dataFile( value.toUtf8() );
		m_params->m_userWave.setAudioFile( dataFile.content().
					firstChildElement().firstChildElement().
					firstChildElement().attribute( "src" ) );
		m_userLfoBtn->model()->setValue( true );
		m_params->m_lfoWaveModel.setValue(EnvelopeAndLfoParameters::UserDefinedWave);
		_de->accept();
		update();
	}
}




void EnvelopeAndLfoView::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );

	// draw envelope-graph
	p.drawPixmap( ENV_GRAPH_X, ENV_GRAPH_Y, *s_envGraph );
	// draw LFO-graph
	p.drawPixmap( LFO_GRAPH_X, LFO_GRAPH_Y, *s_lfoGraph );


	p.setFont( pointSize<8>( p.font() ) );

	const float gray_amount = 1.0f - fabsf( m_amountKnob->value<float>() );

	p.setPen( QPen( QColor( static_cast<int>( 96 * gray_amount ),
				static_cast<int>( 255 - 159 * gray_amount ),
				static_cast<int>( 128 - 32 * gray_amount ) ),
									2 ) );

	const QColor end_points_color( 0x99, 0xAF, 0xFF );
	const QColor end_points_bg_color( 0, 0, 2 );

	const int y_base = ENV_GRAPH_Y + s_envGraph->height() - 3;
	const int avail_height = s_envGraph->height() - 6;
	
	int x1 = static_cast<int>( m_predelayKnob->value<float>() * TIME_UNIT_WIDTH );
	int x2 = x1 + static_cast<int>( m_attackKnob->value<float>() * TIME_UNIT_WIDTH );
	int x3 = x2 + static_cast<int>( m_holdKnob->value<float>() * TIME_UNIT_WIDTH );
	int x4 = x3 + static_cast<int>( ( m_decayKnob->value<float>() *
						( 1 - m_sustainKnob->value<float>() ) ) * TIME_UNIT_WIDTH );
	int x5 = x4 + static_cast<int>( m_releaseKnob->value<float>() * TIME_UNIT_WIDTH );

	if( x5 > 174 )
	{
		x1 = ( x1 * 174 ) / x5;
		x2 = ( x2 * 174 ) / x5;
		x3 = ( x3 * 174 ) / x5;
		x4 = ( x4 * 174 ) / x5;
		x5 = ( x5 * 174 ) / x5;
	}
	x1 += ENV_GRAPH_X + 2;
	x2 += ENV_GRAPH_X + 2;
	x3 += ENV_GRAPH_X + 2;
	x4 += ENV_GRAPH_X + 2;
	x5 += ENV_GRAPH_X + 2;

	p.drawLine( x1, y_base, x2, y_base - avail_height );
	p.fillRect( x1 - 1, y_base - 2, 4, 4, end_points_bg_color );
	p.fillRect( x1, y_base - 1, 2, 2, end_points_color );

	p.drawLine( x2, y_base - avail_height, x3, y_base - avail_height );
	p.fillRect( x2 - 1, y_base - 2 - avail_height, 4, 4,
							end_points_bg_color );
	p.fillRect( x2, y_base - 1 - avail_height, 2, 2, end_points_color );

	p.drawLine( x3, y_base-avail_height, x4, static_cast<int>( y_base -
								avail_height +
				( 1 - m_sustainKnob->value<float>() ) * avail_height ) );
	p.fillRect( x3 - 1, y_base - 2 - avail_height, 4, 4,
							end_points_bg_color );
	p.fillRect( x3, y_base - 1 - avail_height, 2, 2, end_points_color );
	
	p.drawLine( x4, static_cast<int>( y_base - avail_height +
						( 1 - m_sustainKnob->value<float>() ) *
						avail_height ), x5, y_base );
	p.fillRect( x4 - 1, static_cast<int>( y_base - avail_height +
						( 1 - m_sustainKnob->value<float>() ) *
						avail_height ) - 2, 4, 4,
							end_points_bg_color );
	p.fillRect( x4, static_cast<int>( y_base - avail_height +
						( 1 - m_sustainKnob->value<float>() ) *
						avail_height ) - 1, 2, 2,
							end_points_color );
	p.fillRect( x5 - 1, y_base - 2, 4, 4, end_points_bg_color );
	p.fillRect( x5, y_base - 1, 2, 2, end_points_color );


	int LFO_GRAPH_W = s_lfoGraph->width() - 3;      // subtract border
	int LFO_GRAPH_H = s_lfoGraph->height() - 6;	// subtract border
	int graph_x_base = LFO_GRAPH_X + 2;
	int graph_y_base = LFO_GRAPH_Y + 3 + LFO_GRAPH_H / 2;

	const float frames_for_graph = SECS_PER_LFO_OSCILLATION *
				Engine::mixer()->baseSampleRate() / 10;

	const float lfo_gray_amount = 1.0f - fabsf( m_lfoAmountKnob->value<float>() );
	p.setPen( QPen( QColor( static_cast<int>( 96 * lfo_gray_amount ),
				static_cast<int>( 255 - 159 * lfo_gray_amount ),
				static_cast<int>( 128 - 32 *
							lfo_gray_amount ) ),
									1.5 ) );


	float osc_frames = m_params->m_lfoOscillationFrames;

	if( m_params->m_x100Model.value() )
	{
		osc_frames *= 100.0f;
	}

	// userWaveSample() may be used, called out of loop for efficiency
	m_params->m_userWave.dataReadLock();
	float old_y = 0;
	for( int x = 0; x <= LFO_GRAPH_W; ++x )
	{
		float val = 0.0;
		float cur_sample = x * frames_for_graph / LFO_GRAPH_W;
		if( static_cast<f_cnt_t>( cur_sample ) >
						m_params->m_lfoPredelayFrames )
		{
			float phase = ( cur_sample -=
					m_params->m_lfoPredelayFrames ) /
								osc_frames;
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
				case EnvelopeAndLfoParameters::RandomWave:
					if( x % (int)( 900 * m_lfoSpeedKnob->value<float>() + 1 ) == 0 )
					{
						m_randomGraph = Oscillator::noiseSample( 0.0f );
					}
					val = m_randomGraph;
					break;
				case EnvelopeAndLfoParameters::UserDefinedWave:
					val = m_params->m_userWave.
							userWaveSample( phase );
					break;
			}
			if( static_cast<f_cnt_t>( cur_sample ) <=
						m_params->m_lfoAttackFrames )
			{
				val *= cur_sample / m_params->m_lfoAttackFrames;
			}
		}
		float cur_y = -LFO_GRAPH_H / 2.0f * val;
		p.drawLine( QLineF( graph_x_base + x - 1, graph_y_base + old_y,
						graph_x_base + x,
						graph_y_base + cur_y ) );
		old_y = cur_y;
	}
	m_params->m_userWave.dataUnlock();

	p.setPen( QColor( 201, 201, 225 ) );
	int ms_per_osc = static_cast<int>( SECS_PER_LFO_OSCILLATION *
						m_lfoSpeedKnob->value<float>() *
								1000.0f );
	p.drawText( LFO_GRAPH_X + 4, LFO_GRAPH_Y + s_lfoGraph->height() - 6,
							tr( "ms/LFO:" ) );
	p.drawText( LFO_GRAPH_X + 52, LFO_GRAPH_Y + s_lfoGraph->height() - 6,
						QString::number( ms_per_osc ) );

}




void EnvelopeAndLfoView::lfoUserWaveChanged()
{
	if( m_params->m_lfoWaveModel.value() ==
				EnvelopeAndLfoParameters::UserDefinedWave )
	{
		if( m_params->m_userWave.frames() <= 1 )
		{
			TextFloat::displayMessage( tr( "Hint" ),
				tr( "Drag and drop a sample into this window." ),
					embed::getIconPixmap( "hint" ), 3000 );
		}
	}
}







