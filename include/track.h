/*
 * track.h - declaration of classes concerning tracks -> neccessary for all
 *           track-like objects (beat/bassline, sample-track...)
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


#ifndef _TRACK_H
#define _TRACK_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QVector>
#include <QList>
#include <QWidget>

#else

#include <qwidget.h>
#include <qvaluevector.h>
#include <qvaluelist.h>

#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "types.h"
#include "midi_time.h"
#include "settings.h"


class track;
class trackContainer;
class trackContentWidget;
class trackWidget;
class pixmapButton;
class QMenu;

typedef QWidget trackSettingsWidget;
typedef QWidget trackOperationsWidget;



const Uint16 DEFAULT_SETTINGS_WIDGET_WIDTH = 224;
const Uint16 TRACK_OP_WIDTH = 70;
const Uint16 TCO_BORDER_WIDTH = 1;


class trackContentObject : public QWidget, public settings
{
	Q_OBJECT
public:
	trackContentObject( track * _track );
	trackContentObject( const trackContentObject & _copy );
	virtual ~trackContentObject();
	inline track * getTrack( void )
	{
		return( m_track );
	}
	inline const midiTime & startPosition( void ) const
	{
		return( m_startPosition );
	}
	inline midiTime endPosition( void ) const
	{
		return( m_startPosition + m_length );
	}
	inline const midiTime & length( void ) const
	{
		return( m_length );
	}
	bool fixedTCOs( void );
	virtual void FASTCALL movePosition( const midiTime & _pos );
	virtual void FASTCALL changeLength( const midiTime & _length );


public slots:
	virtual void close( void );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void contextMenuEvent( QContextMenuEvent * _cme );
	virtual void constructContextMenu( QMenu * )
	{
	}
	void setAutoResizeEnabled( bool _e = FALSE );
	float pixelsPerTact( void );


protected slots:
	void cut( void );
	void copy( void );
	void paste( void );


private:
	track * m_track;
	midiTime m_startPosition;
	midiTime m_length;
	bool m_moving;
	bool m_resizing;
	bool m_autoResize;
	Sint16 m_initialMouseX;

} ;




class trackContentWidget : public QWidget
{
	Q_OBJECT
public:
	trackContentWidget( trackWidget * _parent );
	virtual ~trackContentWidget();

	trackContentObject * FASTCALL getTCO( csize _tco_num );
	csize numOfTCOs( void );
	trackContentObject * FASTCALL addTCO( trackContentObject * _tco );
	void FASTCALL removeTCO( csize _tco_num, bool _also_delete = TRUE );
	void FASTCALL removeTCO( trackContentObject * _tco,
						bool _also_delete = TRUE );
	void removeAllTCOs( void );
	void FASTCALL swapPositionOfTCOs( csize _tco_num1, csize _tco_num2 );

	inline Uint16 pixelsPerTact( void ) const
	{
		return( m_pixelsPerTact );
	}
	inline void setPixelsPerTact( Uint16 _ppt )
	{
		m_pixelsPerTact = _ppt;
	}
	tact length( void ) const;


public slots:
	void insertTact( const midiTime & _pos );
	void removeTact( const midiTime & _pos );
	void updateTCOs( void );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void resizeEvent( QResizeEvent * _re );


private:
	typedef vvector<trackContentObject *> tcoVector;

	tcoVector m_trackContentObjects;
	trackWidget * m_trackWidget;
	Uint16 m_pixelsPerTact;

} ;




// actual widget shown in trackContainer
class trackWidget : public QWidget
{
	Q_OBJECT
public:
	trackWidget( track * _track, QWidget * _parent );
	virtual ~trackWidget();

	inline const track * getTrack( void ) const
	{
		return( m_track );
	}
	inline track * getTrack( void )
	{
		return( m_track );
	}
	inline const trackSettingsWidget & getTrackSettingsWidget( void ) const
	{
		return( m_trackSettingsWidget );
	}
	inline const trackContentWidget & getTrackContentWidget( void ) const
	{
		return( m_trackContentWidget );
	}
	inline trackSettingsWidget & getTrackSettingsWidget( void )
	{
		return( m_trackSettingsWidget );
	}
	inline trackContentWidget & getTrackContentWidget( void )
	{
		return( m_trackContentWidget );
	}
	bool muted( void ) const;


public slots:
	void changePosition( const midiTime & _new_pos = -1 );
	void cloneTrack( void );
	void deleteTrack( void );
	void moveTrackUp( void );
	void moveTrackDown( void );
	void setMuted( bool _muted );
	void muteBtnRightClicked( void );


protected:
	virtual void resizeEvent( QResizeEvent * _re );
	virtual void paintEvent( QPaintEvent * _pe );
	midiTime FASTCALL endPosition( const midiTime & _pos_start );


private:
	track * m_track;

	trackOperationsWidget m_trackOperationsWidget;
	trackSettingsWidget m_trackSettingsWidget;
	trackContentWidget m_trackContentWidget;

	pixmapButton * m_muteBtn;

} ;




// base-class for all tracks
class track : public settings
{
public:
	enum trackTypes
	{
		CHANNEL_TRACK,
		BB_TRACK,
		SAMPLE_TRACK,
		EVENT_TRACK,
		VIDEO_TRACK,
		NULL_TRACK,
		TOTAL_TRACK_TYPES
	} ;

	track( trackContainer * _tc );
	virtual ~track();
	static track * FASTCALL createTrack( trackTypes _tt,
							trackContainer * _tc );
	static track * FASTCALL createTrack( const QDomElement & _this,
							trackContainer * _tc );
	static track * FASTCALL cloneTrack( track * _track );

	tact length( void ) const;

	inline bool muted( void ) const
	{
		return( m_trackWidget->muted() );
	}

	// pure virtual functions
	virtual trackTypes trackType( void ) const = 0;

	virtual bool FASTCALL play( const midiTime & _start,
						Uint32 _start_frame,
						Uint32 _frames,
						Uint32 _frame_base,
						Sint16 _tco_num = -1 ) = 0;


	virtual trackContentObject * FASTCALL createTCO(
						const midiTime & _pos ) = 0;

	virtual void FASTCALL saveTrackSpecificSettings( QDomDocument & _doc,
						QDomElement & _parent ) = 0;
	virtual void FASTCALL loadTrackSpecificSettings(
						const QDomElement & _this ) = 0;


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
						QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );


	trackContentObject * FASTCALL addTCO( trackContentObject * _tco );
	void FASTCALL removeTCO( csize _tco_num );
	csize numOfTCOs( void );
	trackContentObject * FASTCALL getTCO( csize _tco_num );
	csize FASTCALL getTCONum( trackContentObject * _tco );
	void FASTCALL getTCOsInRange( vlist<trackContentObject *> & _tco_v,
							const midiTime & _start,
							const midiTime & _end );
	void FASTCALL swapPositionOfTCOs( csize _tco_num1, csize _tco_num2 );

	inline trackWidget * getTrackWidget( void )
	{
		return( m_trackWidget );
	}
	inline trackContainer * getTrackContainer( void )
	{
		return( m_trackContainer );
	}
	inline const trackSettingsWidget * getTrackSettingsWidget( void ) const
	{
		return( &m_trackWidget->getTrackSettingsWidget() );
	}
	inline const trackContentWidget * getTrackContentWidget( void ) const
	{
		return( &m_trackWidget->getTrackContentWidget() );
	}
	inline trackSettingsWidget * getTrackSettingsWidget( void )
	{
		return( &m_trackWidget->getTrackSettingsWidget() );
	}
	inline trackContentWidget * getTrackContentWidget( void )
	{
		return( &m_trackWidget->getTrackContentWidget() );
	}


private:
	trackContainer * m_trackContainer;
	trackWidget * m_trackWidget;

} ;


typedef vvector<track *> trackVector;
typedef vvector<const track *> constTrackVector;


#endif
