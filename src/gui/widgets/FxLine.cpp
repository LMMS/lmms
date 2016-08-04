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

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QPainter>
#include <QLineEdit>
#include <QWhatsThis>

#include "CaptionMenu.h"
#include "embed.h"
#include "Engine.h"
#include "FxMixer.h"
#include "FxMixerView.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "SendButtonIndicator.h"
#include "Song.h"


const int FxLine::FxLineHeight = 287;
QPixmap * FxLine::s_sendBgArrow = NULL;
QPixmap * FxLine::s_receiveBgArrow = NULL;

FxLine::FxLine( QWidget * _parent, FxMixerView * _mv, int _channelIndex) :
	QWidget( _parent ),
	m_mv( _mv ),
	m_channelIndex( _channelIndex ),
	m_backgroundActive( Qt::SolidPattern ),
	m_strokeOuterActive( 0, 0, 0 ),
	m_strokeOuterInactive( 0, 0, 0 ),
	m_strokeInnerActive( 0, 0, 0 ),
	m_strokeInnerInactive( 0, 0, 0 ),
	m_inRename( false )
{
	if( !s_sendBgArrow )
	{
		s_sendBgArrow = new QPixmap( embed::getIconPixmap( "send_bg_arrow", 29, 56 ) );
	}
	if( !s_receiveBgArrow )
	{
		s_receiveBgArrow = new QPixmap( embed::getIconPixmap( "receive_bg_arrow", 29, 56 ) );
	}

	setFixedSize( 33, FxLineHeight );
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );

	// mixer sends knob
	m_sendKnob = new Knob( knobBright_26, this, tr( "Channel send amount" ) );
	m_sendKnob->move( 3, 22 );
	m_sendKnob->setVisible( false );

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
	"by right-clicking the FX channel.\n" ) );

	QString name = Engine::fxMixer()->effectChannel( m_channelIndex )->m_name;
	setToolTip( name );

	m_renameLineEdit = new QLineEdit();
	m_renameLineEdit->setText( name );
	m_renameLineEdit->setFixedWidth( 65 );
	m_renameLineEdit->setFont( pointSizeF( font(), 7.5f ) );
	m_renameLineEdit->setReadOnly( true );

	QGraphicsScene * scene = new QGraphicsScene();
	scene->setSceneRect( 0, 0, 33, FxLineHeight );

	m_view = new QGraphicsView( this );
	m_view->setStyleSheet( "border-style: none; background: transparent;" );
	m_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_view->setAttribute( Qt::WA_TransparentForMouseEvents, true );
	m_view->setScene( scene );

	QGraphicsProxyWidget * proxyWidget = scene->addWidget( m_renameLineEdit );
	proxyWidget->setRotation( -90 );
	proxyWidget->setPos( 8, 145 );

	connect( m_renameLineEdit, SIGNAL( editingFinished() ), this, SLOT( renameFinished() ) );
}



FxLine::~FxLine()
{
	delete m_sendKnob;
	delete m_sendBtn;
	delete m_lcd;
}


void FxLine::setChannelIndex( int index )
{
	m_channelIndex = index;
	m_lcd->setValue( m_channelIndex );
	m_lcd->update();
}




void FxLine::drawFxLine( QPainter* p, const FxLine *fxLine, bool isActive, bool sendToThis, bool receiveFromThis )
{
	QString name = Engine::fxMixer()->effectChannel( m_channelIndex )->m_name;
	if( !m_inRename && m_renameLineEdit->text() != elideName( name ) )
	{
		m_renameLineEdit->setText( elideName( name ) );
	}

	int width = fxLine->rect().width();
	int height = fxLine->rect().height();

	p->fillRect( fxLine->rect(), isActive ? fxLine->backgroundActive() : p->background() );
	
	// inner border
	p->setPen( isActive ? fxLine->strokeInnerActive() : fxLine->strokeInnerInactive() );
	p->drawRect( 1, 1, width-3, height-3 );
	
	// outer border
	p->setPen( isActive ? fxLine->strokeOuterActive() : fxLine->strokeOuterInactive() );
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
}




QString FxLine::elideName( QString name )
{
	const int maxTextHeight = 70;
	QFontMetrics metrics( font() );
	QString elidedName = metrics.elidedText( name, Qt::ElideRight, maxTextHeight );
	return elidedName;
}




