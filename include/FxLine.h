/*
 * FxLine.h - FX line widget
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

#ifndef FX_LINE_H
#define FX_LINE_H

#include <QWidget>
#include <QLabel>

#include "knob.h"
#include "LcdWidget.h"
#include "SendButtonIndicator.h"

class FxMixerView;
class SendButtonIndicator;

class FxLine : public QWidget
{
	Q_OBJECT
public:
	Q_PROPERTY( QBrush backgroundActive READ backgroundActive WRITE setBackgroundActive )
	FxLine( QWidget * _parent, FxMixerView * _mv, int _channelIndex);
	~FxLine();

	virtual void paintEvent( QPaintEvent * );
	virtual void mousePressEvent( QMouseEvent * );
	virtual void mouseDoubleClickEvent( QMouseEvent * );
	virtual void contextMenuEvent( QContextMenuEvent * );

	inline int channelIndex() { return m_channelIndex; }
	void setChannelIndex(int index);

	knob * m_sendKnob;
	SendButtonIndicator * m_sendBtn;

	QBrush backgroundActive() const;
	void setBackgroundActive( const QBrush & c );

	static const int FxLineHeight;

private:
	static void drawFxLine( QPainter* p, const FxLine *fxLine, const QString& name, bool isActive, bool sendToThis, bool receiveFromThis );

	FxMixerView * m_mv;
	LcdWidget* m_lcd;
	int m_channelIndex;
	QBrush m_backgroundActive;
	static QPixmap * s_sendBgArrow;
	static QPixmap * s_receiveBgArrow;

private slots:
	void renameChannel();
	void removeChannel();
	void moveChannelLeft();
	void moveChannelRight();
	void displayHelp();

};


#endif // FXLINE_H
