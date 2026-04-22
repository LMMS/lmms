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
	//! @brief Create a new trackContentWidget
	//!
	//! Creates a new track content widget for the given track.
	//! The content widget comprises the 'grip bar' and the 'tools' button for the track's context menu.
	//!
	//! @param parent The parent track.
	TrackContentWidget(TrackView* parent);

	~TrackContentWidget() override = default;

	//! @brief Adds a ClipView to this widget.
	//!
	//! Adds a(nother) ClipView to our list of views. We also check that our position is up-to-date.
	//!
	//! @param clipv The ClipView to add.
	void addClipView(ClipView* clipv);

	//! @brief Removes the given ClipView from this widget.
	//!
	//! Removes the given ClipView from our list of views.
	//!
	//! @param clipv The ClipView to remove.
	void removeClipView(ClipView* clipv);

	void removeClipView( int clipNum )
	{
		if( clipNum >= 0 && clipNum < m_clipViews.size() )
		{
			removeClipView( m_clipViews[clipNum] );
		}
	}

	//! @brief Returns whether a selection of Clips can be pasted into this
	//! @param clipPos The position of the Clip slot being pasted on
	//! @param de The DropEvent generated
	bool canPasteSelection(TimePos clipPos, const QDropEvent* de);

	//! @overload
	//! Overloaded method to make it possible to call this method without a Drag&Drop event
	bool canPasteSelection(TimePos clipPos, const QMimeData* md, bool allowSameBar = false);

	//! @brief Pastes a selection of Clips onto the track
	//! @param clipPos The position of the Clip slot being pasted on
	//! @param de The DropEvent generated
	bool pasteSelection(TimePos clipPos, QDropEvent* de);

	//! @overload
	//! Overloaded method to make it possible to call this method without a Drag&Drop event
	bool pasteSelection( TimePos clipPos, const QMimeData * md, bool skipSafetyCheck = false );

	//! @brief Return the end position of the trackContentWidget in Bars.
	//! @param posStart The starting position of the Widget (from getPosition())
	TimePos endPosition(const TimePos& posStart);

	QBrush darkerColor() const { return m_darkerColor; }
	QBrush lighterColor() const { return m_lighterColor; }
	QBrush coarseGridColor() const { return m_coarseGridColor; }
	QBrush fineGridColor() const { return m_fineGridColor; }
	QBrush horizontalColor() const { return m_horizontalColor; }
	QBrush embossColor() const { return m_embossColor; }
	int coarseGridWidth() const { return m_coarseGridWidth; }
	int fineGridWidth() const { return m_fineGridWidth; }
	int horizontalWidth() const { return m_horizontalWidth; }
	int embossWidth() const { return m_embossWidth; }
	int embossOffset() const { return m_embossOffset; }

	void setDarkerColor(const QBrush& c) { m_darkerColor = c; }
	void setLighterColor(const QBrush& c) { m_lighterColor = c; }
	void setCoarseGridColor(const QBrush& c) { m_coarseGridColor = c; }
	void setFineGridColor(const QBrush& c) { m_fineGridColor = c; }
	void setHorizontalColor(const QBrush& c) { m_horizontalColor = c; }
	void setEmbossColor(const QBrush& c) { m_embossColor = c; }
	void setCoarseGridWidth(int c) { m_coarseGridWidth = c; }
	void setFineGridWidth(int c) { m_fineGridWidth = c; }
	void setHorizontalWidth(int c) { m_horizontalWidth = c; }
	void setEmbossWidth(int c) { m_embossWidth = c; }
	void setEmbossOffset(int c) { m_embossOffset = c; }

public slots:
	//! @brief Update ourselves by updating all the ClipViews attached.
	void update();

	//! @brief Move the trackContentWidget to a new place in time
	//!
	//! Responsible for moving track-content-widgets to appropriate position after change of visible viewport
	//!
	//! @param newPos The MIDI time to move to.
	void changePosition( const lmms::TimePos & newPos = TimePos( -1 ) );

	//! @brief Updates the background tile pixmap.
	void updateBackground();

protected:
	enum class ContextMenuAction
	{
		Paste
	};

	void contextMenuEvent( QContextMenuEvent * cme ) override;
	void contextMenuAction( QContextMenuEvent * cme, ContextMenuAction action );

	//! @brief Respond to a drag enter event on the trackContentWidget
	//! @param dee the Drag Enter Event to respond to
	void dragEnterEvent(QDragEnterEvent* dee) override;

	//! @brief Respond to a drop event on the trackContentWidget
	//! @param de The Drop Event to respond to
	void dropEvent(QDropEvent* de) override;

	//! @brief Respond to a mouse press on the trackContentWidget
	//! @param me The mouse press event to respond to
	void mousePressEvent(QMouseEvent* me) override;

	void mouseReleaseEvent( QMouseEvent * me ) override;

	//! @brief Repaint the trackContentWidget on command
	//! @param pe The Paint Event to respond to
	void paintEvent(QPaintEvent* pe) override;

	//! @brief Updates the background tile pixmap on size changes.
	//! @param re The resize event to pass to base class
	void resizeEvent(QResizeEvent* re) override;

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
	//! @brief Return the track shown by the trackContentWidget
	Track* getTrack();

	//! @brief Return the position of the trackContentWidget in bars.
	//! @param mouseX The mouse's current X position in pixels.
	TimePos getPosition(int mouseX);

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
