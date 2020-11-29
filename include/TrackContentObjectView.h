/*
 * TrackContentObjectView.h - declaration of TrackContentObjectView class
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

#ifndef TRACK_CONTENT_OBJECT_VIEW_H
#define TRACK_CONTENT_OBJECT_VIEW_H


#include <QtCore/QVector>

#include "ModelView.h"
#include "Rubberband.h"
#include "TrackContentObject.h"


class QMenu;
class QContextMenuEvent;

class DataFile;
class TextFloat;
class TrackContentObject;
class TrackView;


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


#endif
