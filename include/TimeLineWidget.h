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

#include <array>

#include <QSize>
#include <QWidget>

#include "Song.h"
#include "embed.h"


class QToolBar;

namespace lmms {

class Timeline;

} // namespace lmms

namespace lmms::gui
{

class TextFloat;


class TimeLineWidget : public QWidget
{
	Q_OBJECT
public:
	Q_PROPERTY( QColor barLineColor READ getBarLineColor WRITE setBarLineColor )
	Q_PROPERTY( QColor barNumberColor READ getBarNumberColor WRITE setBarNumberColor )
	Q_PROPERTY( QColor inactiveLoopColor READ getInactiveLoopColor WRITE setInactiveLoopColor )
	Q_PROPERTY( QBrush inactiveLoopBrush READ getInactiveLoopBrush WRITE setInactiveLoopBrush )
	Q_PROPERTY( QColor inactiveLoopInnerColor READ getInactiveLoopInnerColor WRITE setInactiveLoopInnerColor )
	Q_PROPERTY(QColor inactiveLoopHandleColor MEMBER m_inactiveLoopHandleColor)
	Q_PROPERTY( QColor activeLoopColor READ getActiveLoopColor WRITE setActiveLoopColor )
	Q_PROPERTY( QBrush activeLoopBrush READ getActiveLoopBrush WRITE setActiveLoopBrush )
	Q_PROPERTY( QColor activeLoopInnerColor READ getActiveLoopInnerColor WRITE setActiveLoopInnerColor )
	Q_PROPERTY(QColor activeLoopHandleColor MEMBER m_activeLoopHandleColor)
	Q_PROPERTY( int loopRectangleVerticalPadding READ getLoopRectangleVerticalPadding WRITE setLoopRectangleVerticalPadding )
	Q_PROPERTY(int loopHandleWidth MEMBER m_loopHandleWidth)
	Q_PROPERTY(QSize mouseHotspotSelLeft READ mouseHotspotSelLeft WRITE setMouseHotspotSelLeft)
	Q_PROPERTY(QSize mouseHotspotSelRight READ mouseHotspotSelRight WRITE setMouseHotspotSelRight)

	enum class AutoScrollState
	{
		Stepped,
		Continuous,
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

	auto mouseHotspotSelLeft() const -> QSize
	{
		const auto point = m_cursorSelectLeft.hotSpot();
		return QSize{point.x(), point.y()};
	}

	void setMouseHotspotSelLeft(const QSize& s)
	{
		m_cursorSelectLeft = QCursor{m_cursorSelectLeft.pixmap(), s.width(), s.height()};
	}

	auto mouseHotspotSelRight() const -> QSize
	{
		const auto point = m_cursorSelectRight.hotSpot();
		return QSize{point.x(), point.y()};
	}

	void setMouseHotspotSelRight(const QSize& s)
	{
		m_cursorSelectRight = QCursor{m_cursorSelectRight.pixmap(), s.width(), s.height()};
	}

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

	bool isRecoridng = false;
	bool isPlayheadVisible = true;

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
	void contextMenuEvent(QContextMenuEvent* event) override;

private:
	enum class Action
	{
		NoAction,
		MovePositionMarker,
		MoveLoopBegin,
		MoveLoopEnd,
		MoveLoop,
		SelectSongClip,
	};

	auto getClickedTime(int xPosition) const -> TimePos;
	auto getLoopAction(QMouseEvent* event) const -> Action;
	auto actionCursor(Action action) const -> QCursor;

	QPixmap m_posMarkerPixmap = embed::getIconPixmap("playpos_marker");
	QPixmap m_recordingPosMarkerPixmap = embed::getIconPixmap("recording_playpos_marker");

	QColor m_inactiveLoopColor = QColor{52, 63, 53, 64};
	QBrush m_inactiveLoopBrush = QColor{255, 255, 255, 32};
	QColor m_inactiveLoopInnerColor = QColor{255, 255, 255, 32};
	QColor m_inactiveLoopHandleColor = QColor{255, 255, 255, 32};

	QColor m_activeLoopColor = QColor{52, 63, 53, 255};
	QBrush m_activeLoopBrush = QColor{55, 141, 89};
	QColor m_activeLoopInnerColor = QColor{74, 155, 100, 255};
	QColor m_activeLoopHandleColor = QColor{74, 155, 100, 255};

	int m_loopRectangleVerticalPadding = 1;
	int m_loopHandleWidth = 5;

	QColor m_barLineColor = QColor{192, 192, 192};
	QColor m_barNumberColor = m_barLineColor.darker(120);

	QCursor m_cursorSelectLeft = QCursor{embed::getIconPixmap("cursor_select_left"), 0, 16};
	QCursor m_cursorSelectRight = QCursor{embed::getIconPixmap("cursor_select_right"), 32, 16};

	AutoScrollState m_autoScroll = AutoScrollState::Stepped;

	// Width of the unused region on the widget's left (above track labels or piano)
	int m_xOffset;
	float m_ppb;
	float m_snapSize = 1.f;
	Song::PlayPos & m_pos;
	Timeline* m_timeline;
	// Leftmost position visible in parent editor
	const TimePos & m_begin;
	const Song::PlayMode m_mode;
	// When in MoveLoop mode we need the initial positions. Storing only the latest
	// position allows for unquantized drag but fails when toggling quantization.
	std::array<TimePos, 2> m_oldLoopPos;
	TimePos m_dragStartPos;

	TextFloat* m_hint = nullptr;
	int m_initalXSelect;

	Action m_action = Action::NoAction;
};

} // namespace lmms::gui

#endif // LMMS_GUI_TIMELINE_WIDGET_H
