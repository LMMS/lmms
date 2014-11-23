/*
 * TrackContainerView.h - view-component for TrackContainer
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef TRACK_CONTAINER_VIEW_H
#define TRACK_CONTAINER_VIEW_H

#include <QtCore/QVector>
#include <QtGui/QScrollArea>
#include <QtGui/QWidget>


#include "track.h"
#include "JournallingObject.h"


class QVBoxLayout;
class TrackContainer;


class TrackContainerView : public QWidget, public ModelView,
						public JournallingObject,
						public SerializingObjectHook
{
	Q_OBJECT
public:
	TrackContainerView( TrackContainer* tc );
	virtual ~TrackContainerView();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );

	QWidget * contentWidget()
	{
		return( m_scrollArea );
	}

	inline const MidiTime & currentPosition() const
	{
		return( m_currentPosition );
	}

	virtual bool fixedTCOs() const
	{
		return( false );
	}

	inline float pixelsPerTact() const
	{
		return( m_ppt );
	}

	void setPixelsPerTact( int _ppt );

	const trackView * trackViewAt( const int _y ) const;

	virtual bool allowRubberband() const;

	inline bool rubberBandActive() const
	{
		return( m_rubberBand->isVisible() );
	}

	inline QVector<selectableObject *> selectedObjects()
	{
		if( allowRubberband() == true )
		{
			return( m_rubberBand->selectedObjects() );
		}
		return( QVector<selectableObject *>() );
	}


	TrackContainer* model()
	{
		return m_tc;
	}

	const TrackContainer* model() const
	{
		return m_tc;
	}

	void moveTrackViewUp( trackView * _tv );
	void moveTrackViewDown( trackView * _tv );

	// -- for usage by trackView only ---------------
	trackView * addTrackView( trackView * _tv );
	void removeTrackView( trackView * _tv );
	// -------------------------------------------------------

	void clearAllTracks();

	virtual QString nodeName() const
	{
		return( "trackcontainerview" );
	}


public slots:
	void realignTracks();
	void createTrackView( track * _t );
	void deleteTrackView( trackView * _tv );


protected:
	static const int DEFAULT_PIXELS_PER_TACT = 16;

	const QList<trackView *> & trackViews() const
	{
		return( m_trackViews );
	}

	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void resizeEvent( QResizeEvent * );

	MidiTime m_currentPosition;


private:
	enum Actions
	{
		AddTrack,
		RemoveTrack
	} ;

	class scrollArea : public QScrollArea
	{
	public:
		scrollArea( TrackContainerView* parent );
		virtual ~scrollArea();

	protected:
		virtual void wheelEvent( QWheelEvent * _we );

	private:
		TrackContainerView* m_trackContainerView;

	} ;

	TrackContainer* m_tc;
	typedef QList<trackView *> trackViewList;
	trackViewList m_trackViews;

	scrollArea * m_scrollArea;
	QVBoxLayout * m_scrollLayout;

	float m_ppt;

	rubberBand * m_rubberBand;
	QPoint m_origin;


signals:
	void positionChanged( const MidiTime & _pos );


} ;



#endif
