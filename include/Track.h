/*
 * Track.h - declaration of classes concerning tracks -> necessary for all
 *           track-like objects (beat/bassline, sample-track...)
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

#ifndef TRACK_H
#define TRACK_H

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QWidget>
#include <QColor>
#include <QMimeData>

#include "lmms_basics.h"
#include "MidiTime.h"
#include "Rubberband.h"
#include "JournallingObject.h"
#include "AutomatableModel.h"
#include "ModelView.h"
#include "DataFile.h"
#include "ProcessHandle.h"

class QMenu;
class QPushButton;

class PixmapButton;
class TextFloat;
class Track;
class TrackContentObjectView;
class TrackContainer;
class TrackContainerView;
class TrackContentWidget;
class TrackView;

typedef QWidget trackSettingsWidget;
typedef QVector<Track *> TrackList;

const int DEFAULT_SETTINGS_WIDGET_WIDTH = 224;
const int TRACK_OP_WIDTH = 78;
// This shaves 150-ish pixels off track buttons,
// ruled from config: ui.compacttrackbuttons
const int DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT = 96;
const int TRACK_OP_WIDTH_COMPACT = 60;

/*! The minimum track height in pixels
 *
 * Tracks can be resized by shift-dragging anywhere inside the track
 * display.  This sets the minimum size in pixels for a track.
 */
const int MINIMAL_TRACK_HEIGHT = 8;
const int DEFAULT_TRACK_HEIGHT = 32;

const int TCO_BORDER_WIDTH = 2;


class TrackContentObject : public Model, public JournallingObject
{
	Q_OBJECT
	MM_OPERATORS
	mapPropertyFromModel(bool,isMuted,setMuted,m_mutedModel);
	mapPropertyFromModel(bool,isSolo,setSolo,m_soloModel);
public:
	TrackContentObject( Track * _track, const MidiTime & position );
	virtual ~TrackContentObject();

	inline Track * getTrack() const
	{
		return m_track;
	}

	inline const QString & name() const
	{
		return m_name;
	}

	inline void setName( const QString & _name )
	{
		m_name = _name;
		emit dataChanged();
	}

	virtual QString displayName() const
	{
		return name();
	}


	inline const MidiTime & startPosition() const
	{
		return m_startPosition;
	}

	inline MidiTime endPosition() const
	{
		const int sp = m_startPosition;
		return sp + m_length - 1;
	}

	inline const MidiTime & length() const
	{
		return m_length;
	}
	
	virtual void movePosition( const MidiTime & _pos );
	virtual void changeLength( const MidiTime & _length );

	virtual TrackContentObjectView * createView( TrackView * _tv ) = 0;

	inline void selectViewOnCreate( bool select )
	{
		m_selectViewOnCreate = select;
	}

	inline bool getSelectViewOnCreate()
	{
		return m_selectViewOnCreate;
	}


public slots:
	void copy();
	void paste();
	void toggleMute();


signals:
	void lengthChanged();
	void positionChanged();
	void destroyedTCO();


private:
	enum Actions
	{
		NoAction,
		Move,
		Resize
	} ;

	Track * m_track;
	QString m_name;

	MidiTime m_startPosition;
	MidiTime m_length;

	BoolModel m_mutedModel;
	BoolModel m_soloModel;

	bool m_selectViewOnCreate;

	friend class TrackContentObjectView;

} ;



class TrackContentObjectView : public selectableObject, public ModelView
{
	Q_OBJECT

// theming qproperties
	Q_PROPERTY( QColor fgColor READ fgColor WRITE setFgColor )
	Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )

public:
	TrackContentObjectView( TrackContentObject * _tco, TrackView * _tv );
	virtual ~TrackContentObjectView();

	bool fixedTCOs();

	inline TrackContentObject * getTrackContentObject()
	{
		return m_tco;
	}
// qproperty access func
	QColor fgColor() const;
	QColor textColor() const;
	void setFgColor( const QColor & _c );
	void setTextColor( const QColor & _c );

public slots:
	virtual bool close();
	void cut();
	void remove();

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

	void setAutoResizeEnabled( bool _e = false );
	float pixelsPerTact();

	inline TrackView * getTrackView()
	{
		return m_trackView;
	}

	DataFile createTCODataFiles(const QVector<TrackContentObjectView *> & tcos) const;


protected slots:
	void updateLength();
	void updatePosition();


