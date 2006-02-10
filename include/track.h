/*
 * track.h - declaration of classes concerning tracks -> neccessary for all
 *           track-like objects (beat/bassline, sample-track...)
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "rubberband.h"
#include "engine.h"


class QMenu;
class QPushButton;

class pixmapButton;
class textFloat;
class track;
class trackContainer;
class trackContentWidget;
class trackWidget;

typedef QWidget trackSettingsWidget;



const Uint16 DEFAULT_SETTINGS_WIDGET_WIDTH = 224;
const Uint16 TRACK_OP_WIDTH = 70;
const Uint16 TCO_BORDER_WIDTH = 1;


class trackContentObject : public selectableObject, public settings,
			   public engineObject
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
	virtual void constructContextMenu( QMenu * )
	{
	}

	virtual void contextMenuEvent( QContextMenuEvent * _cme );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void leaveEvent( QEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );

	void setAutoResizeEnabled( bool _e = FALSE );
	float pixelsPerTact( void );


protected slots:
	void cut( void );
	void copy( void );
	void paste( void );


private:
	enum actions
	{
		NONE, MOVE, MOVE_SELECTION, RESIZE
	} ;

	static textFloat * s_textFloat;

	track * m_track;
	midiTime m_startPosition;
	midiTime m_length;
	actions m_action;
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
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );


private:
	track * getTrack( void );
	midiTime getPosition( int _mouse_x );

	typedef vvector<trackContentObject *> tcoVector;

	tcoVector m_trackContentObjects;
	trackWidget * m_trackWidget;
	Uint16 m_pixelsPerTact;

} ;





class trackOperationsWidget : public QWidget
{
	Q_OBJECT
public:
	trackOperationsWidget( trackWidget * _parent );
	~trackOperationsWidget();

	bool muted( void ) const;


public slots:
	void setMuted( bool _muted );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );


private slots:
	void cloneTrack( void );
	void removeTrack( void );
	void muteBtnRightClicked( void );


private:
	static QPixmap * s_grip;

	trackWidget * m_trackWidget;

	QPushButton * m_trackOps;
	pixmapButton * m_muteBtn;

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

	inline const trackOperationsWidget & getTrackOperationsWidget( void )
									const
	{
		return( m_trackOperationsWidget );
	}

	inline const trackSettingsWidget & getTrackSettingsWidget( void ) const
	{
		return( m_trackSettingsWidget );
	}

	inline const trackContentWidget & getTrackContentWidget( void ) const
	{
		return( m_trackContentWidget );
	}

	inline trackOperationsWidget & getTrackOperationsWidget( void )
	{
		return( m_trackOperationsWidget );
	}

	inline trackSettingsWidget & getTrackSettingsWidget( void )
	{
		return( m_trackSettingsWidget );
	}

	inline trackContentWidget & getTrackContentWidget( void )
	{
		return( m_trackContentWidget );
	}

	bool isMovingTrack( void ) const
	{
		return( m_action == MOVE_TRACK );
	}
	
	virtual void repaint( void );


public slots:
	void changePosition( const midiTime & _new_pos = -1 );


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );

	midiTime FASTCALL endPosition( const midiTime & _pos_start );


private:
	enum actions
	{
		NONE, MOVE_TRACK, RESIZE_TRACK
	} ;
	track * m_track;

	trackOperationsWidget m_trackOperationsWidget;
	trackSettingsWidget m_trackSettingsWidget;
	trackContentWidget m_trackContentWidget;

	actions m_action;

} ;




// base-class for all tracks
class track : public settings, public engineObject
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

	static track * FASTCALL create( trackTypes _tt, trackContainer * _tc );
	static track * FASTCALL create( const QDomElement & _this,
							trackContainer * _tc );
	static track * FASTCALL clone( track * _track );

	tact length( void ) const;

	inline bool muted( void ) const
	{
		return( m_trackWidget->getTrackOperationsWidget().muted() );
	}

	inline void setMuted( bool _muted )
	{
		m_trackWidget->getTrackOperationsWidget().setMuted( _muted );
	}


	// pure virtual functions
	virtual trackTypes type( void ) const = 0;

	virtual bool FASTCALL play( const midiTime & _start,
						const f_cnt_t _start_frame,
						const fpab_t _frames,
						const f_cnt_t _frame_base,
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
