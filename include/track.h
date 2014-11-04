/*
 * track.h - declaration of classes concerning tracks -> necessary for all
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
#include <QtGui/QWidget>
#include <QColor>

#include "lmms_basics.h"
#include "MidiTime.h"
#include "rubberband.h"
#include "JournallingObject.h"
#include "AutomatableModel.h"
#include "ModelView.h"


class QMenu;
class QPushButton;

class pixmapButton;
class textFloat;
class track;
class trackContentObjectView;
class TrackContainer;
class TrackContainerView;
class trackContentWidget;
class trackView;

typedef QWidget trackSettingsWidget;

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


class trackContentObject : public Model, public JournallingObject
{
	Q_OBJECT
	mapPropertyFromModel(bool,isMuted,setMuted,m_mutedModel);
	mapPropertyFromModel(bool,isSolo,setSolo,m_soloModel);
public:
	trackContentObject( track * _track );
	virtual ~trackContentObject();

	inline track * getTrack() const
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
		return sp + m_length;
	}

	inline const MidiTime & length() const
	{
		return m_length;
	}

	virtual void movePosition( const MidiTime & _pos );
	virtual void changeLength( const MidiTime & _length );

	virtual trackContentObjectView * createView( trackView * _tv ) = 0;


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

	track * m_track;
	QString m_name;

	MidiTime m_startPosition;
	MidiTime m_length;

	BoolModel m_mutedModel;
	BoolModel m_soloModel;


	friend class trackContentObjectView;

} ;



class trackContentObjectView : public selectableObject, public ModelView
{
	Q_OBJECT

// theming qproperties
	Q_PROPERTY( QColor fgColor READ fgColor WRITE setFgColor )
	Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )

public:
	trackContentObjectView( trackContentObject * _tco, trackView * _tv );
	virtual ~trackContentObjectView();

	bool fixedTCOs();

	inline trackContentObject * getTrackContentObject()
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

	inline trackView * getTrackView()
	{
		return m_trackView;
	}


protected slots:
	void updateLength();
	void updatePosition();


private:
	enum Actions
	{
		NoAction,
		Move,
		MoveSelection,
		Resize
	} ;

	static textFloat * s_textFloat;

	trackContentObject * m_tco;
	trackView * m_trackView;
	Actions m_action;
	bool m_autoResize;
	int m_initialMouseX;

	textFloat * m_hint;

	MidiTime m_oldTime;// used for undo/redo while mouse-button is pressed

// qproperty fields
	QColor m_fgColor;
	QColor m_textColor;
} ;





class trackContentWidget : public QWidget, public JournallingObject
{
	Q_OBJECT

	// qproperties for track background gradients
	Q_PROPERTY( QBrush darkerColor READ darkerColor WRITE setDarkerColor )
	Q_PROPERTY( QBrush lighterColor READ lighterColor WRITE setLighterColor )

public:
	trackContentWidget( trackView * _parent );
	virtual ~trackContentWidget();

	/*! \brief Updates the background tile pixmap. */
	void updateBackground();

	void addTCOView( trackContentObjectView * _tcov );
	void removeTCOView( trackContentObjectView * _tcov );
	void removeTCOView( int _tco_num )
	{
		if( _tco_num >= 0 && _tco_num < m_tcoViews.size() )
		{
			removeTCOView( m_tcoViews[_tco_num] );
		}
	}

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
	track * getTrack();
	MidiTime getPosition( int _mouse_x );

	trackView * m_trackView;

	typedef QVector<trackContentObjectView *> tcoViewVector;
	tcoViewVector m_tcoViews;

	QPixmap m_background;

	// qproperty fields
	QBrush m_darkerColor;
	QBrush m_lighterColor;
} ;





class trackOperationsWidget : public QWidget
{
	Q_OBJECT
public:
	trackOperationsWidget( trackView * _parent );
	~trackOperationsWidget();


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

	trackView * m_trackView;

	QPushButton * m_trackOps;
	pixmapButton * m_muteBtn;
	pixmapButton * m_soloBtn;


	friend class trackView;

signals:
	void trackRemovalScheduled( trackView * _t );

} ;





// base-class for all tracks
class EXPORT track : public Model, public JournallingObject
{
	Q_OBJECT
	mapPropertyFromModel(bool,isMuted,setMuted,m_mutedModel);
	mapPropertyFromModel(bool,isSolo,setSolo,m_soloModel);
public:
	typedef QVector<trackContentObject *> tcoVector;

	enum TrackTypes
	{
		InstrumentTrack,
		BBTrack,
		SampleTrack,
		EventTrack,
		VideoTrack,
		AutomationTrack,
		HiddenAutomationTrack,
		NumTrackTypes
	} ;

	track( TrackTypes _type, TrackContainer * _tc );
	virtual ~track();

	static track * create( TrackTypes _tt, TrackContainer * _tc );
	static track * create( const QDomElement & _this,
							TrackContainer * _tc );
	void clone();


	// pure virtual functions
	TrackTypes type() const
	{
		return m_type;
	}

	virtual bool play( const MidiTime & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 ) = 0;


	virtual trackView * createView( TrackContainerView * _view ) = 0;
	virtual trackContentObject * createTCO( const MidiTime & _pos ) = 0;

	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
						QDomElement & _parent ) = 0;
	virtual void loadTrackSpecificSettings( const QDomElement & _this ) = 0;


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );

	void setSimpleSerializing()
	{
		m_simpleSerializingMode = true;
	}

	// -- for usage by trackContentObject only ---------------
	trackContentObject * addTCO( trackContentObject * _tco );
	void removeTCO( trackContentObject * _tco );
	// -------------------------------------------------------
	void deleteTCOs();

	int numOfTCOs();
	trackContentObject * getTCO( int _tco_num );
	int getTCONum( trackContentObject * _tco );

	const tcoVector & getTCOs() const
	{
		return( m_trackContentObjects );
	}
	void getTCOsInRange( tcoVector & _tco_v, const MidiTime & _start,
							const MidiTime & _end );
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


	friend class trackView;


signals:
	void destroyedTrack();
	void nameChanged();
	void trackContentObjectAdded( trackContentObject * );

} ;




class trackView : public QWidget, public ModelView, public JournallingObject
{
	Q_OBJECT
public:
	trackView( track * _track, TrackContainerView* tcv );
	virtual ~trackView();

	inline const track * getTrack() const
	{
		return( m_track );
	}

	inline track * getTrack()
	{
		return( m_track );
	}

	inline TrackContainerView* trackContainerView()
	{
		return m_trackContainerView;
	}

	inline trackOperationsWidget * getTrackOperationsWidget()
	{
		return( &m_trackOperationsWidget );
	}

	inline trackSettingsWidget * getTrackSettingsWidget()
	{
		return( &m_trackSettingsWidget );
	}

	inline trackContentWidget * getTrackContentWidget()
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

	track * m_track;
	TrackContainerView * m_trackContainerView;

	trackOperationsWidget m_trackOperationsWidget;
	trackSettingsWidget m_trackSettingsWidget;
	trackContentWidget m_trackContentWidget;

	Actions m_action;


	friend class trackLabelButton;


private slots:
	void createTCOView( trackContentObject * _tco );

} ;



#endif
