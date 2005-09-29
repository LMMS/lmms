/*
 * track_container.h - base-class for all track-containers like Song-Editor,
 *                     BB-Editor...
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _TRACK_CONTAINER_H
#define _TRACK_CONTAINER_H

#include "qt3support.h"

#ifdef QT4

#include <QScrollArea>
#include <QVector>
#include <QMainWindow>

#else

#include <qscrollview.h>
#include <qvaluevector.h>
#include <qmainwindow.h>

#endif


#include "track.h"
#include "settings.h"


const Uint16 DEFAULT_PIXELS_PER_TACT = 16;


class trackContainer : public QMainWindow, public settings
{
	Q_OBJECT
public:
	trackContainer();
	~trackContainer();
	inline QWidget * containerWidget( void )
	{
		return( m_scrollArea );
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
	virtual bool fixedTCOs( void ) const
	{
		return( FALSE );
	}
	unsigned int FASTCALL countTracks( track::trackTypes _tt =
					track::TOTAL_TRACK_TYPES ) const;

	void FASTCALL setMutedOfAllTracks( bool _muted );


	virtual void updateAfterTrackAdd( void );
	void FASTCALL setPixelsPerTact( Uint16 _ppt );
	void FASTCALL cloneTrack( track * _track );
	void FASTCALL addTrack( track * _track );
	void FASTCALL removeTrack( track * _track );
	void FASTCALL moveTrackUp( track * _track );
	void FASTCALL moveTrackDown( track * _track );
	void FASTCALL realignTracks( bool _complete_update = FALSE );


protected:
	constTrackVector tracks( void ) const;
	trackVector tracks( void );

	virtual void resizeEvent( QResizeEvent * );

	midiTime m_currentPosition;


protected slots:
	void updateScrollArea( void );


private:
	QScrollArea * m_scrollArea;
	typedef vvector<trackWidget *> trackWidgetVector; 

	trackWidgetVector m_trackWidgets;
	float m_ppt;


signals:
	void positionChanged( const midiTime & _pos );

} ;


#endif
