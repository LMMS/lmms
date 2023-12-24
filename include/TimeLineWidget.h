/*
 * TimeLineWidget.h - class timeLine, representing a time-line with position marker
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_TIMELINE_WIDGET_H
#define LMMS_GUI_TIMELINE_WIDGET_H

#include <QWidget>

#include "Song.h"
#include "embed.h"


class QPixmap;
class QToolBar;

namespace lmms {

class Timeline;

} // namespace lmms

namespace lmms::gui
{

class NStateButton;
class TextFloat;
class SongEditor;


class TimeLineWidget : public QWidget
{
	Q_OBJECT
public:
	Q_PROPERTY( QColor barLineColor READ getBarLineColor WRITE setBarLineColor )
	Q_PROPERTY( QColor barNumberColor READ getBarNumberColor WRITE setBarNumberColor )
	Q_PROPERTY( QColor inactiveLoopColor READ getInactiveLoopColor WRITE setInactiveLoopColor )
	Q_PROPERTY( QBrush inactiveLoopBrush READ getInactiveLoopBrush WRITE setInactiveLoopBrush )
	Q_PROPERTY( QColor inactiveLoopInnerColor READ getInactiveLoopInnerColor WRITE setInactiveLoopInnerColor )
	Q_PROPERTY( QColor activeLoopColor READ getActiveLoopColor WRITE setActiveLoopColor )
	Q_PROPERTY( QBrush activeLoopBrush READ getActiveLoopBrush WRITE setActiveLoopBrush )
	Q_PROPERTY( QColor activeLoopInnerColor READ getActiveLoopInnerColor WRITE setActiveLoopInnerColor )
	Q_PROPERTY( int loopRectangleVerticalPadding READ getLoopRectangleVerticalPadding WRITE setLoopRectangleVerticalPadding )

	enum class AutoScrollState
	{
		Enabled,
		Disabled
	};

	TimeLineWidget(int xoff, int yoff, float ppb, Song::PlayPos& pos, Timeline& timeline,
				const TimePos& begin, Song::PlayMode mode, QWidget* parent);
	~TimeLineWidget() override;

	inline QColor const & getBarLineColor() const { return m_barLineColor; }
	inline void setBarLineColor(QColor const & barLineColor) { m_barLineColor = barLineColor; }

	inline QColor const & getBarNumberColor() const { return m_barNumberColor; }
	inline void setBarNumberColor(QColor const & barNumberColor) { m_barNumberColor = barNumberColor; }

	inline QColor const & getInactiveLoopColor() const { return m_inactiveLoopColor; }
	inline void setInactiveLoopColor(QColor const & inactiveLoopColor) { m_inactiveLoopColor = inactiveLoopColor; }

	inline QBrush const & getInactiveLoopBrush() const { return m_inactiveLoopBrush; }
	inline void setInactiveLoopBrush(QBrush const & inactiveLoopBrush) { m_inactiveLoopBrush = inactiveLoopBrush; }

	inline QColor const & getInactiveLoopInnerColor() const { return m_inactiveLoopInnerColor; }
	inline void setInactiveLoopInnerColor(QColor const & inactiveLoopInnerColor) { m_inactiveLoopInnerColor = inactiveLoopInnerColor; }

	inline QColor const & getActiveLoopColor() const { return m_activeLoopColor; }
	inline void setActiveLoopColor(QColor const & activeLoopColor) { m_activeLoopColor = activeLoopColor; }

	inline QBrush const & getActiveLoopBrush() const { return m_activeLoopBrush; }
	inline void setActiveLoopBrush(QBrush const & activeLoopBrush) { m_activeLoopBrush = activeLoopBrush; }

	inline QColor const & getActiveLoopInnerColor() const { return m_activeLoopInnerColor; }
	inline void setActiveLoopInnerColor(QColor const & activeLoopInnerColor) { m_activeLoopInnerColor = activeLoopInnerColor; }

	inline int const & getLoopRectangleVerticalPadding() const { return m_loopRectangleVerticalPadding; }
	inline void setLoopRectangleVerticalPadding(int const & loopRectangleVerticalPadding) { m_loopRectangleVerticalPadding = loopRectangleVerticalPadding; }

	inline Song::PlayPos & pos()
	{
		return( m_pos );
	}

	AutoScrollState autoScroll() const
	{
		return m_autoScroll;
	}

	inline void setPixelsPerBar( float ppb )
	{
		m_ppb = ppb;
		update();
	}

	void setXOffset(const int x);

	void addToolButtons(QToolBar* _tool_bar );

	inline int markerX( const TimePos & _t ) const
	{
		return m_xOffset + static_cast<int>( ( _t - m_begin ) *
					m_ppb / TimePos::ticksPerBar() );
	}

signals:
	void positionChanged(const lmms::TimePos& postion);
	void regionSelectedFromPixels( int, int );
	void selectionFinished();

public slots:
	void updatePosition();
	void setSnapSize( const float snapSize )
	{
		m_snapSize = snapSize;
	}
	void toggleAutoScroll( int _n );

protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseMoveEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;


private:
	QPixmap m_posMarkerPixmap = embed::getIconPixmap("playpos_marker");

	QColor m_inactiveLoopColor;
	QBrush m_inactiveLoopBrush;
	QColor m_inactiveLoopInnerColor;

	QColor m_activeLoopColor;
	QBrush m_activeLoopBrush;
	QColor m_activeLoopInnerColor;

	int m_loopRectangleVerticalPadding;

	QColor m_barLineColor;
	QColor m_barNumberColor;

	AutoScrollState m_autoScroll;

	bool m_changedPosition;

	int m_xOffset;
	int m_posMarkerX;
	float m_ppb;
	float m_snapSize;
	Song::PlayPos & m_pos;
	Timeline* m_timeline;
	const TimePos & m_begin;
	const Song::PlayMode m_mode;

	TextFloat * m_hint;
	int m_initalXSelect;


	enum class Action
	{
		NoAction,
		MovePositionMarker,
		MoveLoopBegin,
		MoveLoopEnd,
		SelectSongClip,
	} m_action;

	int m_moveXOff;
};

} // namespace lmms::gui

#endif // LMMS_GUI_TIMELINE_WIDGET_H
