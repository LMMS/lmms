/*
 * Track.h - declaration of classes concerning tracks -> necessary for all
 *           track-like objects (beat/bassline, sample-track...)
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

#ifndef TRACK_H
#define TRACK_H

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QWidget>
#include <QSize>
#include <QColor>
#include <QMimeData>

#include "lmms_basics.h"
#include "TimePos.h"
#include "Rubberband.h"
#include "JournallingObject.h"
#include "AutomatableModel.h"
#include "ModelView.h"
#include "DataFile.h"
#include "FadeButton.h"


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


const int DEFAULT_SETTINGS_WIDGET_WIDTH = 224;
const int TRACK_OP_WIDTH = 78;
// This shaves 150-ish pixels off track buttons,
// ruled from config: ui.compacttrackbuttons
const int DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT = 96;
const int TRACK_OP_WIDTH_COMPACT = 62;

/*! The minimum track height in pixels
 *
 * Tracks can be resized by shift-dragging anywhere inside the track
 * display.  This sets the minimum size in pixels for a track.
 */
const int MINIMAL_TRACK_HEIGHT = 32;
const int DEFAULT_TRACK_HEIGHT = 32;

const int TCO_BORDER_WIDTH = 2;

char const *const FILENAME_FILTER = "[\\0000-\x1f\"*/:<>?\\\\|\x7f]";


class LMMS_EXPORT TrackContentObject : public Model, public JournallingObject
{
	Q_OBJECT
	MM_OPERATORS
	mapPropertyFromModel(bool,isMuted,setMuted,m_mutedModel);
	mapPropertyFromModel(bool,isSolo,setSolo,m_soloModel);
public:
	TrackContentObject( Track * track );
	virtual ~TrackContentObject();

	inline Track * getTrack() const
	{
		return m_track;
	}

	inline const QString & name() const
	{
		return m_name;
	}

	inline void setName( const QString & name )
	{
		m_name = name;
		emit dataChanged();
	}

	QString displayName() const override
	{
		return name();
	}


	inline const TimePos & startPosition() const
	{
		return m_startPosition;
	}

	inline TimePos endPosition() const
	{
		const int sp = m_startPosition;
		return sp + m_length;
	}

	inline const TimePos & length() const
	{
		return m_length;
	}

	inline void setAutoResize( const bool r )
	{
		m_autoResize = r;
	}

	inline const bool getAutoResize() const
	{
		return m_autoResize;
	}
	
	QColor color() const
	{
		return m_color;
	}

	void setColor( const QColor & c )
	{
		m_color = c;
	}
	
	bool hasColor();
	
	void useCustomClipColor( bool b );
	
	bool usesCustomClipColor()
	{
		return m_useCustomClipColor;
	}

	virtual void movePosition( const TimePos & pos );
	virtual void changeLength( const TimePos & length );

	virtual TrackContentObjectView * createView( TrackView * tv ) = 0;

	inline void selectViewOnCreate( bool select )
	{
		m_selectViewOnCreate = select;
	}

	inline bool getSelectViewOnCreate()
	{
		return m_selectViewOnCreate;
	}

	/// Returns true if and only if a->startPosition() < b->startPosition()
	static bool comparePosition(const TrackContentObject* a, const TrackContentObject* b);

	TimePos startTimeOffset() const;
	void setStartTimeOffset( const TimePos &startTimeOffset );
	
	void updateColor();

	// Will copy the state of a TCO to another TCO
	static void copyStateTo( TrackContentObject *src, TrackContentObject *dst );

public slots:
	void toggleMute();


signals:
	void lengthChanged();
	void positionChanged();
	void destroyedTCO();
	void trackColorChanged();


private:
	enum Actions
	{
		NoAction,
		Move,
		Resize
	} ;

	Track * m_track;
	QString m_name;

	TimePos m_startPosition;
	TimePos m_length;
	TimePos m_startTimeOffset;

	BoolModel m_mutedModel;
	BoolModel m_soloModel;
	bool m_autoResize;

	bool m_selectViewOnCreate;

	QColor m_color;
	bool m_useCustomClipColor;

	friend class TrackContentObjectView;

} ;



class TrackContentObjectView : public selectableObject, public ModelView
{
	Q_OBJECT

// theming qproperties
	Q_PROPERTY( QColor mutedColor READ mutedColor WRITE setMutedColor )
	Q_PROPERTY( QColor mutedBackgroundColor READ mutedBackgroundColor WRITE setMutedBackgroundColor )
	Q_PROPERTY( QColor selectedColor READ selectedColor WRITE setSelectedColor )
	Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )
	Q_PROPERTY( QColor textBackgroundColor READ textBackgroundColor WRITE setTextBackgroundColor )
	Q_PROPERTY( QColor textShadowColor READ textShadowColor WRITE setTextShadowColor )
	Q_PROPERTY( QColor BBPatternBackground READ BBPatternBackground WRITE setBBPatternBackground )
	Q_PROPERTY( bool gradient READ gradient WRITE setGradient )
	// We have to use a QSize here because using QPoint isn't supported.
	// width -> x, height -> y
	Q_PROPERTY( QSize mouseHotspotHand WRITE setMouseHotspotHand )

