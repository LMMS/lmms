/*
 * EffectView.cpp - view-component for an effect
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QMdiSubWindow>
#include <QPainter>
#include <QLayout>

#include "EffectView.h"
#include "DummyEffect.h"
#include "CaptionMenu.h"
#include "embed.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "MainWindow.h"
#include "TempoSyncKnob.h"
#include "ToolTip.h"


EffectView::EffectView( Effect * _model, QWidget * _parent ) :
	PluginView( _model, _parent ),
	m_bg( embed::getIconPixmap( "effect_plugin" ) ),
	m_subWindow( NULL ),
	m_controlView( NULL )
{
	setFixedSize( 210, 60 );

	// Disable effects that are of type "DummyEffect"
	bool isEnabled = !dynamic_cast<DummyEffect *>( effect() );
	m_bypass = new LedCheckBox( this, "", isEnabled ? LedCheckBox::Green : LedCheckBox::Red );
	m_bypass->move( 3, 3 );
	m_bypass->setEnabled( isEnabled );

	ToolTip::add( m_bypass, tr( "On/Off" ) );


	m_wetDry = new Knob( knobBright_26, this );
	m_wetDry->setLabel( tr( "W/D" ) );
	m_wetDry->move( 27, 5 );
	m_wetDry->setEnabled( isEnabled );
	m_wetDry->setHintText( tr( "Wet Level:" ), "" );


	m_autoQuit = new TempoSyncKnob( knobBright_26, this );
	m_autoQuit->setLabel( tr( "DECAY" ) );
	m_autoQuit->move( 60, 5 );
	m_autoQuit->setEnabled( isEnabled && !effect()->m_autoQuitDisabled );
	m_autoQuit->setHintText( tr( "Time:" ), "ms" );


	m_gate = new Knob( knobBright_26, this );
	m_gate->setLabel( tr( "GATE" ) );
	m_gate->move( 93, 5 );
	m_gate->setEnabled( isEnabled && !effect()->m_autoQuitDisabled );
	m_gate->setHintText( tr( "Gate:" ), "" );


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
			m_subWindow = gui->mainWindow()->addWindowedWidget( m_controlView );

			if ( !m_controlView->isResizable() )
			{
				m_subWindow->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
				if (m_subWindow->layout())
				{
					m_subWindow->layout()->setSizeConstraint(QLayout::SetFixedSize);
				}
			}

			Qt::WindowFlags flags = m_subWindow->windowFlags();
			flags &= ~Qt::WindowMaximizeButtonHint;
			m_subWindow->setWindowFlags( flags );

			connect( m_controlView, SIGNAL( closed() ),
					this, SLOT( closeEffects() ) );

			m_subWindow->hide();
		}
	}


	//move above vst effect view creation
	//setModel( _model );
}




EffectView::~EffectView()
{
	delete m_subWindow;
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
	QPointer<CaptionMenu> contextMenu = new CaptionMenu( model()->displayName(), this );
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
