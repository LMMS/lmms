/*
 * track_container_scene.cpp - A TrackContainer is represented in the
 * SongEditor as the GraphicsScene.  This is it.
 *
 * Copyright (c) 2009 Paul Giblock <pgib/at/users.sourceforge.net>
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



#ifndef _TRACK_CONTAINER_SCENE_H
#define _TRACK_CONTAINER_SCENE_H

#include <QtCore/QVector>
#include <QtGui/QGraphicsScene>

#include "track.h"

class trackContainer;
class TrackContentObjectItem;
class TrackItem;

class TrackContainerScene : public QGraphicsScene
{
	Q_OBJECT

public:
	const static float DEFAULT_CELL_HEIGHT = 32;
	const static float DEFAULT_CELL_WIDTH = 16;
	
	TrackContainerScene( QObject * parent, trackContainer * _tc );
	virtual ~TrackContainerScene();


	inline const midiTime & currentPosition() const
	{
		return( m_currentPosition );
	}

	inline float pixelsPerTact() const
	{
		return( m_ppt );
	}


	void setPixelsPerTact( float _ppt );

	trackContainer * model()
	{
		return( m_trackContainer );
	}

	const trackContainer * model() const
	{
		return( m_trackContainer );
	}


	virtual QString nodeName() const
	{
		return( "trackcontainerscene" );
	}

private slots:

	void addTrack( track * _t );
	void removeTrack( track * _t );

	/*
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void resizeEvent( QResizeEvent * );
	virtual void undoStep( journalEntry & _je );
	virtual void redoStep( journalEntry & _je );
	*/

protected:
	midiTime m_currentPosition;
	trackContainer * m_trackContainer;
	float m_ppt;

	//QVector<TrackContentItem*> m_trackItems;
	QMap<track*, TrackItem*> m_trackItems;

    virtual void keyPressEvent( QKeyEvent * event );

private:


//signals:
	//void positionChanged( const midiTime & _pos );


} ;



#endif
