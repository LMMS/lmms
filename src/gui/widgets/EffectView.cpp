/*
 * EffectView.cpp - view-component for an effect
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QWhatsThis>

#include "EffectView.h"
#include "DummyEffect.h"
#include "caption_menu.h"
#include "EffectControls.h"
#include "EffectControlDialog.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "knob.h"
#include "led_checkbox.h"
#include "MainWindow.h"
#include "TempoSyncKnob.h"
#include "tooltip.h"


EffectView::EffectView( Effect * _model, QWidget * _parent ) :
	PluginView( _model, _parent ),
	m_bg( embed::getIconPixmap( "effect_plugin" ) ),
	m_subWindow( NULL ),
	m_controlView( NULL )
{
	setFixedSize( 210, 60 );
	
	// Disable effects that are of type "DummyEffect"
	bool isEnabled = !dynamic_cast<DummyEffect *>( effect() );
	m_bypass = new ledCheckBox( this, "", isEnabled ? ledCheckBox::Green : ledCheckBox::Red );
	m_bypass->move( 3, 3 );
	m_bypass->setEnabled( isEnabled );
	m_bypass->setWhatsThis( tr( "Toggles the effect on or off." ) );
	
	toolTip::add( m_bypass, tr( "On/Off" ) );


	m_wetDry = new knob( knobBright_26, this );
	m_wetDry->setLabel( tr( "W/D" ) );
	m_wetDry->move( 27, 5 );
	m_wetDry->setEnabled( isEnabled );
	m_wetDry->setHintText( tr( "Wet Level:" ) + " ", "" );
	m_wetDry->setWhatsThis( tr( "The Wet/Dry knob sets the ratio between "
					"the input signal and the effect signal that "
					"forms the output." ) );


	m_autoQuit = new TempoSyncKnob( knobBright_26, this );
	m_autoQuit->setLabel( tr( "DECAY" ) );
	m_autoQuit->move( 60, 5 );
	m_autoQuit->setEnabled( isEnabled );
	m_autoQuit->setHintText( tr( "Time:" ) + " ", "ms" );
	m_autoQuit->setWhatsThis( tr(
"The Decay knob controls how many buffers of silence must pass before the "
"plugin stops processing.  Smaller values will reduce the CPU overhead but "
"run the risk of clipping the tail on delay and reverb effects." ) );


	m_gate = new knob( knobBright_26, this );
	m_gate->setLabel( tr( "GATE" ) );
	m_gate->move( 93, 5 );
	m_gate->setEnabled( isEnabled );
	m_gate->setHintText( tr( "Gate:" ) + " ", "" );
	m_gate->setWhatsThis( tr(
"The Gate knob controls the signal level that is considered to be 'silence' "
"while deciding when to stop processing signals." ) );


	setModel( _model );

	if( effect()->controls()->controlCount() > 0 )
	{
		QPushButton * ctls_btn = new QPushButton( tr( "Controls" ),
									this );
		QFont f = ctls_btn->font();
		ctls_btn->setFont( pointSize<8>( f ) );
		ctls_btn->setGeometry( 140, 14, 50, 20 );
		connect( ctls_btn, SIGNAL( clicked() ),
					this, SLOT( editControls() ) );

		m_controlView = effect()->controls()->createView();
		if( m_controlView )
		{
			m_subWindow = engine::mainWindow()->workspace()->addSubWindow(
								m_controlView,
				Qt::SubWindow | Qt::CustomizeWindowHint  |
					Qt::WindowTitleHint | Qt::WindowSystemMenuHint );
			m_subWindow->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
			m_subWindow->setFixedSize( m_subWindow->size() );

			connect( m_controlView, SIGNAL( closed() ),
					this, SLOT( closeEffects() ) );

			m_subWindow->hide();
		}
	}


	setWhatsThis( tr(
"Effect plugins function as a chained series of effects where the signal will "
"be processed from top to bottom.\n\n"

"The On/Off switch allows you to bypass a given plugin at any point in "
"time.\n\n"

"The Wet/Dry knob controls the balance between the input signal and the "
"effected signal that is the resulting output from the effect.  The input "
"for the stage is the output from the previous stage. So, the 'dry' signal "
"for effects lower in the chain contains all of the previous effects.\n\n"

"The Decay knob controls how long the signal will continue to be processed "
"after the notes have been released.  The effect will stop processing signals "
"when the volume has dropped below a given threshold for a given length of "
"time.  This knob sets the 'given length of time'.  Longer times will require "
"more CPU, so this number should be set low for most effects.  It needs to be "
"bumped up for effects that produce lengthy periods of silence, e.g. "
"delays.\n\n"

"The Gate knob controls the 'given threshold' for the effect's auto shutdown.  "
"The clock for the 'given length of time' will begin as soon as the processed "
"signal level drops below the level specified with this knob.\n\n"

"The Controls button opens a dialog for editing the effect's parameters.\n\n"

"Right clicking will bring up a context menu where you can change the order "
"in which the effects are processed or delete an effect altogether." ) );

	//move above vst effect view creation
	//setModel( _model );
}




EffectView::~EffectView()
{

#ifdef LMMS_BUILD_LINUX

	delete m_subWindow;
#else
	if( m_subWindow )
	{
		// otherwise on win32 build VST GUI can get lost
		m_subWindow->hide();
	}
#endif

}




void EffectView::editControls()
{
	if( m_subWindow )
	{
		if( !m_subWindow->isVisible() )
		{
			m_subWindow->show();
			m_subWindow->raise();
			effect()->controls()->setViewVisible( true );
		}
		else
		{
			m_subWindow->hide();
			effect()->controls()->setViewVisible( false );
		}
	}
}




void EffectView::moveUp()
{
	emit moveUp( this );
}




void EffectView::moveDown()
{
	emit moveDown( this );
}



void EffectView::deletePlugin()
{
	emit deletePlugin( this );
}




void EffectView::displayHelp()
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
}




void EffectView::closeEffects()
{
	if( m_subWindow )
	{
		m_subWindow->hide();
	}
	effect()->controls()->setViewVisible( false );
}



void EffectView::contextMenuEvent( QContextMenuEvent * )
{
	QPointer<captionMenu> contextMenu = new captionMenu( model()->displayName(), this );
	contextMenu->addAction( embed::getIconPixmap( "arp_up" ),
						tr( "Move &up" ),
						this, SLOT( moveUp() ) );
	contextMenu->addAction( embed::getIconPixmap( "arp_down" ),
						tr( "Move &down" ),
						this, SLOT( moveDown() ) );
	contextMenu->addSeparator();
	contextMenu->addAction( embed::getIconPixmap( "cancel" ),
						tr( "&Remove this plugin" ),
						this, SLOT( deletePlugin() ) );
	contextMenu->addSeparator();
	contextMenu->addHelpAction();
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}




void EffectView::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.drawPixmap( 0, 0, m_bg );

	QFont f = pointSizeF( font(), 7.5f );
	f.setBold( true );
	p.setFont( f );

	p.setPen( palette().shadow().color() );
	p.drawText( 6, 55, model()->displayName() );
	p.setPen( palette().text().color() );
	p.drawText( 5, 54, model()->displayName() );
}




void EffectView::modelChanged()
{
	m_bypass->setModel( &effect()->m_enabledModel );
	m_wetDry->setModel( &effect()->m_wetDryModel );
	m_autoQuit->setModel( &effect()->m_autoQuitModel );
	m_gate->setModel( &effect()->m_gateModel );
}



#include "moc_EffectView.cxx"

