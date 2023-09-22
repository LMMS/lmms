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


class QPixmap;
class QToolBar;

namespace lmms::gui
{

class NStateButton;
class TextFloat;
class SongEditor;


class TimeLineWidget : public QWidget, public JournallingObject
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
	} ;

	enum class LoopPointState
	{
		Disabled,
		Enabled
	} ;

	enum class BehaviourAtStopState
	{
		BackToZero,
		BackToStart,
		KeepStopPosition
	} ;


	TimeLineWidget(int xoff, int yoff, float ppb, Song::PlayPos & pos,
				const TimePos & begin, Song::PlayMode mode, QWidget * parent);
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

	BehaviourAtStopState behaviourAtStop() const
	{
		return m_behaviourAtStop;
	}

	void setBehaviourAtStop (int state)
	{
		emit loadBehaviourAtStop (state);
	}

	bool loopPointsEnabled() const
	{
		return m_loopPoints == LoopPointState::Enabled;
	}

	inline const TimePos & loopBegin() const
	{
		return ( m_loopPos[0] < m_loopPos[1] ) ?
						m_loopPos[0] : m_loopPos[1];
	}

	inline const TimePos & loopEnd() const
	{
		return ( m_loopPos[0] > m_loopPos[1] ) ?
						m_loopPos[0] : m_loopPos[1];
	}

	inline void savePos( const TimePos & pos )
	{
		m_savedPos = pos;
	}
	inline const TimePos & savedPos() const
	{
		return m_savedPos;
	}

	inline void setPixelsPerBar( float ppb )
	{
		m_ppb = ppb;
		update();
	}

	void setXOffset(const int x);

	void addToolButtons(QToolBar* _tool_bar );


	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "timeline";
	}

	inline int markerX( const TimePos & _t ) const
	{
		return m_xOffset + static_cast<int>( ( _t - m_begin ) *
					m_ppb / TimePos::ticksPerBar() );
	}

signals:

	void regionSelectedFromPixels( int, int );
	void selectionFinished();


public slots:
	void updatePosition( const lmms::TimePos & );
	void updatePosition()
	{
		updatePosition( TimePos() );
	}
	void setSnapSize( const float snapSize )
	{
		m_snapSize = snapSize;
	}
	void toggleAutoScroll( int _n );
	void toggleLoopPoints( int _n );
	void toggleBehaviourAtStop( int _n );


protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseMoveEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;


private:
	QPixmap* m_posMarkerPixmap;

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
	LoopPointState m_loopPoints;
	BehaviourAtStopState m_behaviourAtStop;

	bool m_changedPosition;

	int m_xOffset;
	int m_posMarkerX;
	float m_ppb;
	float m_snapSize;
	Song::PlayPos & m_pos;
	const TimePos & m_begin;
	const Song::PlayMode m_mode;
	TimePos m_loopPos[2];

	TimePos m_savedPos;


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


signals:
	void positionChanged( const lmms::TimePos & _t );
	void loopPointStateLoaded( int _n );
	void positionMarkerMoved();
	void loadBehaviourAtStop( int _n );

} ;



} // namespace lmms::gui

#endif // LMMS_GUI_TIMELINE_WIDGET_H
