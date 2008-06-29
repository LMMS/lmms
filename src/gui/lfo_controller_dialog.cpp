#ifndef SINGLE_SOURCE_COMPILE

/*
 * lfo_controller_dialog.cpp - per-controller-specific view for changing a
 * controller's settings
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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
#include "main_window.h"
#include "tooltip.h"


#include "lfo_controller.h"
#include "controller_dialog.h"
#include "mv_base.h"
#include "knob.h"
#include "tempo_sync_knob.h"
#include "pixmap_button.h"

const int ENV_KNOBS_LBL_Y = 0;
const int KNOB_X_SPACING = 32;

const int LFO_SHAPES_X = 6;
const int LFO_SHAPES_Y = 16;

const int LFO_GRAPH_X = 6;
const int LFO_GRAPH_Y = ENV_KNOBS_LBL_Y+15;
const int LFO_KNOB_Y = LFO_GRAPH_Y-2;
const int LFO_BASE_KNOB_X = LFO_SHAPES_X + 64;
const int LFO_SPEED_KNOB_X = LFO_BASE_KNOB_X+KNOB_X_SPACING;
const int LFO_AMOUNT_KNOB_X = LFO_SPEED_KNOB_X+KNOB_X_SPACING;
const int LFO_PHASE_KNOB_X = LFO_AMOUNT_KNOB_X+KNOB_X_SPACING;
const int LFO_MULTIPLIER_X = LFO_PHASE_KNOB_X+KNOB_X_SPACING;

lfoControllerDialog::lfoControllerDialog( controller * _model, QWidget * _parent ) :
	controllerDialog( _model, _parent )
{
	QString title = tr( "LFO" );
	title.append( " (" );
	title.append( _model->name() );
	title.append( ")" );
	setWindowTitle( tr( "LFO (name)" ) );
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setFixedSize( 240, 80 );
	
	toolTip::add( this, tr( "LFO Controller" ) );

	m_baseKnob = new knob( knobBright_26, this );
	m_baseKnob->setLabel( tr( "BASE" ) );
	m_baseKnob->move( LFO_BASE_KNOB_X, LFO_KNOB_Y );
	m_baseKnob->setHintText( tr( "Base amount:" ) + " ", "" );
	m_baseKnob->setWhatsThis( tr("todo") );


	m_speedKnob = new tempoSyncKnob( knobBright_26, this );
	m_speedKnob->setLabel( tr( "SPD" ) );
	m_speedKnob->move( LFO_SPEED_KNOB_X, LFO_KNOB_Y );
	m_speedKnob->setHintText( tr( "LFO-speed:" ) + " ", "" );
	m_speedKnob->setWhatsThis(
		tr( "Use this knob for setting speed of the LFO. The "
			"bigger this value the faster the LFO oscillates and "
			"the faster the effect." ) );


	m_amountKnob = new knob( knobBright_26, this );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->move( LFO_AMOUNT_KNOB_X, LFO_KNOB_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
	m_amountKnob->setWhatsThis(
		tr( "Use this knob for setting modulation amount of the "
			"current LFO. The bigger this value the more the "
			"selected size (e.g. volume or cutoff-frequency) will "
			"be influenced by this LFO." ) );

	m_phaseKnob = new knob( knobBright_26, this );
	m_phaseKnob->setLabel( tr( "PHS" ) );
	m_phaseKnob->move( LFO_PHASE_KNOB_X, LFO_KNOB_Y );
	m_phaseKnob->setHintText( tr( "Phase offset:" ) + " ", "" + tr( "degrees" ) );
	m_phaseKnob->setWhatsThis(
			tr( "With this knob you can set the phase-offset of "
				"the LFO. That means you can move the "
				"point within an oscillation where the "
				"oscillator begins to oscillate. For example "
				"if you have a sine-wave and have a phase-"
				"offset of 180 degrees the wave will first go "
				"down. It's the same with a square-wave."
				) );

	pixmapButton * sin_wave_btn = new pixmapButton( this, NULL );
	sin_wave_btn->move( LFO_SHAPES_X, LFO_SHAPES_Y );
	sin_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"sin_wave_active" ) );
	sin_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"sin_wave_inactive" ) );
	toolTip::add( sin_wave_btn,
			tr( "Click here if you want a sine-wave for "
					"current oscillator." ) );

	pixmapButton * triangle_wave_btn =
					new pixmapButton( this, NULL );
	triangle_wave_btn->move( LFO_SHAPES_X + 15, LFO_SHAPES_Y );
	triangle_wave_btn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	triangle_wave_btn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	toolTip::add( triangle_wave_btn,
			tr( "Click here if you want a triangle-wave "
					"for current oscillator." ) );

	pixmapButton * saw_wave_btn = new pixmapButton( this, NULL );
	saw_wave_btn->move( LFO_SHAPES_X + 30, LFO_SHAPES_Y );
	saw_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"saw_wave_active" ) );
	saw_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"saw_wave_inactive" ) );
	toolTip::add( saw_wave_btn,
			tr( "Click here if you want a saw-wave for "
					"current oscillator." ) );

	pixmapButton * sqr_wave_btn = new pixmapButton( this, NULL );
	sqr_wave_btn->move( LFO_SHAPES_X + 45, LFO_SHAPES_Y );
	sqr_wave_btn->setActiveGraphic( embed::getIconPixmap(
					"square_wave_active" ) );
	sqr_wave_btn->setInactiveGraphic( embed::getIconPixmap(
					"square_wave_inactive" ) );
	toolTip::add( sqr_wave_btn,
			tr( "Click here if you want a square-wave for "
					"current oscillator." ) );

	pixmapButton * moog_saw_wave_btn =
					new pixmapButton( this, NULL );
	moog_saw_wave_btn->move( LFO_SHAPES_X, LFO_SHAPES_Y + 15 );
	moog_saw_wave_btn->setActiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_active" ) );
	moog_saw_wave_btn->setInactiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_inactive" ) );
	toolTip::add( moog_saw_wave_btn,
			tr( "Click here if you want a moog-saw-wave "
					"for current oscillator." ) );

	pixmapButton * exp_wave_btn = new pixmapButton( this, NULL );
	exp_wave_btn->move( LFO_SHAPES_X + 15, LFO_SHAPES_Y + 15 );
	exp_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"exp_wave_active" ) );
	exp_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"exp_wave_inactive" ) );
	toolTip::add( exp_wave_btn,
			tr( "Click here if you want an exponential "
				"wave for current oscillator." ) );

	pixmapButton * white_noise_btn = new pixmapButton( this, NULL );
	white_noise_btn->move( LFO_SHAPES_X + 30, LFO_SHAPES_Y + 15 );
	white_noise_btn->setActiveGraphic(
		embed::getIconPixmap( "white_noise_wave_active" ) );
	white_noise_btn->setInactiveGraphic(
		embed::getIconPixmap( "white_noise_wave_inactive" ) );
	toolTip::add( white_noise_btn,
			tr( "Click here if you want a white-noise for "
					"current oscillator." ) );

	pixmapButton * uwb = new pixmapButton( this, NULL );
	uwb->move( LFO_SHAPES_X + 45, LFO_SHAPES_Y + 15 );
	uwb->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	uwb->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	uwb->setEnabled( false );
	toolTip::add( uwb, tr( "Click here if you want a user-defined "
			"wave-shape for current oscillator." ) );
	
	m_waveBtnGrp = new automatableButtonGroup( this );
	m_waveBtnGrp->addButton( sin_wave_btn );
	m_waveBtnGrp->addButton( triangle_wave_btn );
	m_waveBtnGrp->addButton( saw_wave_btn );
	m_waveBtnGrp->addButton( sqr_wave_btn );
	m_waveBtnGrp->addButton( moog_saw_wave_btn );
	m_waveBtnGrp->addButton( exp_wave_btn );
	m_waveBtnGrp->addButton( white_noise_btn );
	m_waveBtnGrp->addButton( uwb );


	pixmapButton * x1 = new pixmapButton( this, NULL );
	x1->move( LFO_MULTIPLIER_X, LFO_SHAPES_Y );
	x1->setActiveGraphic( embed::getIconPixmap(
						"lfo_x1_active" ) );
	x1->setInactiveGraphic( embed::getIconPixmap(
						"lfo_x1_inactive" ) );

	pixmapButton * x100 = new pixmapButton( this, NULL );
	x100->move( LFO_MULTIPLIER_X, LFO_SHAPES_Y - 15 );
	x100->setActiveGraphic( embed::getIconPixmap(
						"lfo_x100_active" ) );
	x100->setInactiveGraphic( embed::getIconPixmap(
						"lfo_x100_inactive" ) );

	pixmapButton * d100 = new pixmapButton( this, NULL );
	d100->move( LFO_MULTIPLIER_X, LFO_SHAPES_Y + 15 );
	d100->setActiveGraphic( embed::getIconPixmap(
						"lfo_d100_active" ) );
	d100->setInactiveGraphic( embed::getIconPixmap(
						"lfo_d100_inactive" ) );

	m_multiplierBtnGrp = new automatableButtonGroup( this );
	m_multiplierBtnGrp->addButton( x1 );
	m_multiplierBtnGrp->addButton( x100 );
	m_multiplierBtnGrp->addButton( d100 );


	setModel( _model );

	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				QColor( 255 ,0 ,0, 255 ) );
	setPalette( pal );

}



lfoControllerDialog::~lfoControllerDialog()
{
	//delete m_subWindow;
}



/*
void effectView::displayHelp( void )
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
}




void effectView::closeEffects( void )
{
	m_subWindow->hide();
	m_show = TRUE;
}
*/


void lfoControllerDialog::contextMenuEvent( QContextMenuEvent * )
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


void lfoControllerDialog::modelChanged( void )
{
	m_lfo = castModel<lfoController>();

	m_baseKnob->setModel( &m_lfo->m_baseModel );
	m_speedKnob->setModel( &m_lfo->m_speedModel );
	m_amountKnob->setModel( &m_lfo->m_amountModel );
	m_phaseKnob->setModel( &m_lfo->m_phaseModel );
	m_waveBtnGrp->setModel( &m_lfo->m_waveModel );
	m_multiplierBtnGrp->setModel( &m_lfo->m_multiplierModel );
}


#endif
