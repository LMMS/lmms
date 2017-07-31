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


#ifndef TIMELINE_H
#define TIMELINE_H

#include <QWidget>
#include <QToolButton>

#include "Song.h"

class QPixmap;
class QToolBar;
class QToolButton;
class MidiTime;
class NStateButton;
class SongEditor;
class TextFloat;

class TimeLineWidget : public QWidget, public JournallingObject
{
	Q_OBJECT
public:
	Q_PROPERTY( QColor barLineColor READ getBarLineColor WRITE setBarLineColor )
	Q_PROPERTY( QColor barNumberColor READ getBarNumberColor WRITE setBarNumberColor )

	Q_PROPERTY( QColor inactiveLoopColor READ getInactiveLoopColor WRITE setInactiveLoopColor )
	Q_PROPERTY( QBrush inactiveLoopBrush READ getInactiveLoopBrush WRITE setInactiveLoopBrush )
	Q_PROPERTY( QColor inactiveLoopInnerColor READ getInactiveLoopInnerColor WRITE setInactiveLoopInnerColor )
	Q_PROPERTY( QColor inactiveLoopTextColor READ getInactiveLoopTextColor WRITE setInactiveLoopTextColor )

	Q_PROPERTY( QColor activeLoopColor READ getActiveLoopColor WRITE setActiveLoopColor )
	Q_PROPERTY( QBrush activeLoopBrush READ getActiveLoopBrush WRITE setActiveLoopBrush )
	Q_PROPERTY( QColor activeLoopInnerColor READ getActiveLoopInnerColor WRITE setActiveLoopInnerColor )
	Q_PROPERTY( QColor activeLoopTextColor READ getActiveLoopTextColor WRITE setActiveLoopTextColor )

	Q_PROPERTY( QColor selectedLoopColor READ getSelectedLoopColor WRITE setSelectedLoopColor )
	Q_PROPERTY( QBrush selectedLoopBrush READ getSelectedLoopBrush WRITE setSelectedLoopBrush )
	Q_PROPERTY( QColor selectedLoopInnerColor READ getSelectedLoopInnerColor WRITE setSelectedLoopInnerColor )
	Q_PROPERTY( QColor selectedLoopTextColor READ getSelectedLoopTextColor WRITE setSelectedLoopTextColor )

	Q_PROPERTY( int loopRectangleVerticalPadding READ getLoopRectangleVerticalPadding WRITE setLoopRectangleVerticalPadding )

	enum AutoScrollStates
	{
		AutoScrollEnabled,
		AutoScrollDisabled
	} ;

	enum LoopPointStates
	{
		LoopPointsDisabled,
		LoopPointsEnabled
	} ;

	enum BehaviourAtStopStates
	{
		BackToZero,
		BackToStart,
		KeepStopPosition
	} ;

	static const int NB_LOOPS=8;

	TimeLineWidget( int xoff, int yoff, float ppt, Song::PlayPos & pos,
				const MidiTime & begin, QWidget * parent );
	virtual ~TimeLineWidget();

	inline QColor const & getBarLineColor() const { return m_barLineColor; }
	inline void setBarLineColor(QColor const & tactLineColor) { m_barLineColor = tactLineColor; }

	inline QColor const & getBarNumberColor() const { return m_barNumberColor; }
	inline void setBarNumberColor(QColor const & tactNumberColor) { m_barNumberColor = tactNumberColor; }

	inline QColor const & getInactiveLoopColor() const { return m_inactiveLoopColor; }
	inline void setInactiveLoopColor(QColor const & inactiveLoopColor) { m_inactiveLoopColor = inactiveLoopColor; }

	inline QBrush const & getInactiveLoopBrush() const { return m_inactiveLoopBrush; }
	inline void setInactiveLoopBrush(QBrush const & inactiveLoopBrush) { m_inactiveLoopBrush = inactiveLoopBrush; }

	inline QColor const & getInactiveLoopInnerColor() const { return m_inactiveLoopInnerColor; }
	inline void setInactiveLoopInnerColor(QColor const & inactiveLoopInnerColor) { m_inactiveLoopInnerColor = inactiveLoopInnerColor; }

	inline QColor const & getInactiveLoopTextColor() const { return m_inactiveLoopTextColor; }
	inline void setInactiveLoopTextColor(QColor const & inactiveLoopTextColor) { m_inactiveLoopTextColor = inactiveLoopTextColor; }

	inline QColor const & getActiveLoopColor() const { return m_activeLoopColor; }
	inline void setActiveLoopColor(QColor const & activeLoopColor) { m_activeLoopColor = activeLoopColor; }

	inline QBrush const & getActiveLoopBrush() const { return m_activeLoopBrush; }
	inline void setActiveLoopBrush(QBrush const & activeLoopBrush) { m_activeLoopBrush = activeLoopBrush; }

	inline QColor const & getActiveLoopInnerColor() const { return m_activeLoopInnerColor; }
	inline void setActiveLoopInnerColor(QColor const & activeLoopInnerColor) { m_activeLoopInnerColor = activeLoopInnerColor; }

	inline QColor const & getActiveLoopTextColor() const { return m_activeLoopTextColor; }
	inline void setActiveLoopTextColor(QColor const & activeLoopTextColor) { m_activeLoopTextColor = activeLoopTextColor; }

