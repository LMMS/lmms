/*
 * track_container.h - base-class for all track-containers like Song-Editor,
 *                     BB-Editor...
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TRACK_CONTAINER_H
#define _TRACK_CONTAINER_H

#include <QtGui/QScrollArea>
#include <QtCore/QVector>
#include <QtGui/QWidget>


#include "track.h"
#include "journalling_object.h"



const Uint16 DEFAULT_PIXELS_PER_TACT = 16;
const Uint16 DEFAULT_SCROLLBAR_SIZE = 16;



class trackContainer : public QWidget, public journallingObject
{
	Q_OBJECT
public:
	trackContainer( void );
	virtual ~trackContainer();

	inline QWidget * containerWidget( void )
	{
		return( m_scrollArea );
	}

#warning centralWidget is obsolete
	QWidget * centralWidget( void ) const
	{
		return( (QWidget *) this );
	}

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );

	virtual void FASTCALL loadSettings( const QDomElement & _this );


	inline float pixelsPerTact( void ) const
	{
		return( m_ppt );
	}

	inline const midiTime & currentPosition( void ) const
	{
		return( m_currentPosition );
	}

	virtual automationPattern * tempoAutomationPattern( void )
	{
		return( NULL );
	}

	virtual bool fixedTCOs( void ) const
	{
		return( FALSE );
	}

	Uint16 FASTCALL countTracks( track::trackTypes _tt =
					track::TOTAL_TRACK_TYPES ) const;

	void FASTCALL setMutedOfAllTracks( bool _muted );


	virtual void updateAfterTrackAdd( void );
	void FASTCALL setPixelsPerTact( Uint16 _ppt );
	void FASTCALL addTrack( track * _track );
	void FASTCALL removeTrack( track * _track );
	void FASTCALL moveTrackUp( track * _track );
	void FASTCALL moveTrackDown( track * _track );

	void FASTCALL realignTracks( bool _complete_update = FALSE );
	void clearAllTracks( void );

	const trackWidget * trackWidgetAt( const int _y ) const;

	virtual bool allowRubberband( void ) const;

	inline bool rubberBandActive( void ) const
	{
		return( m_rubberBand->isVisible() );
	}

	inline QVector<selectableObject *> selectedObjects( void )
	{
		if( allowRubberband() == TRUE )
		{
			return( m_rubberBand->selectedObjects() );
		}
		return( QVector<selectableObject *>() );
/*		QVector<selectableObject *> foo;
		return( foo );*/
	}

	trackVector tracks( void );

	static const QString classNodeName( void )
	{
		return( "trackcontainer" );
	}


protected:
	virtual void undoStep( journalEntry & _je );
	virtual void redoStep( journalEntry & _je );

	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );

	virtual void resizeEvent( QResizeEvent * );

	constTrackVector tracks( void ) const;

	virtual QRect scrollAreaRect( void ) const
	{
		return( rect() );
	}

	midiTime m_currentPosition;

protected slots:
	void updateScrollArea( void );


private:
	enum actions
	{
		ADD_TRACK, REMOVE_TRACK
	} ;

	class scrollArea : public QScrollArea
	{
	public:
		scrollArea( trackContainer * _parent );
		virtual ~scrollArea();

	protected:
		virtual void wheelEvent( QWheelEvent * _we );

	private:
		trackContainer * m_trackContainer;

	} ;


	scrollArea * m_scrollArea;
	typedef QVector<trackWidget *> trackWidgetVector; 

	trackWidgetVector m_trackWidgets;
	float m_ppt;

	rubberBand * m_rubberBand;
	QPoint m_origin;


signals:
	void positionChanged( const midiTime & _pos );


} ;


#endif
