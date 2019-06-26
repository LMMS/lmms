/*
 * LfoControllerDialog.cpp - per-controller-specific view for changing a
 *                           controller's settings
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
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

#include <QLabel>
#include <QPushButton>
#include <QMdiArea>
#include <QPainter>

#include "CaptionMenu.h"
#include "gui_templates.h"
#include "embed.h"
#include "LedCheckbox.h"
#include "MainWindow.h"
#include "ToolTip.h"


#include "LfoController.h"
#include "Knob.h"
#include "TempoSyncKnob.h"
#include "PixmapButton.h"

const int CD_ENV_KNOBS_LBL_Y = 11;
const int CD_KNOB_X_SPACING = 32;

const int CD_LFO_SHAPES_X = 6;
const int CD_LFO_SHAPES_Y = CD_ENV_KNOBS_LBL_Y + 4;

const int CD_LFO_GRAPH_Y = CD_ENV_KNOBS_LBL_Y+3;
const int CD_LFO_CD_KNOB_Y = CD_LFO_GRAPH_Y-2;
const int CD_LFO_BASE_CD_KNOB_X = CD_LFO_SHAPES_X + 68;
const int CD_LFO_SPEED_CD_KNOB_X = CD_LFO_BASE_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_AMOUNT_CD_KNOB_X = CD_LFO_SPEED_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_PHASE_CD_KNOB_X = CD_LFO_AMOUNT_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_MULTIPLIER_X = CD_LFO_PHASE_CD_KNOB_X+CD_KNOB_X_SPACING+3;

LfoControllerDialog::LfoControllerDialog( Controller * _model, QWidget * _parent ) :
	ControllerDialog( _model, _parent )
{
	QString title = tr( "LFO" );
	title.append( " (" );
	title.append( _model->name() );
	title.append( ")" );
	setWindowTitle( title );
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setFixedSize( 240, 58 );
	
	m_baseKnob = new Knob( knobBright_26, this );
	m_baseKnob->setLabel( tr( "BASE" ) );
	m_baseKnob->move( CD_LFO_BASE_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_baseKnob->setHintText( tr( "Base:" ), "" );

	m_speedKnob = new TempoSyncKnob( knobBright_26, this );
	m_speedKnob->setLabel( tr( "FREQ" ) );
	m_speedKnob->move( CD_LFO_SPEED_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_speedKnob->setHintText( tr( "LFO frequency:" ), "" );

	m_amountKnob = new Knob( knobBright_26, this );
	m_amountKnob->setLabel( tr( "AMNT" ) );
	m_amountKnob->move( CD_LFO_AMOUNT_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ), "" );

	m_phaseKnob = new Knob( knobBright_26, this );
	m_phaseKnob->setLabel( tr( "PHS" ) );
	m_phaseKnob->move( CD_LFO_PHASE_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_phaseKnob->setHintText( tr( "Phase offset:" ) , "" + tr( " degrees" ) );

	PixmapButton * sin_wave_btn = new PixmapButton( this, NULL );
	sin_wave_btn->move( CD_LFO_SHAPES_X, CD_LFO_SHAPES_Y );
	sin_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"sin_wave_active" ) );
	sin_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"sin_wave_inactive" ) );
	ToolTip::add( sin_wave_btn,
			tr( "Sine wave" ) );

	PixmapButton * triangle_wave_btn =
					new PixmapButton( this, NULL );
	triangle_wave_btn->move( CD_LFO_SHAPES_X + 15, CD_LFO_SHAPES_Y );
	triangle_wave_btn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	triangle_wave_btn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	ToolTip::add( triangle_wave_btn,
			tr( "Triangle wave" ) );

	PixmapButton * saw_wave_btn = new PixmapButton( this, NULL );
	saw_wave_btn->move( CD_LFO_SHAPES_X + 30, CD_LFO_SHAPES_Y );
	saw_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"saw_wave_active" ) );
	saw_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"saw_wave_inactive" ) );
	ToolTip::add( saw_wave_btn,
			tr( "Saw wave" ) );

	PixmapButton * sqr_wave_btn = new PixmapButton( this, NULL );
	sqr_wave_btn->move( CD_LFO_SHAPES_X + 45, CD_LFO_SHAPES_Y );
	sqr_wave_btn->setActiveGraphic( embed::getIconPixmap(
					"square_wave_active" ) );
	sqr_wave_btn->setInactiveGraphic( embed::getIconPixmap(
					"square_wave_inactive" ) );
	ToolTip::add( sqr_wave_btn,
			tr( "Square wave" ) );

	PixmapButton * moog_saw_wave_btn =
					new PixmapButton( this, NULL );
	moog_saw_wave_btn->move( CD_LFO_SHAPES_X, CD_LFO_SHAPES_Y + 15 );
	moog_saw_wave_btn->setActiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_active" ) );
	moog_saw_wave_btn->setInactiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_inactive" ) );
	ToolTip::add( moog_saw_wave_btn,
			tr( "Moog saw wave" ) );

	PixmapButton * exp_wave_btn = new PixmapButton( this, NULL );
	exp_wave_btn->move( CD_LFO_SHAPES_X + 15, CD_LFO_SHAPES_Y + 15 );
	exp_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"exp_wave_active" ) );
	exp_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"exp_wave_inactive" ) );
	ToolTip::add( exp_wave_btn,
			tr( "Exponential wave" ) );

	PixmapButton * white_noise_btn = new PixmapButton( this, NULL );
	white_noise_btn->move( CD_LFO_SHAPES_X + 30, CD_LFO_SHAPES_Y + 15 );
	white_noise_btn->setActiveGraphic(
		embed::getIconPixmap( "white_noise_wave_active" ) );
	white_noise_btn->setInactiveGraphic(
		embed::getIconPixmap( "white_noise_wave_inactive" ) );
	ToolTip::add( white_noise_btn,
				tr( "White noise" ) );

	m_userWaveBtn = new PixmapButton( this, NULL );
	m_userWaveBtn->move( CD_LFO_SHAPES_X + 45, CD_LFO_SHAPES_Y + 15 );
	m_userWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	m_userWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	connect( m_userWaveBtn,
					SIGNAL( doubleClicked() ),
			this, SLOT( askUserDefWave() ) );
	ToolTip::add( m_userWaveBtn,
				tr( "User-defined shape.\nDouble click to pick a file." ) );
	
	m_waveBtnGrp = new automatableButtonGroup( this );
	m_waveBtnGrp->addButton( sin_wave_btn );
	m_waveBtnGrp->addButton( triangle_wave_btn );
	m_waveBtnGrp->addButton( saw_wave_btn );
	m_waveBtnGrp->addButton( sqr_wave_btn );
	m_waveBtnGrp->addButton( moog_saw_wave_btn );
	m_waveBtnGrp->addButton( exp_wave_btn );
	m_waveBtnGrp->addButton( white_noise_btn );
	m_waveBtnGrp->addButton( m_userWaveBtn );


	PixmapButton * x1 = new PixmapButton( this, NULL );
	x1->move( CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y +7);
	x1->setActiveGraphic( embed::getIconPixmap(
						"lfo_x1_active" ) );
	x1->setInactiveGraphic( embed::getIconPixmap(
						"lfo_x1_inactive" ) );
	ToolTip::add( x1,
				tr( "Mutliply modulation frequency by 1" ));

	PixmapButton * x100 = new PixmapButton( this, NULL );
	x100->move( CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y - 8 );
	x100->setActiveGraphic( embed::getIconPixmap(
						"lfo_x100_active" ) );
	x100->setInactiveGraphic( embed::getIconPixmap(
						"lfo_x100_inactive" ) );
	ToolTip::add( x100,
				tr( "Mutliply modulation frequency by 100" ));

	PixmapButton * d100 = new PixmapButton( this, NULL );
	d100->move( CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y + 22 );
	d100->setActiveGraphic( embed::getIconPixmap(
						"lfo_d100_active" ) );
	d100->setInactiveGraphic( embed::getIconPixmap(
						"lfo_d100_inactive" ) );
	ToolTip::add( d100,
				tr( "Divide modulation frequency by 100" ));

	m_multiplierBtnGrp = new automatableButtonGroup( this );
	m_multiplierBtnGrp->addButton( x1 );
	m_multiplierBtnGrp->addButton( x100 );
	m_multiplierBtnGrp->addButton( d100 );


	setModel( _model );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), embed::getIconPixmap( "lfo_controller_artwork" ) );
	setPalette( pal );

}



LfoControllerDialog::~LfoControllerDialog()
{
	m_userWaveBtn->disconnect( this );
}



void LfoControllerDialog::askUserDefWave()
{
	SampleBuffer * sampleBuffer = dynamic_cast<LfoController*>(this->model())->
									m_userDefSampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	if( fileName.isEmpty() == false )
	{
		// TODO:
		ToolTip::add( m_userWaveBtn, sampleBuffer->audioFile() );
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
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
	*/
}



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


