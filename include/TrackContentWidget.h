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

#ifndef LMMS_GUI_TRACK_CONTENT_WIDGET_H
#define LMMS_GUI_TRACK_CONTENT_WIDGET_H

#include <QWidget>

#include "JournallingObject.h"
#include "TimePos.h"

class QMimeData;  // IWYU pragma: keep


namespace lmms
{

class Track;

namespace gui
{

class ClipView;  // IWYU pragma: keep
class TrackView;

class TrackContentWidget : public QWidget, public JournallingObject
{
	Q_OBJECT

	// qproperties for track background gradients
	Q_PROPERTY(QBrush darkerColor READ darkerColor WRITE setDarkerColor)
	Q_PROPERTY(QBrush lighterColor READ lighterColor WRITE setLighterColor)
	Q_PROPERTY(QBrush coarseGridColor READ coarseGridColor WRITE setCoarseGridColor)
	Q_PROPERTY(QBrush fineGridColor READ fineGridColor WRITE setFineGridColor)
	Q_PROPERTY(QBrush horizontalColor READ horizontalColor WRITE setHorizontalColor)
	Q_PROPERTY(QBrush embossColor READ embossColor WRITE setEmbossColor)

	Q_PROPERTY(int coarseGridWidth READ coarseGridWidth WRITE setCoarseGridWidth)
	Q_PROPERTY(int fineGridWidth READ fineGridWidth WRITE setFineGridWidth)
	Q_PROPERTY(int horizontalWidth READ horizontalWidth WRITE setHorizontalWidth)
	Q_PROPERTY(int embossWidth READ embossWidth WRITE setEmbossWidth)

	Q_PROPERTY(int embossOffset READ embossOffset WRITE setEmbossOffset)

public:
	TrackContentWidget( TrackView * parent );
	~TrackContentWidget() override = default;

	void addClipView( ClipView * clipv );
	void removeClipView( ClipView * clipv );
	void removeClipView( int clipNum )
	{
		if( clipNum >= 0 && clipNum < m_clipViews.size() )
		{
			removeClipView( m_clipViews[clipNum] );
		}
	}

	bool canPasteSelection( TimePos clipPos, const QDropEvent *de );
	bool canPasteSelection( TimePos clipPos, const QMimeData *md, bool allowSameBar = false );
	bool pasteSelection( TimePos clipPos, QDropEvent * de );
	bool pasteSelection( TimePos clipPos, const QMimeData * md, bool skipSafetyCheck = false );

	TimePos endPosition( const TimePos & posStart );

	// qproperty access methods

	QBrush darkerColor() const;
	QBrush lighterColor() const;
	QBrush coarseGridColor() const;
	QBrush fineGridColor() const;
	QBrush horizontalColor() const;
	QBrush embossColor() const;

	int coarseGridWidth() const;
	int fineGridWidth() const;
	int horizontalWidth() const;
	int embossWidth() const;

	int embossOffset() const;

	void setDarkerColor(const QBrush & c);
	void setLighterColor(const QBrush & c);
	void setCoarseGridColor(const QBrush & c);
	void setFineGridColor(const QBrush & c);
	void setHorizontalColor(const QBrush & c);
	void setEmbossColor(const QBrush & c);

	void setCoarseGridWidth(int c);
	void setFineGridWidth(int c);
	void setHorizontalWidth(int c);
	void setEmbossWidth(int c);

	void setEmbossOffset(int c);

public slots:
	void update();
	void changePosition( const lmms::TimePos & newPos = TimePos( -1 ) );
	/*! \brief Updates the background tile pixmap. */
	void updateBackground();

protected:
	enum class ContextMenuAction
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

	using clipViewVector = QVector<ClipView*>;
	clipViewVector m_clipViews;

	QPixmap m_background;

	// qproperty fields
	QBrush m_darkerColor;
	QBrush m_lighterColor;
	QBrush m_coarseGridColor;
	QBrush m_fineGridColor;
	QBrush m_horizontalColor;
	QBrush m_embossColor;

	int m_coarseGridWidth;
	int m_fineGridWidth;
	int m_horizontalWidth;
	int m_embossWidth;

	int m_embossOffset;
} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_TRACK_CONTENT_WIDGET_H
