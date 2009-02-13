/*
 * track_container_view.h - view-component for trackContainer
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TRACK_CONTAINER_VIEW_H
#define _TRACK_CONTAINER_VIEW_H

#include <QtCore/QVector>
#include <QtGui/QScrollArea>
#include <QtGui/QWidget>


#include "track.h"
#include "journalling_object.h"
#include "fluiq/collapsible_widget.h"


class QVBoxLayout;
class trackContainer;


class trackContainerView : public FLUIQ::CollapsibleWidget,
						public modelView,
						public journallingObject,
						public serializingObjectHook
{
	Q_OBJECT
public:
	trackContainerView( trackContainer * _tc );
	virtual ~trackContainerView();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );

	QWidget * contentWidget( void )
	{
		return m_scrollArea;
	}

	inline const midiTime & currentPosition( void ) const
	{
		return m_currentPosition;
	}

	virtual bool fixedTCOs( void ) const
	{
		return false;
	}

	inline float pixelsPerTact( void ) const
	{
		return m_ppt;
	}

	void setPixelsPerTact( int _ppt );

	const trackView * trackViewAt( const int _y ) const;

	virtual bool allowRubberband( void ) const;

	inline bool rubberBandActive( void ) const
	{
		return m_rubberBand->isVisible();
	}

	inline QVector<selectableObject *> selectedObjects( void )
	{
		if( allowRubberband() == true )
		{
			return m_rubberBand->selectedObjects();
		}
		return QVector<selectableObject *>();
	}


	trackContainer * model( void )
	{
		return m_tc;
	}

	const trackContainer * model( void ) const
	{
		return m_tc;
	}

	void moveTrackViewUp( trackView * _tv );
	void moveTrackViewDown( trackView * _tv );

	// -- for usage by trackView only ---------------
	trackView * addTrackView( trackView * _tv );
	void removeTrackView( trackView * _tv );
	// -------------------------------------------------------

	void clearAllTracks( void );

	virtual QString nodeName( void ) const
	{
		return "trackcontainerview";
	}


public slots:
	void realignTracks( void );
	void createTrackView( track * _t );
	void deleteTrackView( trackView * _tv );


protected:
	static const int DEFAULT_PIXELS_PER_TACT = 16;

	const QList<trackView *> & trackViews( void ) const
	{
		return m_trackViews;
	}

	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void resizeEvent( QResizeEvent * );

	virtual void undoStep( journalEntry & _je );
	virtual void redoStep( journalEntry & _je );

	midiTime m_currentPosition;


private:
	enum Actions
	{
		AddTrack,
		RemoveTrack
	} ;

	class scrollArea : public QScrollArea
	{
	public:
		scrollArea( trackContainerView * _parent );
		virtual ~scrollArea();

	protected:
		virtual void wheelEvent( QWheelEvent * _we );

	private:
		trackContainerView * m_trackContainerView;

	} ;

	trackContainer * m_tc;
	typedef QList<trackView *> trackViewList;
	trackViewList m_trackViews;

	scrollArea * m_scrollArea;
	QVBoxLayout * m_scrollLayout;

	float m_ppt;

	rubberBand * m_rubberBand;
	QPoint m_origin;


signals:
	void positionChanged( const midiTime & _pos );


} ;



#endif