	inline QColor const & getSelectedLoopColor() const { return m_selectedLoopColor; }
	inline void setSelectedLoopColor(QColor const & selectedLoopColor) { m_selectedLoopColor = selectedLoopColor; }

	inline QBrush const & getSelectedLoopBrush() const { return m_selectedLoopBrush; }
	inline void setSelectedLoopBrush(QBrush const & selectedLoopBrush) { m_selectedLoopBrush = selectedLoopBrush; }

	inline QColor const & getSelectedLoopInnerColor() const { return m_selectedLoopInnerColor; }
	inline void setSelectedLoopInnerColor(QColor const & selectedLoopInnerColor) { m_selectedLoopInnerColor = selectedLoopInnerColor; }

	inline QColor const & getSelectedLoopTextColor() const { return m_selectedLoopTextColor; }
	inline void setSelectedLoopTextColor(QColor const & selectedLoopTextColor) { m_selectedLoopTextColor = selectedLoopTextColor; }

	inline int const & getLoopRectangleVerticalPadding() const { return m_loopRectangleVerticalPadding; }
	inline void setLoopRectangleVerticalPadding(int const & loopRectangleVerticalPadding) { m_loopRectangleVerticalPadding = loopRectangleVerticalPadding; }

	inline Song::PlayPos & pos()
	{
		return( m_pos );
	}

	AutoScrollStates autoScroll() const
	{
		return m_autoScroll;
	}

	BehaviourAtStopStates behaviourAtStop() const
	{
		return m_behaviourAtStop;
	}

	inline int currentLoop()
	{
		return m_currentLoop;
	}

	void setCurrentLoop(int n);
	int  findLoop(const MidiTime & t);

	bool loopPointsEnabled(int n=-1) const
	{
		if(n==-1) n=m_currentLoop;
		Q_ASSERT ((n>=0)&&(n<NB_LOOPS));

		return (n == m_currentLoop) &&
			(m_loopPoints == LoopPointsEnabled);
	}

	inline const MidiTime & loopBegin(int n=-1) const
	{
		if(n==-1) n=m_currentLoop;
		Q_ASSERT ((n>=0)&&(n<NB_LOOPS));

		return ( m_loopPos[2*n+0] < m_loopPos[2*n+1] )
			? m_loopPos[2*n+0]
			: m_loopPos[2*n+1];
	}

	inline const MidiTime & loopEnd(int n=-1) const
	{
		if(n==-1) n=m_currentLoop;
		Q_ASSERT ((n>=0)&&(n<NB_LOOPS));

		return ( m_loopPos[2*n+0] > m_loopPos[2*n+1] )
			? m_loopPos[2*n+0]
			: m_loopPos[2*n+1];
	}

	inline void savePos( const MidiTime & _pos )
	{
		m_savedPos = _pos;
	}
	inline const MidiTime & savedPos() const
	{
		return m_savedPos;
	}

	inline void setPixelsPerTact( float _ppt )
	{
		m_ppt = _ppt;
		update();
	}

	void addToolButtons(QToolBar* _tool_bar );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "timeline";
	}

	inline int markerX( const MidiTime & _t ) const
	{
		return m_xOffset + static_cast<int>( ( _t - m_begin ) *
					m_ppt / MidiTime::ticksPerTact() );
	}

signals:

	void regionSelectedFromPixels( int, int );
	void selectionFinished();


public slots:
	void updatePosition( const MidiTime & );
	void updatePosition()
	{
		updatePosition( MidiTime() );
	}
	void toggleAutoScroll( int _n );
	void toggleLoopPoints( int _n );
	void toggleBehaviourAtStop( int _n );

	void selectLoop(QAction * _a);
	void selectLoop(const MidiTime & t);


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void paintLoop(const int num, QPainter& p, const int cy);
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	static QPixmap * s_posMarkerPixmap;

	QColor m_inactiveLoopColor;
	QBrush m_inactiveLoopBrush;
	QColor m_inactiveLoopInnerColor;
	QColor m_inactiveLoopTextColor;

	QColor m_activeLoopColor;
	QBrush m_activeLoopBrush;
	QColor m_activeLoopInnerColor;
	QColor m_activeLoopTextColor;

	QColor m_selectedLoopColor;
	QBrush m_selectedLoopBrush;
	QColor m_selectedLoopInnerColor;
	QColor m_selectedLoopTextColor;

	int m_loopRectangleVerticalPadding;

	QColor m_barLineColor;
	QColor m_barNumberColor;

	AutoScrollStates m_autoScroll;
	LoopPointStates m_loopPoints;
	BehaviourAtStopStates m_behaviourAtStop;

	bool m_changedPosition;

	int m_xOffset;
	int m_posMarkerX;
	float m_ppt;
	Song::PlayPos & m_pos;
	const MidiTime & m_begin;
	MidiTime m_loopPos[2*NB_LOOPS];

	MidiTime m_savedPos;

	int m_currentLoop;
	QToolButton * m_loopButton;

	TextFloat * m_hint;
	int m_initalXSelect;


	enum actions
	{
		NoAction,
		MovePositionMarker,
		MoveLoopBegin,
		MoveLoopEnd,
		SelectSongTCO,
	} m_action;

	int m_moveXOff;


signals:
	void positionChanged( const MidiTime & _t );
	void loopPointStateLoaded( int _n );
	void positionMarkerMoved();

} ;


#endif
