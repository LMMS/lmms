/*
 * FxLine.cpp - FX line widget
 *
 * Copyright (c) 2009 Andrew Kelley <superjoe30/at/gmail/dot/com>
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "FxLine.h"

#include <QDebug>
#include <QtGui/QInputDialog>
#include <QtGui/QPainter>
#include <QtGui/QLineEdit>

#include "FxMixer.h"
#include "FxMixerView.h"
#include "embed.h"
#include "engine.h"
#include "SendButtonIndicator.h"
#include "gui_templates.h"

FxLine::FxLine( QWidget * _parent, FxMixerView * _mv, int _channelIndex) :
	QWidget( _parent ),
	m_mv( _mv ),
	m_channelIndex( _channelIndex )
{
	setFixedSize( 32, 287 );
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 0, 0 ) );

	// mixer sends knob
	m_sendKnob = new knob(0, this, tr("Channel send amount"));
	m_sendKnob->move(0, 22);
	m_sendKnob->setVisible(false);

	// send button indicator
	m_sendBtn = new SendButtonIndicator(this, this, m_mv);
	m_sendBtn->setPixmap(embed::getIconPixmap("mixer_send_off", 23, 16));
	m_sendBtn->move(4,4);

	// channel number
	m_lcd = new LcdWidget( 2, this );
	m_lcd->setValue( m_channelIndex );
	m_lcd->move( 2, 58 );
	m_lcd->setMarginWidth( 1 );
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


static void drawFxLine( QPainter* p, const QWidget *fxLine, const QString& name, bool isActive, bool sendToThis )
{
	int width = fxLine->rect().width();
	int height = fxLine->rect().height();

	QColor bg_color = QApplication::palette().color( QPalette::Active,
							QPalette::Background );
	QColor sh_color = QApplication::palette().color( QPalette::Active,
							QPalette::Shadow );
	QColor te_color = QApplication::palette().color( QPalette::Active,
							QPalette::Text );
	QColor bt_color = QApplication::palette().color( QPalette::Active,
							QPalette::BrightText );


	p->fillRect( fxLine->rect(), isActive ? bg_color.lighter(130) : bg_color );

	p->setPen( bg_color.darker(130) );
	p->drawRect( 0, 0, width-2, height-2 );

	p->setPen( bg_color.lighter(150) );
	p->drawRect( 1, 1, width-2, height-2 );

	p->setPen( isActive ? sh_color : bg_color.darker(130) );
	p->drawRect( 0, 0, width-1, height-1 );

	// draw the mixer send background
	if( sendToThis )
	{
		p->drawPixmap(2, 0, 28, 56,
					  embed::getIconPixmap("send_bg_arrow", 28, 56));
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
		m_mv->currentFxLine()->m_channelIndex, m_channelIndex) != NULL;
	QPainter painter;
	painter.begin( this );
	drawFxLine( &painter, this,
		mix->effectChannel(m_channelIndex)->m_name,
		m_mv->currentFxLine() == this, sendToThis );
	painter.end();
}

void FxLine::mousePressEvent( QMouseEvent * )
{
	m_mv->setCurrentFxLine( this );
}

void FxLine::mouseDoubleClickEvent( QMouseEvent * )
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
		mix->effectChannel(m_channelIndex)->m_name = new_name;
		update();
	}
}

#include "moc_FxLine.cxx"

