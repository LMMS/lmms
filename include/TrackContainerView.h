/*
 * TrackContainerView.h - view-component for TrackContainer
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef TRACK_CONTAINER_VIEW_H
#define TRACK_CONTAINER_VIEW_H

#include <QtCore/QVector>
#include <QScrollArea>
#include <QWidget>
#include <QThread>

#include "Track.h"
#include "JournallingObject.h"
#include "InstrumentTrack.h"


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

	QScrollArea * contentWidget()
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

	const TrackView * trackViewAt( const int _y ) const;

	virtual bool allowRubberband() const;

	inline bool rubberBandActive() const
	{
		return( m_rubberBand->isEnabled() && m_rubberBand->isVisible() );
	}

	inline QVector<selectableObject *> selectedObjects()
	{
		return( m_rubberBand->selectedObjects() );
	}


	TrackContainer* model()
	{
		return m_tc;
	}

	const TrackContainer* model() const
	{
		return m_tc;
	}

	const QList<TrackView *> & trackViews() const
	{
		return( m_trackViews );
	}

	void moveTrackView( TrackView * trackView, int indexTo );
	void moveTrackViewUp( TrackView * trackView );
	void moveTrackViewDown( TrackView * trackView );
	void scrollToTrackView( TrackView * _tv );

	// -- for usage by trackView only ---------------
	TrackView * addTrackView( TrackView * _tv );
	void removeTrackView( TrackView * _tv );
	// -------------------------------------------------------

	void clearAllTracks();

	virtual QString nodeName() const
	{
		return( "trackcontainerview" );
	}

	inline QVector<QPointer<HyperBarView> >& hyperBarViews() { return m_hyperBarViews; }
	inline QVector<QPointer<BarView> >& barViews() { return m_barViews; }

	RubberBand *rubberBand() const;

public slots:
	void realignTracks();
	void updateBackgrounds();
	TrackView * createTrackView( Track * _t );
	void deleteTrackView( TrackView * _tv );

	virtual void dropEvent( QDropEvent * _de );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	///
	/// \brief selectRegionFromPixels
	/// \param x
	/// \param y
	/// Use the rubber band to select TCO from all tracks using x, y pixels
	void selectRegionFromPixels(int xStart, int xEnd);

	///
	/// \brief stopRubberBand
	/// Removes the rubber band from display when finished with.
	void stopRubberBand();

protected:
	static const int DEFAULT_PIXELS_PER_TACT = 16;

	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void resizeEvent( QResizeEvent * );

	virtual void computeHyperBarViews();
	virtual void computeBarViews();

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
	typedef QList<TrackView *> trackViewList;
	trackViewList m_trackViews;

	scrollArea * m_scrollArea;
	QVBoxLayout * m_scrollLayout;

	float m_ppt;

	RubberBand * m_rubberBand;
	QPoint m_origin;

	QVector<QPointer<HyperBarView> > m_hyperBarViews;
	QVector<QPointer<BarView> > m_barViews;

signals:
	void positionChanged( const MidiTime & _pos );


} ;

class InstrumentLoaderThread : public QThread
{
	Q_OBJECT
public:
	InstrumentLoaderThread( QObject *parent = 0, InstrumentTrack *it = 0,
							QString name = "" );

	void run();

private:
	InstrumentTrack *m_it;
	QString m_name;
	QThread *m_containerThread;
};

#endif