private:
	enum Actions
	{
		NoAction,
		Move,
		MoveSelection,
		Resize,
		CopySelection,
		ToggleSelected
	} ;

	static TextFloat * s_textFloat;

	TrackContentObject * m_tco;
	TrackView * m_trackView;
	Actions m_action;
	bool m_autoResize;
	QPoint m_initialMousePos;
	QPoint m_initialMouseGlobalPos;

	TextFloat * m_hint;

	MidiTime m_oldTime;// used for undo/redo while mouse-button is pressed

// qproperty fields
	QColor m_fgColor;
	QColor m_textColor;

	inline void setInitialMousePos( QPoint pos )
	{
		m_initialMousePos = pos;
		m_initialMouseGlobalPos = mapToGlobal( pos );
	}

	bool mouseMovedDistance( QMouseEvent * _me, int distance );

} ;





class TrackContentWidget : public QWidget, public JournallingObject
{
	Q_OBJECT

	// qproperties for track background gradients
	Q_PROPERTY( QBrush darkerColor READ darkerColor WRITE setDarkerColor )
	Q_PROPERTY( QBrush lighterColor READ lighterColor WRITE setLighterColor )

public:
	TrackContentWidget( TrackView * _parent );
	virtual ~TrackContentWidget();

	/*! \brief Updates the background tile pixmap. */
	void updateBackground();

	void addTCOView( TrackContentObjectView * _tcov );
	void removeTCOView( TrackContentObjectView * _tcov );
	void removeTCOView( int _tco_num )
	{
		if( _tco_num >= 0 && _tco_num < m_tcoViews.size() )
		{
			removeTCOView( m_tcoViews[_tco_num] );
		}
	}

	bool canPasteSelection( MidiTime tcoPos, const QMimeData * mimeData );
	bool pasteSelection( MidiTime tcoPos, QDropEvent * _de );

	MidiTime endPosition( const MidiTime & _pos_start );

	// qproperty access methods

	QBrush darkerColor() const;
	QBrush lighterColor() const;

	void setDarkerColor( const QBrush & _c );
	void setLighterColor( const QBrush & _c );

public slots:
	void update();
	void changePosition( const MidiTime & _new_pos = MidiTime( -1 ) );


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );

	virtual QString nodeName() const
	{
		return "trackcontentwidget";
	}

	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED(element)
	}


private:
	Track * getTrack();
	MidiTime getPosition( int _mouse_x );

	TrackView * m_trackView;

	typedef QVector<TrackContentObjectView *> tcoViewVector;
	tcoViewVector m_tcoViews;

	QPixmap m_background;

	// qproperty fields
	QBrush m_darkerColor;
	QBrush m_lighterColor;
} ;





class TrackOperationsWidget : public QWidget
{
	Q_OBJECT
public:
	TrackOperationsWidget( TrackView * _parent );
	~TrackOperationsWidget();


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );


private slots:
	void cloneTrack();
	void removeTrack();
	void updateMenu();
	void recordingOn();
	void recordingOff();
	void clearTrack();

private:
	static QPixmap * s_grip;

	TrackView * m_trackView;

	QPushButton * m_trackOps;
	PixmapButton * m_muteBtn;
	PixmapButton * m_soloBtn;


	friend class TrackView;

signals:
	void trackRemovalScheduled( TrackView * _t );

} ;



typedef QVector<TrackContentObject *> tcoVector;

// base-class for all tracks
class EXPORT Track : public Model, public JournallingObject
{
	Q_OBJECT
	MM_OPERATORS
	mapPropertyFromModel(bool,isMuted,setMuted,m_mutedModel);
	mapPropertyFromModel(bool,isSolo,setSolo,m_soloModel);
public:
	typedef QVector<TrackContentObject *> tcoVector;

	enum TrackTypes
	{
		InstrumentTrack,
		BBTrack,
		SampleTrack,
		EventTrack,
		VideoTrack,
		AutomationTrack,
		HiddenAutomationTrack,
		TempoTrack,
		CommentTrack,
		NumTrackTypes
	} ;

	Track( TrackTypes _type, TrackContainer * _tc );
	virtual ~Track();

	static Track * create( TrackTypes _tt, TrackContainer * _tc );
	static Track * create( const QDomElement & _this,
							TrackContainer * _tc );
	void clone();


	// pure virtual functions
	TrackTypes type() const
	{
		return m_type;
	}

	virtual ProcessHandle * getProcessHandle() = 0;

	virtual bool play( const MidiTime & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 ) = 0;


