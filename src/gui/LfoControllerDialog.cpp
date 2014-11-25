/*
 * LfoControllerDialog.cpp - per-controller-specific view for changing a
 *                           controller's settings
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
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

#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>

#include "caption_menu.h"
#include "gui_templates.h"
#include "embed.h"
#include "engine.h"
#include "led_checkbox.h"
#include "MainWindow.h"
#include "tooltip.h"


#include "LfoController.h"
#include "ControllerDialog.h"
#include "knob.h"
#include "TempoSyncKnob.h"
#include "pixmap_button.h"

const int CD_ENV_KNOBS_LBL_Y = 20;
const int CD_KNOB_X_SPACING = 32;

const int CD_LFO_SHAPES_X = 6;
const int CD_LFO_SHAPES_Y = 36;

const int CD_LFO_GRAPH_X = 6;
const int CD_LFO_GRAPH_Y = CD_ENV_KNOBS_LBL_Y+15;
const int CD_LFO_CD_KNOB_Y = CD_LFO_GRAPH_Y-2;
const int CD_LFO_BASE_CD_KNOB_X = CD_LFO_SHAPES_X + 64;
const int CD_LFO_SPEED_CD_KNOB_X = CD_LFO_BASE_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_AMOUNT_CD_KNOB_X = CD_LFO_SPEED_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_PHASE_CD_KNOB_X = CD_LFO_AMOUNT_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_MULTIPLIER_X = CD_LFO_PHASE_CD_KNOB_X+CD_KNOB_X_SPACING;

LfoControllerDialog::LfoControllerDialog( Controller * _model, QWidget * _parent ) :
	ControllerDialog( _model, _parent )
{
	QString title = tr( "LFO" );
	title.append( " (" );
	title.append( _model->name() );
	title.append( ")" );
	setWindowTitle( title );
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setFixedSize( 240, 80 );
	
	toolTip::add( this, tr( "LFO Controller" ) );

	m_baseKnob = new knob( knobBright_26, this );
	m_baseKnob->setLabel( tr( "BASE" ) );
	m_baseKnob->move( CD_LFO_BASE_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_baseKnob->setHintText( tr( "Base amount:" ) + " ", "" );
	m_baseKnob->setWhatsThis( tr("todo") );


	m_speedKnob = new TempoSyncKnob( knobBright_26, this );
	m_speedKnob->setLabel( tr( "SPD" ) );
	m_speedKnob->move( CD_LFO_SPEED_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_speedKnob->setHintText( tr( "LFO-speed:" ) + " ", "" );
	m_speedKnob->setWhatsThis(
		tr( "Use this knob for setting speed of the LFO. The "
			"bigger this value the faster the LFO oscillates and "
			"the faster the effect." ) );


	m_amountKnob = new knob( knobBright_26, this );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->move( CD_LFO_AMOUNT_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
	m_amountKnob->setWhatsThis(
		tr( "Use this knob for setting modulation amount of the "
			"LFO. The bigger this value, the more the connected "
			"control (e.g. volume or cutoff-frequency) will "
			"be influenced by the LFO." ) );

	m_phaseKnob = new knob( knobBright_26, this );
	m_phaseKnob->setLabel( tr( "PHS" ) );
	m_phaseKnob->move( CD_LFO_PHASE_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_phaseKnob->setHintText( tr( "Phase offset:" ) + " ", "" + tr( "degrees" ) );
	m_phaseKnob->setWhatsThis(
			tr( "With this knob you can set the phase offset of "
				"the LFO. That means you can move the "
				"point within an oscillation where the "
				"oscillator begins to oscillate. For example "
				"if you have a sine-wave and have a phase-"
				"offset of 180 degrees the wave will first go "
				"down. It's the same with a square-wave."
				) );

	pixmapButton * sin_wave_btn = new pixmapButton( this, NULL );
	sin_wave_btn->move( CD_LFO_SHAPES_X, CD_LFO_SHAPES_Y );
	sin_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"sin_wave_active" ) );
	sin_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"sin_wave_inactive" ) );
	toolTip::add( sin_wave_btn,
			tr( "Click here for a sine-wave." ) );

	pixmapButton * triangle_wave_btn =
					new pixmapButton( this, NULL );
	triangle_wave_btn->move( CD_LFO_SHAPES_X + 15, CD_LFO_SHAPES_Y );
	triangle_wave_btn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	triangle_wave_btn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	toolTip::add( triangle_wave_btn,
			tr( "Click here for a triangle-wave." ) );

	pixmapButton * saw_wave_btn = new pixmapButton( this, NULL );
	saw_wave_btn->move( CD_LFO_SHAPES_X + 30, CD_LFO_SHAPES_Y );
	saw_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"saw_wave_active" ) );
	saw_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"saw_wave_inactive" ) );
	toolTip::add( saw_wave_btn,
			tr( "Click here for a saw-wave." ) );

	pixmapButton * sqr_wave_btn = new pixmapButton( this, NULL );
	sqr_wave_btn->move( CD_LFO_SHAPES_X + 45, CD_LFO_SHAPES_Y );
	sqr_wave_btn->setActiveGraphic( embed::getIconPixmap(
					"square_wave_active" ) );
	sqr_wave_btn->setInactiveGraphic( embed::getIconPixmap(
					"square_wave_inactive" ) );
	toolTip::add( sqr_wave_btn,
			tr( "Click here for a square-wave." ) );

	pixmapButton * moog_saw_wave_btn =
					new pixmapButton( this, NULL );
	moog_saw_wave_btn->move( CD_LFO_SHAPES_X, CD_LFO_SHAPES_Y + 15 );
	moog_saw_wave_btn->setActiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_active" ) );
	moog_saw_wave_btn->setInactiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_inactive" ) );
	toolTip::add( moog_saw_wave_btn,
			tr( "Click here for a moog saw-wave." ) );

	pixmapButton * exp_wave_btn = new pixmapButton( this, NULL );
	exp_wave_btn->move( CD_LFO_SHAPES_X + 15, CD_LFO_SHAPES_Y + 15 );
	exp_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"exp_wave_active" ) );
	exp_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"exp_wave_inactive" ) );
	toolTip::add( exp_wave_btn,
			tr( "Click here for an exponential wave." ) );

	pixmapButton * white_noise_btn = new pixmapButton( this, NULL );
	white_noise_btn->move( CD_LFO_SHAPES_X + 30, CD_LFO_SHAPES_Y + 15 );
	white_noise_btn->setActiveGraphic(
		embed::getIconPixmap( "white_noise_wave_active" ) );
	white_noise_btn->setInactiveGraphic(
		embed::getIconPixmap( "white_noise_wave_inactive" ) );
	toolTip::add( white_noise_btn,
				tr( "Click here for white-noise." ) );

	m_userWaveBtn = new pixmapButton( this, NULL );
	m_userWaveBtn->move( CD_LFO_SHAPES_X + 45, CD_LFO_SHAPES_Y + 15 );
	m_userWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	m_userWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	connect( m_userWaveBtn,
					SIGNAL( doubleClicked() ),
			this, SLOT( askUserDefWave() ) );
	toolTip::add( m_userWaveBtn,
				tr( "Click here for a user-defined shape.\nDouble click to pick a file." ) );
	
	m_waveBtnGrp = new automatableButtonGroup( this );
	m_waveBtnGrp->addButton( sin_wave_btn );
	m_waveBtnGrp->addButton( triangle_wave_btn );
	m_waveBtnGrp->addButton( saw_wave_btn );
	m_waveBtnGrp->addButton( sqr_wave_btn );
	m_waveBtnGrp->addButton( moog_saw_wave_btn );
	m_waveBtnGrp->addButton( exp_wave_btn );
	m_waveBtnGrp->addButton( white_noise_btn );
	m_waveBtnGrp->addButton( m_userWaveBtn );


	pixmapButton * x1 = new pixmapButton( this, NULL );
	x1->move( CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y );
	x1->setActiveGraphic( embed::getIconPixmap(
						"lfo_x1_active" ) );
	x1->setInactiveGraphic( embed::getIconPixmap(
						"lfo_x1_inactive" ) );

	pixmapButton * x100 = new pixmapButton( this, NULL );
	x100->move( CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y - 15 );
	x100->setActiveGraphic( embed::getIconPixmap(
						"lfo_x100_active" ) );
	x100->setInactiveGraphic( embed::getIconPixmap(
						"lfo_x100_inactive" ) );

	pixmapButton * d100 = new pixmapButton( this, NULL );
	d100->move( CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y + 15 );
	d100->setActiveGraphic( embed::getIconPixmap(
						"lfo_d100_active" ) );
	d100->setInactiveGraphic( embed::getIconPixmap(
						"lfo_d100_inactive" ) );

	m_multiplierBtnGrp = new automatableButtonGroup( this );
	m_multiplierBtnGrp->addButton( x1 );
	m_multiplierBtnGrp->addButton( x100 );
	m_multiplierBtnGrp->addButton( d100 );


	setModel( _model );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
					embed::getIconPixmap( "lfo_controller_artwork" ) );
	setPalette( pal );

}



LfoControllerDialog::~LfoControllerDialog()
{
	m_userWaveBtn->disconnect( this );
	//delete m_subWindow;
}



void LfoControllerDialog::askUserDefWave()
{
	SampleBuffer * sampleBuffer = dynamic_cast<LfoController*>(this->model())->
									m_userDefSampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	if( fileName.isEmpty() == false )
	{
		// TODO:
		toolTip::add( m_userWaveBtn, sampleBuffer->audioFile() );
	}
}



void LfoControllerDialog::contextMenuEvent( QContextMenuEvent * )
{
	/*
	QPointer<captionMenu> contextMenu = new captionMenu(
						getEffect()->publicName() );
	contextMenu->addAction( embed::getIconPixmap( "arp_up_on" ),
						tr( "Move &up" ),
						this, SLOT( moveUp() ) );
	contextMenu->addAction( embed::getIconPixmap( "arp_down_on" ),
						tr( "Move &down" ),
						this, SLOT( moveDown() ) );
	contextMenu->addSeparator();
	contextMenu->addAction( embed::getIconPixmap( "cancel" ),
						tr( "&Remove this plugin" ),
						this, SLOT( deletePlugin() ) );
	contextMenu->addSeparator();
	contextMenu->addAction( embed::getIconPixmap( "help" ),
						tr( "&Help" ),
						this, SLOT( displayHelp() ) );
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
	*/
}


/*
void lfoControllerDialog::paintEvent( QPaintEvent * _pe )
{
	QWidget::paintEvent( _pe );
}
*/


void LfoControllerDialog::modelChanged()
{
	m_lfo = castModel<LfoController>();

	m_baseKnob->setModel( &m_lfo->m_baseModel );
	m_speedKnob->setModel( &m_lfo->m_speedModel );
	m_amountKnob->setModel( &m_lfo->m_amountModel );
	m_phaseKnob->setModel( &m_lfo->m_phaseModel );
	m_waveBtnGrp->setModel( &m_lfo->m_waveModel );
	m_multiplierBtnGrp->setModel( &m_lfo->m_multiplierModel );
}