public:
	TrackContentObjectView( TrackContentObject * tco, TrackView * tv );
	virtual ~TrackContentObjectView();

	bool fixedTCOs();

	inline TrackContentObject * getTrackContentObject()
	{
		return m_tco;
	}

	inline TrackView * getTrackView()
	{
		return m_trackView;
	}

	// qproperty access func
	QColor mutedColor() const;
	QColor mutedBackgroundColor() const;
	QColor selectedColor() const;
	QColor textColor() const;
	QColor textBackgroundColor() const;
	QColor textShadowColor() const;
	QColor BBPatternBackground() const;
	bool gradient() const;
	void setMutedColor( const QColor & c );
	void setMutedBackgroundColor( const QColor & c );
	void setSelectedColor( const QColor & c );
	void setTextColor( const QColor & c );
	void setTextBackgroundColor( const QColor & c );
	void setTextShadowColor( const QColor & c );
	void setBBPatternBackground( const QColor & c );
	void setGradient( const bool & b );
	void setMouseHotspotHand(const QSize & s);

	// access needsUpdate member variable
	bool needsUpdate();
	void setNeedsUpdate( bool b );

	// Method to get a QVector of TCOs to be affected by a context menu action
	QVector<TrackContentObjectView *> getClickedTCOs();

	// Methods to remove, copy, cut, paste and mute a QVector of TCO views
	void copy( QVector<TrackContentObjectView *> tcovs );
	void cut( QVector<TrackContentObjectView *> tcovs );
	void paste();
	// remove and toggleMute are static because they don't depend
	// being called from a particular TCO view, but can be called anywhere as long
	// as a valid TCO view list is given, while copy/cut require an instance for
	// some metadata to be written to the clipboard.
	static void remove( QVector<TrackContentObjectView *> tcovs );
	static void toggleMute( QVector<TrackContentObjectView *> tcovs );
	
	QColor getColorForDisplay( QColor );

public slots:
	virtual bool close();
	void remove();
	void update() override;
	
	void changeClipColor();
	void useTrackColor();

protected:
	enum ContextMenuAction
	{
		Remove,
		Cut,
		Copy,
		Paste,
		Mute
	};

	virtual void constructContextMenu( QMenu * )
	{
	}

	void contextMenuEvent( QContextMenuEvent * cme ) override;
	void contextMenuAction( ContextMenuAction action );
	void dragEnterEvent( QDragEnterEvent * dee ) override;
	void dropEvent( QDropEvent * de ) override;
	void leaveEvent( QEvent * e ) override;
	void mousePressEvent( QMouseEvent * me ) override;
	void mouseMoveEvent( QMouseEvent * me ) override;
	void mouseReleaseEvent( QMouseEvent * me ) override;
	void resizeEvent( QResizeEvent * re ) override
	{
		m_needsUpdate = true;
		selectableObject::resizeEvent( re );
	}

	float pixelsPerBar();


	DataFile createTCODataFiles(const QVector<TrackContentObjectView *> & tcos) const;

	virtual void paintTextLabel(QString const & text, QPainter & painter);


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
		ResizeLeft,
		CopySelection,
		ToggleSelected
	} ;

	static TextFloat * s_textFloat;

	TrackContentObject * m_tco;
	TrackView * m_trackView;
	Actions m_action;
	QPoint m_initialMousePos;
	QPoint m_initialMouseGlobalPos;
	TimePos m_initialTCOPos;
	TimePos m_initialTCOEnd;
	QVector<TimePos> m_initialOffsets;

	TextFloat * m_hint;

// qproperty fields
	QColor m_mutedColor;
	QColor m_mutedBackgroundColor;
	QColor m_selectedColor;
	QColor m_textColor;
	QColor m_textBackgroundColor;
	QColor m_textShadowColor;
	QColor m_BBPatternBackground;
	bool m_gradient;
	QSize m_mouseHotspotHand; // QSize must be used because QPoint isn't supported by property system
	bool m_cursorSetYet;

	bool m_needsUpdate;
	inline void setInitialPos( QPoint pos )
	{
		m_initialMousePos = pos;
		m_initialMouseGlobalPos = mapToGlobal( pos );
		m_initialTCOPos = m_tco->startPosition();
		m_initialTCOEnd = m_initialTCOPos + m_tco->length();
	}
	void setInitialOffsets();

	bool mouseMovedDistance( QMouseEvent * me, int distance );
	TimePos draggedTCOPos( QMouseEvent * me );
} ;





