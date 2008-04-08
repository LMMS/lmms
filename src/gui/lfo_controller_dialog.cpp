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

const int LFO_GRAPH_X = 6;
const int LFO_GRAPH_Y = ENV_KNOBS_LBL_Y+14;
const int LFO_KNOB_Y = LFO_GRAPH_Y-2;
const int LFO_PREDELAY_KNOB_X = LFO_GRAPH_X + 10;
const int LFO_ATTACK_KNOB_X = LFO_PREDELAY_KNOB_X+KNOB_X_SPACING;
const int LFO_SPEED_KNOB_X = LFO_ATTACK_KNOB_X+KNOB_X_SPACING;
const int LFO_AMOUNT_KNOB_X = LFO_SPEED_KNOB_X+KNOB_X_SPACING;
const int LFO_SHAPES_X = LFO_GRAPH_X;//PREDELAY_KNOB_X;
const int LFO_SHAPES_Y = LFO_GRAPH_Y + 50;

lfoControllerDialog::lfoControllerDialog( controller * _model, QWidget * _parent ) :
	controllerDialog( _model, _parent )
{
	setFixedSize( 256, 64 );
	
	toolTip::add( this, tr( "Poor lonely controller" ) );

	

	m_lfoAttackKnob = new knob( knobBright_26, this,
						tr( "LFO-attack-time" ) );
	m_lfoAttackKnob->setLabel( tr( "ATT" ) );
	m_lfoAttackKnob->move( LFO_ATTACK_KNOB_X, LFO_KNOB_Y );
	m_lfoAttackKnob->setHintText( tr( "LFO-attack:" ) + " ", "" );
	m_lfoAttackKnob->setWhatsThis(
		tr( "Use this knob for setting attack-time of the current LFO. "
			"The bigger this value the longer the LFO needs to "
			"increase its amplitude to maximum." ) );


	m_lfoSpeedKnob = new tempoSyncKnob( knobBright_26, this,
							tr( "LFO-speed" ) );
	m_lfoSpeedKnob->setLabel( tr( "SPD" ) );
	m_lfoSpeedKnob->move( LFO_SPEED_KNOB_X, LFO_KNOB_Y );
	m_lfoSpeedKnob->setHintText( tr( "LFO-speed:" ) + " ", "" );
	m_lfoSpeedKnob->setWhatsThis(
		tr( "Use this knob for setting speed of the current LFO. The "
			"bigger this value the faster the LFO oscillates and "
			"the faster will be your effect." ) );


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
	m_lfoWaveBtnGrp->addButton( sin_lfo_btn );
	m_lfoWaveBtnGrp->addButton( triangle_lfo_btn );
	m_lfoWaveBtnGrp->addButton( saw_lfo_btn );
	m_lfoWaveBtnGrp->addButton( sqr_lfo_btn );
	m_lfoWaveBtnGrp->addButton( m_userLfoBtn );


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

	p.setPen( QColor( 128, 64, 0 ) );
	p.drawLine(0,0,40,40);
}



void lfoControllerDialog::modelChanged( void )
{
	m_lfo = castModel<lfoController>();

	m_lfoAttackKnob->setModel( &m_lfo->m_lfoAttackModel );
	m_lfoSpeedKnob->setModel( &m_lfo->m_lfoSpeedModel );
	m_lfoAmountKnob->setModel( &m_lfo->m_lfoAmountModel );
	m_lfoWaveBtnGrp->setModel( &m_lfo->m_lfoWaveModel );
}


#endif