void FxLine::paintEvent( QPaintEvent * )
{
	bool sendToThis = Engine::fxMixer()->channelSendModel( m_mv->currentFxLine()->m_channelIndex, m_channelIndex ) != NULL;
	bool receiveFromThis = Engine::fxMixer()->channelSendModel( m_channelIndex, m_mv->currentFxLine()->m_channelIndex ) != NULL;
	QPainter painter;
	painter.begin( this );
	drawFxLine( &painter, this, m_mv->currentFxLine() == this, sendToThis, receiveFromThis );
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
	QPointer<CaptionMenu> contextMenu = new CaptionMenu( Engine::fxMixer()->effectChannel( m_channelIndex )->m_name, this );
	if( m_channelIndex != 0 ) // no move-options in master 
	{
		contextMenu->addAction( tr( "Move &left" ),	this, SLOT( moveChannelLeft() ) );
		contextMenu->addAction( tr( "Move &right" ), this, SLOT( moveChannelRight() ) );
	}
	contextMenu->addAction( tr( "Rename &channel" ), this, SLOT( renameChannel() ) );
	contextMenu->addSeparator();

	if( m_channelIndex != 0 ) // no remove-option in master
	{
		contextMenu->addAction( embed::getIconPixmap( "cancel" ), tr( "R&emove channel" ), this, SLOT( removeChannel() ) );
		contextMenu->addSeparator();
	}
	contextMenu->addAction( embed::getIconPixmap( "cancel" ), tr( "Remove &unused channels" ), this, SLOT( removeUnusedChannels() ) );
	contextMenu->addSeparator();
	contextMenu->addHelpAction();
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}




void FxLine::renameChannel()
{
	m_inRename = true;
	m_renameLineEdit->setReadOnly( false );
	m_lcd->hide();
	m_renameLineEdit->setFixedWidth( 135 );
	m_renameLineEdit->setText( Engine::fxMixer()->effectChannel( m_channelIndex )->m_name );
	m_view->setFocus();
	m_renameLineEdit->selectAll();
	m_renameLineEdit->setFocus();
}




void FxLine::renameFinished()
{
	m_inRename = false;
	m_renameLineEdit->setReadOnly( true );
	m_renameLineEdit->setFixedWidth( 65 );
	m_lcd->show();
	QString newName = m_renameLineEdit->text();
	setFocus();
	if( !newName.isEmpty() && Engine::fxMixer()->effectChannel( m_channelIndex )->m_name != newName )
	{
		Engine::fxMixer()->effectChannel( m_channelIndex )->m_name = newName;
		setToolTip( newName );
		m_renameLineEdit->setText( elideName( newName ) );
		Engine::getSong()->setModified();
	}
}




void FxLine::removeChannel()
{
	FxMixerView * mix = gui->fxMixerView();
	mix->deleteChannel( m_channelIndex );
}




void FxLine::removeUnusedChannels()
{
	FxMixerView * mix = gui->fxMixerView();
	mix->deleteUnusedChannels();
}




void FxLine::moveChannelLeft()
{
	FxMixerView * mix = gui->fxMixerView();
	mix->moveChannelLeft( m_channelIndex );
}




void FxLine::moveChannelRight()
{
	FxMixerView * mix = gui->fxMixerView();
	mix->moveChannelRight( m_channelIndex );
}




void FxLine::displayHelp()
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ), whatsThis() );
}




QBrush FxLine::backgroundActive() const
{
	return m_backgroundActive;
}




void FxLine::setBackgroundActive( const QBrush & c )
{
	m_backgroundActive = c;
}




QColor FxLine::strokeOuterActive() const
{
	return m_strokeOuterActive;
}




void FxLine::setStrokeOuterActive( const QColor & c )
{
	m_strokeOuterActive = c;
}




QColor FxLine::strokeOuterInactive() const
{
	return m_strokeOuterInactive;
}




void FxLine::setStrokeOuterInactive( const QColor & c )
{
	m_strokeOuterInactive = c;
}




QColor FxLine::strokeInnerActive() const
{
	return m_strokeInnerActive;
}




void FxLine::setStrokeInnerActive( const QColor & c )
{
	m_strokeInnerActive = c;
}




QColor FxLine::strokeInnerInactive() const
{
	return m_strokeInnerInactive;
}




void FxLine::setStrokeInnerInactive( const QColor & c )
{
	m_strokeInnerInactive = c;
}
