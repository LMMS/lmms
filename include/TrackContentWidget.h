/*
 * TrackContentWidget.h - declaration of TrackContentWidget class
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

#ifndef TRACK_CONTENT_WIDGET_H
#define TRACK_CONTENT_WIDGET_H

#include <QWidget>

#include "JournallingObject.h"
#include "TimePos.h"


class QMimeData;

class Track;
class TrackContentObjectView;
class TrackView;


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
	void mouseReleaseEvent( QMouseEvent * me ) override;
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



#endif
