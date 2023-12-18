/*
 * MixerLine.h - Mixer line widget
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

#ifndef LMMS_GUI_MIXER_LINE_H
#define LMMS_GUI_MIXER_LINE_H

#include <QWidget>

class QGraphicsView;
class QLineEdit;

namespace lmms::gui
{


class Knob;
class LcdWidget;
class MixerView;
class SendButtonIndicator;

class MixerLine : public QWidget
{
	Q_OBJECT
public:
	Q_PROPERTY( QBrush backgroundActive READ backgroundActive WRITE setBackgroundActive )
	Q_PROPERTY( QColor strokeOuterActive READ strokeOuterActive WRITE setStrokeOuterActive )
	Q_PROPERTY( QColor strokeOuterInactive READ strokeOuterInactive WRITE setStrokeOuterInactive )
	Q_PROPERTY( QColor strokeInnerActive READ strokeInnerActive WRITE setStrokeInnerActive )
	Q_PROPERTY( QColor strokeInnerInactive READ strokeInnerInactive WRITE setStrokeInnerInactive )
	MixerLine( QWidget * _parent, MixerView * _mv, int _channelIndex);
	~MixerLine() override;

	void paintEvent( QPaintEvent * ) override;
	void mousePressEvent( QMouseEvent * ) override;
	void mouseDoubleClickEvent( QMouseEvent * ) override;
	void contextMenuEvent( QContextMenuEvent * ) override;

	inline int channelIndex() { return m_channelIndex; }
	void setChannelIndex(int index);

	Knob * m_sendKnob;
	SendButtonIndicator * m_sendBtn;

	QBrush backgroundActive() const;
	void setBackgroundActive( const QBrush & c );
	
	QColor strokeOuterActive() const;
	void setStrokeOuterActive( const QColor & c );
	
	QColor strokeOuterInactive() const;
	void setStrokeOuterInactive( const QColor & c );
	
	QColor strokeInnerActive() const;
	void setStrokeInnerActive( const QColor & c );
	
	QColor strokeInnerInactive() const;
	void setStrokeInnerInactive( const QColor & c );

	static const int MixerLineHeight;

	bool eventFilter (QObject *dist, QEvent *event) override;

private:
	void drawMixerLine( QPainter* p, const MixerLine *mixerLine, bool isActive, bool sendToThis, bool receiveFromThis );
	QString elideName( const QString & name );

	MixerView * m_mv;
	LcdWidget* m_lcd;
	int m_channelIndex;
	QBrush m_backgroundActive;
	QColor m_strokeOuterActive;
	QColor m_strokeOuterInactive;
	QColor m_strokeInnerActive;
	QColor m_strokeInnerInactive;
	bool m_inRename;
	QLineEdit * m_renameLineEdit;
	QGraphicsView * m_view;

public slots:
	void renameChannel();
	void resetColor();
	void selectColor();
	void randomizeColor();

private slots:
	void renameFinished();
	void removeChannel();
	void removeUnusedChannels();
	void moveChannelLeft();
	void moveChannelRight();
};


} // namespace lmms::gui

#endif // LMMS_GUI_MIXER_LINE_H
