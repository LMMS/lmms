/*
 * MixerLine.cpp - Mixer line widget
 *
 * Copyright (c) 2009 Andrew Kelley <superjoe30/at/gmail/dot/com>
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MixerLine.h"

#include <cstdlib>

#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QLineEdit>
#include <QPainter>

#include "CaptionMenu.h"
#include "ColorChooser.h"
#include "embed.h"
#include "Knob.h"
#include "LcdWidget.h"
#include "Mixer.h"
#include "MixerView.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "SendButtonIndicator.h"
#include "Song.h"

namespace lmms::gui
{


bool MixerLine::eventFilter( QObject *dist, QEvent *event )
{
	// If we are in a rename, capture the enter/return events and handle them
	if ( event->type() == QEvent::KeyPress )
	{
		auto keyEvent = static_cast<QKeyEvent*>(event);
		if( keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
		{
			if( m_inRename )
			{
				renameFinished();
				event->accept(); // Stop the event from propagating
				return true;
			}
		}
	}
	return false;
}

const int MixerLine::MixerLineHeight = 287;

MixerLine::MixerLine( QWidget * _parent, MixerView * _mv, int _channelIndex ) :
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
	setFixedSize( 33, MixerLineHeight );
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );

	// mixer sends knob
	m_sendKnob = new Knob( KnobType::Bright26, this, tr( "Channel send amount" ) );
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
	
	QString name = Engine::mixer()->mixerChannel( m_channelIndex )->m_name;
	setToolTip( name );

	m_renameLineEdit = new QLineEdit();
	m_renameLineEdit->setText( name );
	m_renameLineEdit->setFixedWidth( 65 );
	m_renameLineEdit->setFont( pointSizeF( font(), 7.5f ) );
	m_renameLineEdit->setReadOnly( true );
	m_renameLineEdit->installEventFilter( this );

	auto scene = new QGraphicsScene();
	scene->setSceneRect( 0, 0, 33, MixerLineHeight );

	m_view = new QGraphicsView( this );
	m_view->setStyleSheet( "border-style: none; background: transparent;" );
	m_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_view->setAttribute( Qt::WA_TransparentForMouseEvents, true );
	m_view->setScene( scene );

	QGraphicsProxyWidget * proxyWidget = scene->addWidget( m_renameLineEdit );
	proxyWidget->setRotation( -90 );
	proxyWidget->setPos( 8, 145 );

	connect( m_renameLineEdit, SIGNAL(editingFinished()), this, SLOT(renameFinished()));
	connect( &Engine::mixer()->mixerChannel( m_channelIndex )->m_muteModel, SIGNAL(dataChanged()), this, SLOT(update()));
}




MixerLine::~MixerLine()
{
	delete m_sendKnob;
	delete m_sendBtn;
	delete m_lcd;
}




void MixerLine::setChannelIndex( int index )
{
	m_channelIndex = index;
	m_lcd->setValue( m_channelIndex );
	m_lcd->update();
}



void MixerLine::drawMixerLine( QPainter* p, const MixerLine *mixerLine, bool isActive, bool sendToThis, bool receiveFromThis )
{
	auto channel = Engine::mixer()->mixerChannel( m_channelIndex );
	bool muted = channel->m_muteModel.value();
	QString name = channel->m_name;
	QString elidedName = elideName( name );
	if( !m_inRename && m_renameLineEdit->text() != elidedName )
	{
		m_renameLineEdit->setText( elidedName );
	}

	int width = mixerLine->rect().width();
	int height = mixerLine->rect().height();
	
	if( channel->m_hasColor && !muted )
	{
		p->fillRect( mixerLine->rect(), channel->m_color.darker( isActive ? 120 : 150 ) );
	}
	else
	{
		p->fillRect( mixerLine->rect(),
					 isActive ? mixerLine->backgroundActive().color() : p->background().color() );
	}
	
	// inner border
	p->setPen( isActive ? mixerLine->strokeInnerActive() : mixerLine->strokeInnerInactive() );
	p->drawRect( 1, 1, width-3, height-3 );
	
	// outer border
	p->setPen( isActive ? mixerLine->strokeOuterActive() : mixerLine->strokeOuterInactive() );
	p->drawRect( 0, 0, width-1, height-1 );

	// draw the mixer send background

	static auto s_sendBgArrow = QPixmap{embed::getIconPixmap("send_bg_arrow", 29, 56)};
	static auto s_receiveBgArrow = QPixmap{embed::getIconPixmap("receive_bg_arrow", 29, 56)};
	p->drawPixmap(2, 0, 29, 56, sendToThis ? s_sendBgArrow : s_receiveBgArrow);
}




QString MixerLine::elideName( const QString & name )
{
	const int maxTextHeight = 60;
	QFontMetrics metrics( m_renameLineEdit->font() );
	QString elidedName = metrics.elidedText( name, Qt::ElideRight, maxTextHeight );
	return elidedName;
}




void MixerLine::paintEvent( QPaintEvent * )
{
	bool sendToThis = Engine::mixer()->channelSendModel( m_mv->currentMixerLine()->m_channelIndex, m_channelIndex ) != nullptr;
	bool receiveFromThis = Engine::mixer()->channelSendModel( m_channelIndex, m_mv->currentMixerLine()->m_channelIndex ) != nullptr;
	QPainter painter;
	painter.begin( this );
	drawMixerLine( &painter, this, m_mv->currentMixerLine() == this, sendToThis, receiveFromThis );
	painter.end();
}




void MixerLine::mousePressEvent( QMouseEvent * )
{
	m_mv->setCurrentMixerLine( this );
}




void MixerLine::mouseDoubleClickEvent( QMouseEvent * )
{
	renameChannel();
}




void MixerLine::contextMenuEvent( QContextMenuEvent * )
{
	QPointer<CaptionMenu> contextMenu = new CaptionMenu( Engine::mixer()->mixerChannel( m_channelIndex )->m_name, this );
	if( m_channelIndex != 0 ) // no move-options in master
	{
		contextMenu->addAction( tr( "Move &left" ),	this, SLOT(moveChannelLeft()));
		contextMenu->addAction( tr( "Move &right" ), this, SLOT(moveChannelRight()));
	}
	contextMenu->addAction( tr( "Rename &channel" ), this, SLOT(renameChannel()));
	contextMenu->addSeparator();

	if( m_channelIndex != 0 ) // no remove-option in master
	{
		contextMenu->addAction( embed::getIconPixmap( "cancel" ), tr( "R&emove channel" ), this, SLOT(removeChannel()));
		contextMenu->addSeparator();
	}
	contextMenu->addAction( embed::getIconPixmap( "cancel" ), tr( "Remove &unused channels" ), this, SLOT(removeUnusedChannels()));
	contextMenu->addSeparator();

	QMenu colorMenu(tr("Color"), this);
	colorMenu.setIcon(embed::getIconPixmap("colorize"));
	colorMenu.addAction(tr("Change"), this, SLOT(selectColor()));
	colorMenu.addAction(tr("Reset"), this, SLOT(resetColor()));
	colorMenu.addAction(tr("Pick random"), this, SLOT(randomizeColor()));
	contextMenu->addMenu(&colorMenu);

	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}




void MixerLine::renameChannel()
{
	m_inRename = true;
	setToolTip( "" );
	m_renameLineEdit->setReadOnly( false );
	m_lcd->hide();
	m_renameLineEdit->setFixedWidth( 135 );
	m_renameLineEdit->setText( Engine::mixer()->mixerChannel( m_channelIndex )->m_name );
	m_view->setFocus();
	m_renameLineEdit->selectAll();
	m_renameLineEdit->setFocus();
}




void MixerLine::renameFinished()
{
	m_inRename = false;
	m_renameLineEdit->deselect();
	m_renameLineEdit->setReadOnly( true );
	m_renameLineEdit->setFixedWidth( 65 );
	m_lcd->show();
	QString newName = m_renameLineEdit->text();
	setFocus();
	if( !newName.isEmpty() && Engine::mixer()->mixerChannel( m_channelIndex )->m_name != newName )
	{
		Engine::mixer()->mixerChannel( m_channelIndex )->m_name = newName;
		m_renameLineEdit->setText( elideName( newName ) );
		Engine::getSong()->setModified();
	}
	QString name = Engine::mixer()->mixerChannel( m_channelIndex )->m_name;
	setToolTip( name );
}




void MixerLine::removeChannel()
{
	MixerView * mix = getGUI()->mixerView();
	mix->deleteChannel( m_channelIndex );
}




void MixerLine::removeUnusedChannels()
{
	MixerView * mix = getGUI()->mixerView();
	mix->deleteUnusedChannels();
}




void MixerLine::moveChannelLeft()
{
	MixerView * mix = getGUI()->mixerView();
	mix->moveChannelLeft( m_channelIndex );
}




void MixerLine::moveChannelRight()
{
	MixerView * mix = getGUI()->mixerView();
	mix->moveChannelRight( m_channelIndex );
}




QBrush MixerLine::backgroundActive() const
{
	return m_backgroundActive;
}




void MixerLine::setBackgroundActive( const QBrush & c )
{
	m_backgroundActive = c;
}




QColor MixerLine::strokeOuterActive() const
{
	return m_strokeOuterActive;
}




void MixerLine::setStrokeOuterActive( const QColor & c )
{
	m_strokeOuterActive = c;
}




QColor MixerLine::strokeOuterInactive() const
{
	return m_strokeOuterInactive;
}




void MixerLine::setStrokeOuterInactive( const QColor & c )
{
	m_strokeOuterInactive = c;
}




QColor MixerLine::strokeInnerActive() const
{
	return m_strokeInnerActive;
}




void MixerLine::setStrokeInnerActive( const QColor & c )
{
	m_strokeInnerActive = c;
}




QColor MixerLine::strokeInnerInactive() const
{
	return m_strokeInnerInactive;
}




void MixerLine::setStrokeInnerInactive( const QColor & c )
{
	m_strokeInnerInactive = c;
}


// Ask user for a color, and set it as the mixer line color
void MixerLine::selectColor()
{
	auto channel = Engine::mixer()->mixerChannel( m_channelIndex );
	auto new_color = ColorChooser(this).withPalette(ColorChooser::Palette::Mixer)->getColor(channel->m_color);
	if(!new_color.isValid()) { return; }
	channel->setColor (new_color);
	Engine::getSong()->setModified();
	update();
}


// Disable the usage of color on this mixer line
void MixerLine::resetColor()
{
	Engine::mixer()->mixerChannel( m_channelIndex )->m_hasColor = false;
	Engine::getSong()->setModified();
	update();
}


// Pick a random color from the mixer palette and set it as our color
void MixerLine::randomizeColor()
{
	auto channel = Engine::mixer()->mixerChannel( m_channelIndex );
	channel->setColor (ColorChooser::getPalette(ColorChooser::Palette::Mixer)[rand() % 48]);
	Engine::getSong()->setModified();
	update();
}


} // namespace lmms::gui