class TrackContentWidget : public QWidget, public JournallingObject
{
	Q_OBJECT

	// qproperties for track background gradients
	Q_PROPERTY( QBrush darkerColor READ darkerColor WRITE setDarkerColor )
	Q_PROPERTY( QBrush lighterColor READ lighterColor WRITE setLighterColor )
	Q_PROPERTY( QBrush gridColor READ gridColor WRITE setGridColor )
	Q_PROPERTY( QBrush embossColor READ embossColor WRITE setEmbossColor )

public:
	TrackContentWidget( TrackView * parent );
	virtual ~TrackContentWidget();

	/*! \brief Updates the background tile pixmap. */
	void updateBackground();

	void addTCOView( TrackContentObjectView * tcov );
	void removeTCOView( TrackContentObjectView * tcov );
	void removeTCOView( int tcoNum )
	{
		if( tcoNum >= 0 && tcoNum < m_tcoViews.size() )
		{
			removeTCOView( m_tcoViews[tcoNum] );
		}
	}

	bool canPasteSelection( TimePos tcoPos, const QDropEvent *de );
	bool canPasteSelection( TimePos tcoPos, const QMimeData *md, bool allowSameBar = false );
	bool pasteSelection( TimePos tcoPos, QDropEvent * de );
	bool pasteSelection( TimePos tcoPos, const QMimeData * md, bool skipSafetyCheck = false );

	TimePos endPosition( const TimePos & posStart );

	// qproperty access methods

	QBrush darkerColor() const;
	QBrush lighterColor() const;
	QBrush gridColor() const;
	QBrush embossColor() const;

	void setDarkerColor( const QBrush & c );
	void setLighterColor( const QBrush & c );
	void setGridColor( const QBrush & c );
	void setEmbossColor( const QBrush & c);

public slots:
	void update();
	void changePosition( const TimePos & newPos = TimePos( -1 ) );

protected:
	enum ContextMenuAction
	{
		Paste
	};

	void contextMenuEvent( QContextMenuEvent * cme ) override;
	void contextMenuAction( QContextMenuEvent * cme, ContextMenuAction action );
	void dragEnterEvent( QDragEnterEvent * dee ) override;
	void dropEvent( QDropEvent * de ) override;
	void mousePressEvent( QMouseEvent * me ) override;
	void paintEvent( QPaintEvent * pe ) override;
	void resizeEvent( QResizeEvent * re ) override;

	QString nodeName() const override
	{
		return "trackcontentwidget";
	}

	void saveSettings( QDomDocument& doc, QDomElement& element ) override
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	void loadSettings( const QDomElement& element ) override
	{
		Q_UNUSED(element)
	}


private:
	Track * getTrack();
	TimePos getPosition( int mouseX );

	TrackView * m_trackView;

	typedef QVector<TrackContentObjectView *> tcoViewVector;
	tcoViewVector m_tcoViews;

	QPixmap m_background;

	// qproperty fields
	QBrush m_darkerColor;
	QBrush m_lighterColor;
	QBrush m_gridColor;
	QBrush m_embossColor;
} ;





class TrackOperationsWidget : public QWidget
{
	Q_OBJECT
public:
	TrackOperationsWidget( TrackView * parent );
	~TrackOperationsWidget();


protected:
	void mousePressEvent( QMouseEvent * me ) override;
	void paintEvent( QPaintEvent * pe ) override;


private slots:
	void cloneTrack();
	void removeTrack();
	void updateMenu();
	void changeTrackColor();
	void randomTrackColor();
	void resetTrackColor();
	void useTrackColor();
	void toggleRecording(bool on);
	void recordingOn();
	void recordingOff();
	void clearTrack();

private:
	TrackView * m_trackView;

	QPushButton * m_trackOps;
	PixmapButton * m_muteBtn;
	PixmapButton * m_soloBtn;


	friend class TrackView;

signals:
	void trackRemovalScheduled( TrackView * t );
	void colorChanged( QColor & c );
	void colorParented();
	void colorReset();

} ;





