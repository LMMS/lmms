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

lfoControllerDialog::lfoControllerDialog( controller * _model, QWidget * _parent ) :
	controllerDialog( _model, _parent )
{
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setWindowTitle( tr( "LFO (name)" ) );
	setFixedSize( 256, 64 );
	
	toolTip::add( this, tr( "LFO Controller" ) );

	m_lfoBaseKnob = new knob( knobBright_26, this,
						tr( "LFO base value" ) );
	m_lfoBaseKnob->setLabel( tr( "BASE" ) );
	m_lfoBaseKnob->move( LFO_BASE_KNOB_X, LFO_KNOB_Y );
	m_lfoBaseKnob->setHintText( tr( "Base amount:" ) + " ", "" );
	m_lfoBaseKnob->setWhatsThis( tr("todo") );


	m_lfoSpeedKnob = new tempoSyncKnob( knobBright_26, this,
							tr( "LFO-speed" ) );
	m_lfoSpeedKnob->setLabel( tr( "SPD" ) );
	m_lfoSpeedKnob->move( LFO_SPEED_KNOB_X, LFO_KNOB_Y );
	m_lfoSpeedKnob->setHintText( tr( "LFO-speed:" ) + " ", "" );
	m_lfoSpeedKnob->setWhatsThis(
		tr( "Use this knob for setting speed of the LFO. The "
			"bigger this value the faster the LFO oscillates and "
			"the faster the effect." ) );


	m_lfoAmountKnob = new knob( knobBright_26, this,
						tr( "LFO-modulation-amount" ) );
	m_lfoAmountKnob->setLabel( tr( "AMT" ) );
	m_lfoAmountKnob->move( LFO_AMOUNT_KNOB_X, LFO_KNOB_Y );
	m_lfoAmountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );
	m_lfoAmountKnob->setWhatsThis(
		tr( "Use this knob for setting modulation amount of the "
			"current LFO. The bigger this value the more the "
			"selected size (e.g. volume or cutoff-frequency) will "
			"be influenced by this LFO." ) );

	m_lfoPhaseKnob = new knob( knobBright_26, this,
						tr( "LFO phase" ) );
	m_lfoPhaseKnob->setLabel( tr( "PHS" ) );
	m_lfoPhaseKnob->move( LFO_PHASE_KNOB_X, LFO_KNOB_Y );
	m_lfoPhaseKnob->setHintText( tr( "Phase offset:" ) + " ", "" + tr( "degrees" ) );
	m_lfoPhaseKnob->setWhatsThis(
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

	
	m_lfoWaveBtnGrp = new automatableButtonGroup( this,
						tr( "LFO wave shape" ) );
	m_lfoWaveBtnGrp->addButton( sin_wave_btn );
	m_lfoWaveBtnGrp->addButton( triangle_wave_btn );
	m_lfoWaveBtnGrp->addButton( saw_wave_btn );
	m_lfoWaveBtnGrp->addButton( sqr_wave_btn );
	m_lfoWaveBtnGrp->addButton( moog_saw_wave_btn );
	m_lfoWaveBtnGrp->addButton( exp_wave_btn );
	m_lfoWaveBtnGrp->addButton( white_noise_btn );
	m_lfoWaveBtnGrp->addButton( uwb );


/*
	if( getEffect()->getControls()->getControlCount() > 0 )
	{
		QPushButton * ctls_btn = new QPushButton( tr( "Controls" ),
									this );
		QFont f = ctls_btn->font();
		ctls_btn->setFont( pointSize<7>( f ) );
		ctls_btn->setGeometry( 140, 14, 50, 20 );
		connect( ctls_btn, SIGNAL( clicked() ), 
					this, SLOT( editControls() ) );
	}


	m_controlView = getEffect()->getControls()->createView();
	m_subWindow = engine::getMainWindow()->workspace()->addSubWindow(
								m_controlView );
	connect( m_controlView, SIGNAL( closed() ),
				this, SLOT( closeEffects() ) );

	m_subWindow->hide();
*/

	setModel( _model );
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




void lfoControllerDialog::paintEvent( QPaintEvent * )
{
	QPainter p( this );
}



void lfoControllerDialog::modelChanged( void )
{
	m_lfo = castModel<lfoController>();

	m_lfoBaseKnob->setModel( &m_lfo->m_lfoBaseModel );
	m_lfoSpeedKnob->setModel( &m_lfo->m_lfoSpeedModel );
	m_lfoAmountKnob->setModel( &m_lfo->m_lfoAmountModel );
	m_lfoPhaseKnob->setModel( &m_lfo->m_lfoPhaseModel );
	m_lfoWaveBtnGrp->setModel( &m_lfo->m_lfoWaveModel );
}


#endif
