/*
 * FxLine.cpp - FX line widget
 *
 * Copyright (c) 2009 Andrew Kelley <superjoe30/at/gmail/dot/com>
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "FxLine.h"

#include <QDebug>
#include <QtGui/QInputDialog>
#include <QtGui/QPainter>
#include <QtGui/QLineEdit>
#include <QtGui/QWhatsThis>

#include "FxMixer.h"
#include "FxMixerView.h"
#include "embed.h"
#include "engine.h"
#include "SendButtonIndicator.h"
#include "gui_templates.h"
#include "caption_menu.h"

const int FxLine::FxLineHeight = 287;
QPixmap * FxLine::s_sendBgArrow = NULL;
QPixmap * FxLine::s_receiveBgArrow = NULL;

FxLine::FxLine( QWidget * _parent, FxMixerView * _mv, int _channelIndex) :
	QWidget( _parent ),
	m_mv( _mv ),
	m_channelIndex( _channelIndex )
{
	if( ! s_sendBgArrow )
	{
		s_sendBgArrow = new QPixmap( embed::getIconPixmap( "send_bg_arrow", 29, 56 ) );
	}
	if( ! s_receiveBgArrow )
	{
		s_receiveBgArrow = new QPixmap( embed::getIconPixmap( "receive_bg_arrow", 29, 56 ) );
	}

	setFixedSize( 33, FxLineHeight );
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );

	// mixer sends knob
	m_sendKnob = new knob( knobBright_26, this, tr("Channel send amount") );
	m_sendKnob->move( 3, 22 );
	m_sendKnob->setVisible(false);

	// send button indicator
	m_sendBtn = new SendButtonIndicator( this, this, m_mv );
	m_sendBtn->move( 2, 2 );

	// channel number
	m_lcd = new LcdWidget( 2, this );
	m_lcd->setValue( m_channelIndex );
	m_lcd->move( 4, 58 );
	m_lcd->setMarginWidth( 1 );
	
	setWhatsThis( tr(
	"The FX channel receives input from one or more instrument tracks.\n "
	"It in turn can be routed to multiple other FX channels. LMMS automatically "
	"takes care of preventing infinite loops for you and doesn't allow making "
	"a connection that would result in an infinite loop.\n\n"
	
	"In order to route the channel to another channel, select the FX channel "
	"and click on the \"send\" button on the channel you want to send to. "
	"The knob under the send button controls the level of signal that is sent "
	"to the channel.\n\n"
	
	"You can remove and move FX channels in the context menu, which is accessed "
	"by right-clicking the FX channel.\n") );
}


FxLine::~FxLine()
{
	delete m_sendKnob;
	delete m_sendBtn;
	delete m_lcd;
}


void FxLine::setChannelIndex(int index) {
	m_channelIndex = index;

	m_lcd->setValue( m_channelIndex );
	m_lcd->update();
}


void FxLine::drawFxLine( QPainter* p, const FxLine *fxLine, const QString& name, bool isActive, bool sendToThis, bool receiveFromThis )
{
	int width = fxLine->rect().width();
	int height = fxLine->rect().height();

	QColor sh_color = QApplication::palette().color( QPalette::Active,
							QPalette::Shadow );
	QColor te_color = p->pen().brush().color();
	QColor bt_color = QApplication::palette().color( QPalette::Active,
							QPalette::BrightText );


	p->fillRect( fxLine->rect(), isActive ? fxLine->backgroundActive() : p->background() );

	p->setPen( QColor( 255, 255, 255, isActive ? 100 : 50 ) );
	p->drawRect( 1, 1, width-3, height-3 );

	p->setPen( isActive ? sh_color : QColor( 0, 0, 0, 50 ) );
	p->drawRect( 0, 0, width-1, height-1 );

	// draw the mixer send background
	if( sendToThis )
	{
		p->drawPixmap( 2, 0, 29, 56, *s_sendBgArrow );
	}
	else if( receiveFromThis )
	{
		p->drawPixmap( 2, 0, 29, 56, *s_receiveBgArrow );
	}

	// draw the channel name
	p->rotate( -90 );

	p->setFont( pointSizeF( fxLine->font(), 7.5f ) );	
	p->setPen( sh_color );
	p->drawText( -146, 21, name ); 
	
	p->setPen( isActive ? bt_color : te_color );

	p->drawText( -145, 20, name );

}


void FxLine::paintEvent( QPaintEvent * )
{
	FxMixer * mix = engine::fxMixer();
	bool sendToThis = mix->channelSendModel(
		m_mv->currentFxLine()->m_channelIndex, m_channelIndex ) != NULL;
	bool receiveFromThis = mix->channelSendModel(
		m_channelIndex, m_mv->currentFxLine()->m_channelIndex ) != NULL;
	QPainter painter;
	painter.begin( this );
	drawFxLine( &painter, this,
		mix->effectChannel( m_channelIndex )->m_name,
		m_mv->currentFxLine() == this, sendToThis, receiveFromThis );
	painter.end();
}


void FxLine::mousePressEvent( QMouseEvent * )
{
	m_mv->setCurrentFxLine( this );
}


void FxLine::mouseDoubleClickEvent( QMouseEvent * )
{
	renameChannel();
}


void FxLine::contextMenuEvent( QContextMenuEvent * )
{
	FxMixer * mix = engine::fxMixer();
	QPointer<captionMenu> contextMenu = new captionMenu( mix->effectChannel( m_channelIndex )->m_name, this );
	if( m_channelIndex != 0 ) // no move-options in master 
	{
		contextMenu->addAction( tr( "Move &left" ),	this, SLOT( moveChannelLeft() ) );
		contextMenu->addAction( tr( "Move &right" ), this, SLOT( moveChannelRight() ) );
	}
	contextMenu->addAction( tr( "Rename &channel" ), this, SLOT( renameChannel() ) );
	contextMenu->addSeparator();
	
	if( m_channelIndex != 0 ) // no remove-option in master
	{
		contextMenu->addAction( embed::getIconPixmap( "cancel" ), tr( "R&emove channel" ),
							this, SLOT( removeChannel() ) );
		contextMenu->addSeparator();
	}
	
	contextMenu->addHelpAction();
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}


void FxLine::renameChannel()
{
	bool ok;
	FxMixer * mix = engine::fxMixer();
	QString new_name = QInputDialog::getText( this,
			FxMixerView::tr( "Rename FX channel" ),
			FxMixerView::tr( "Enter the new name for this "
						"FX channel" ),
			QLineEdit::Normal, mix->effectChannel(m_channelIndex)->m_name, &ok );
	if( ok && !new_name.isEmpty() )
	{
		mix->effectChannel( m_channelIndex )->m_name = new_name;
		update();
	}
}


void FxLine::removeChannel()
{
	FxMixerView * mix = engine::fxMixerView();
	mix->deleteChannel( m_channelIndex );
}


void FxLine::moveChannelLeft()
{
	FxMixerView * mix = engine::fxMixerView();
	mix->moveChannelLeft( m_channelIndex );
}


void FxLine::moveChannelRight()
{
	FxMixerView * mix = engine::fxMixerView();
	mix->moveChannelRight( m_channelIndex );
}


void FxLine::displayHelp()
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
}

QBrush FxLine::backgroundActive() const
{
	return m_backgroundActive;
}

void FxLine::setBackgroundActive( const QBrush & c )
{
	m_backgroundActive = c;
}

#include "moc_FxLine.cxx"