	virtual TrackView * createView( TrackContainerView * _view ) = 0;
	virtual TrackContentObject * createTCO( const MidiTime & _pos ) = 0;

	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
						QDomElement & _parent ) = 0;
	virtual void loadTrackSpecificSettings( const QDomElement & _this ) = 0;


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );

	void setSimpleSerializing()
	{
		m_simpleSerializingMode = true;
	}

	// -- for usage by TrackContentObject only ---------------
	TrackContentObject * addTCO( TrackContentObject * _tco );
	void removeTCO( TrackContentObject * _tco );
	// -------------------------------------------------------
	void deleteTCOs();

	int numOfTCOs();
	TrackContentObject * getTCO( int _tco_num );
	int getTCONum(const TrackContentObject* _tco );

	const tcoVector & getTCOs() const
	{
		return( m_trackContentObjects );
	}
	void getTCOsInRange( tcoVector & _tco_v, const MidiTime & _start,
							const MidiTime & _end );
	bool hasTCOsInRange( const MidiTime & start, const MidiTime & end, TrackContentObject * otherThan = NULL );
	void swapPositionOfTCOs( int _tco_num1, int _tco_num2 );


	void insertTact( const MidiTime & _pos );
	void removeTact( const MidiTime & _pos );

	tact_t length() const;


	inline TrackContainer* trackContainer() const
	{
		return m_trackContainer;
	}

	// name-stuff
	virtual const QString & name() const
	{
		return( m_name );
	}

	virtual QString displayName() const
	{
		return( name() );
	}

	using Model::dataChanged;

	inline int getHeight() {
	  return ( m_height >= MINIMAL_TRACK_HEIGHT ? m_height : DEFAULT_TRACK_HEIGHT );
	}
	inline void setHeight( int _height ) {
	  m_height = _height;
	}

	void lock()
	{
		m_processingLock.lock();
	}
	void unlock()
	{
		m_processingLock.unlock();
	}
	bool tryLock()
	{
		return m_processingLock.tryLock();
	}
	
	bool allowsOverlap() const
	{
		return m_allowsOverlap;
	}
	
	void setAllowsOverlap( bool b )
	{
		m_allowsOverlap = b;
	}

public slots:
	virtual void setName( const QString & _new_name )
	{
		m_name = _new_name;
		emit nameChanged();
	}

	void toggleSolo();


private:
	TrackContainer* m_trackContainer;
	TrackTypes m_type;
	QString m_name;
	int m_height;

	BoolModel m_mutedModel;
	BoolModel m_soloModel;
	bool m_mutedBeforeSolo;

	bool m_simpleSerializingMode;

	tcoVector m_trackContentObjects;

	QMutex m_processingLock;

	bool m_allowsOverlap;

	friend class TrackView;


signals:
	void destroyedTrack();
	void nameChanged();
	void trackContentObjectAdded( TrackContentObject * );

} ;




class TrackView : public QWidget, public ModelView, public JournallingObject
{
	Q_OBJECT
public:
	TrackView( Track * _track, TrackContainerView* tcv );
	virtual ~TrackView();

	inline const Track * getTrack() const
	{
		return( m_track );
	}

	inline Track * getTrack()
	{
		return( m_track );
	}

	inline TrackContainerView* trackContainerView()
	{
		return m_trackContainerView;
	}

	inline TrackOperationsWidget * getTrackOperationsWidget()
	{
		return( &m_trackOperationsWidget );
	}

	inline trackSettingsWidget * getTrackSettingsWidget()
	{
		return( &m_trackSettingsWidget );
	}

	inline TrackContentWidget * getTrackContentWidget()
	{
		return( &m_trackContentWidget );
	}

	bool isMovingTrack() const
	{
		return( m_action == MoveTrack );
	}

	virtual void update();


public slots:
	virtual bool close();


protected:
	virtual void modelChanged();

	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED(element)
	}

	virtual QString nodeName() const
	{
		return "trackview";
	}


	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );


private:
	enum Actions
	{
		NoAction,
		MoveTrack,
		ResizeTrack
	} ;

	Track * m_track;
	TrackContainerView * m_trackContainerView;

	TrackOperationsWidget m_trackOperationsWidget;
	trackSettingsWidget m_trackSettingsWidget;
	TrackContentWidget m_trackContentWidget;

	Actions m_action;


	friend class TrackLabelButton;


private slots:
	void createTCOView( TrackContentObject * _tco );

} ;



#endif