// base-class for all tracks
class LMMS_EXPORT Track : public Model, public JournallingObject
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
		NumTrackTypes
	} ;

	Track( TrackTypes type, TrackContainer * tc );
	virtual ~Track();

	static Track * create( TrackTypes tt, TrackContainer * tc );
	static Track * create( const QDomElement & element,
							TrackContainer * tc );
	Track * clone();


	// pure virtual functions
	TrackTypes type() const
	{
		return m_type;
	}

	virtual bool play( const TimePos & start, const fpp_t frames,
						const f_cnt_t frameBase, int tcoNum = -1 ) = 0;


	virtual TrackView * createView( TrackContainerView * view ) = 0;
	virtual TrackContentObject * createTCO( const TimePos & pos ) = 0;

	virtual void saveTrackSpecificSettings( QDomDocument & doc,
						QDomElement & parent ) = 0;
	virtual void loadTrackSpecificSettings( const QDomElement & element ) = 0;


	void saveSettings( QDomDocument & doc, QDomElement & element ) override;
	void loadSettings( const QDomElement & element ) override;

	void setSimpleSerializing()
	{
		m_simpleSerializingMode = true;
	}

	// -- for usage by TrackContentObject only ---------------
	TrackContentObject * addTCO( TrackContentObject * tco );
	void removeTCO( TrackContentObject * tco );
	// -------------------------------------------------------
	void deleteTCOs();

	int numOfTCOs();
	TrackContentObject * getTCO( int tcoNum );
	int getTCONum(const TrackContentObject* tco );

	const tcoVector & getTCOs() const
	{
		return m_trackContentObjects;
	}
	void getTCOsInRange( tcoVector & tcoV, const TimePos & start,
							const TimePos & end );
	void swapPositionOfTCOs( int tcoNum1, int tcoNum2 );

	void createTCOsForBB( int bb );


	void insertBar( const TimePos & pos );
	void removeBar( const TimePos & pos );

	bar_t length() const;


	inline TrackContainer* trackContainer() const
	{
		return m_trackContainer;
	}

	// name-stuff
	virtual const QString & name() const
	{
		return m_name;
	}

	QString displayName() const override
	{
		return name();
	}

	using Model::dataChanged;

	inline int getHeight()
	{
		return m_height >= MINIMAL_TRACK_HEIGHT
			? m_height
			: DEFAULT_TRACK_HEIGHT;
	}
	inline void setHeight( int height )
	{
		m_height = height;
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
	
	QColor color()
	{
		return m_color;
	}
	bool useColor()
	{
		return m_hasColor;
	}
	
	BoolModel* getMutedModel();

public slots:
	virtual void setName( const QString & newName )
	{
		m_name = newName;
		emit nameChanged();
	}

	void toggleSolo();

	void trackColorChanged( QColor & c );
	void trackColorReset();

private:
	TrackContainer* m_trackContainer;
	TrackTypes m_type;
	QString m_name;
	int m_height;

protected:
	BoolModel m_mutedModel;
private:
	BoolModel m_soloModel;
	bool m_mutedBeforeSolo;

	bool m_simpleSerializingMode;

	tcoVector m_trackContentObjects;

	QMutex m_processingLock;
	
	QColor m_color;
	bool m_hasColor;

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
		return m_track;
	}

	inline Track * getTrack()
	{
		return m_track;
	}

	inline TrackContainerView* trackContainerView()
	{
		return m_trackContainerView;
	}

	inline TrackOperationsWidget * getTrackOperationsWidget()
	{
		return &m_trackOperationsWidget;
	}

	inline QWidget * getTrackSettingsWidget()
	{
		return &m_trackSettingsWidget;
	}

	inline TrackContentWidget * getTrackContentWidget()
	{
		return &m_trackContentWidget;
	}

	bool isMovingTrack() const
	{
		return m_action == MoveTrack;
	}

	virtual void update();

	// Create a menu for assigning/creating channels for this track
	// Currently instrument track and sample track supports it
	virtual QMenu * createFxMenu(QString title, QString newFxLabel);


public slots:
	virtual bool close();


protected:
	void modelChanged() override;

	void saveSettings( QDomDocument& doc, QDomElement& element ) override
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	void loadSettings( const QDomElement& element ) override
	{
		Q_UNUSED(element)
	}

	QString nodeName() const override
	{
		return "trackview";
	}


	void dragEnterEvent( QDragEnterEvent * dee ) override;
	void dropEvent( QDropEvent * de ) override;
	void mousePressEvent( QMouseEvent * me ) override;
	void mouseMoveEvent( QMouseEvent * me ) override;
	void mouseReleaseEvent( QMouseEvent * me ) override;
	void paintEvent( QPaintEvent * pe ) override;
	void resizeEvent( QResizeEvent * re ) override;


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
	QWidget m_trackSettingsWidget;
	TrackContentWidget m_trackContentWidget;

	Actions m_action;

	virtual FadeButton * getActivityIndicator()
	{
		return nullptr;
	}

	void setIndicatorMute(FadeButton* indicator, bool muted);

	friend class TrackLabelButton;


private slots:
	void createTCOView( TrackContentObject * tco );
	void muteChanged();

} ;



#endif
