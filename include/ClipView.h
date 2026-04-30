/*
 * ClipView.h - declaration of ClipView class
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

#ifndef LMMS_GUI_CLIP_VIEW_H
#define LMMS_GUI_CLIP_VIEW_H

#include <optional>

#include <QVector>

#include "ModelView.h"
#include "Rubberband.h"
#include "Clip.h"


class QMenu;

namespace lmms
{

class DataFile;

namespace gui
{

class TextFloat;
class TrackView;


class ClipView : public selectableObject, public ModelView
{
	Q_OBJECT

// theming qproperties
	Q_PROPERTY( QColor mutedColor READ mutedColor WRITE setMutedColor )
	Q_PROPERTY( QColor mutedBackgroundColor READ mutedBackgroundColor WRITE setMutedBackgroundColor )
	Q_PROPERTY( QColor selectedColor READ selectedColor WRITE setSelectedColor )
	Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )
	Q_PROPERTY( QColor textBackgroundColor READ textBackgroundColor WRITE setTextBackgroundColor )
	Q_PROPERTY( QColor textShadowColor READ textShadowColor WRITE setTextShadowColor )
	Q_PROPERTY( QColor patternClipBackground READ patternClipBackground WRITE setPatternClipBackground )
	Q_PROPERTY( bool gradient READ gradient WRITE setGradient )
	Q_PROPERTY(QColor markerColor READ markerColor WRITE setMarkerColor)

public:
	const static int BORDER_WIDTH = 2;

	//! @brief Creates a new clip view for the given clip in the given track view.
	//! @param clip The clip to be displayed
	//! @param tv The track view that will contain the new object
	ClipView(Clip* clip, TrackView* tv);

	//! @brief Destroys the given ClipView.
	~ClipView() override;

	//! @brief Checks if the containing trackView has fixed Clips.
	//! @todo In what circumstance are they fixed?
	bool fixedClips();

	inline Clip * getClip()
	{
		return m_clip;
	}

	inline TrackView * getTrackView()
	{
		return m_trackView;
	}

	// qproperty access functions, to be inherited & used by Clipviews
	QColor mutedColor() const { return m_mutedColor; }
	QColor mutedBackgroundColor() const { return m_mutedBackgroundColor; }
	QColor selectedColor() const { return m_selectedColor; }
	QColor textColor() const { return m_textColor; }
	QColor textBackgroundColor() const { return m_textBackgroundColor; }
	QColor textShadowColor() const { return m_textShadowColor; }
	QColor patternClipBackground() const { return m_patternClipBackground; }
	bool gradient() const { return m_gradient; }
	QColor markerColor() const { return m_markerColor; }
	void setMutedColor(const QColor& c) { m_mutedColor = QColor(c); }
	void setMutedBackgroundColor(const QColor& c) { m_mutedBackgroundColor = QColor(c); }
	void setSelectedColor(const QColor& c) { m_selectedColor = QColor(c); }
	void setTextColor(const QColor& c) { m_textColor = QColor(c); }
	void setTextBackgroundColor(const QColor& c) { m_textBackgroundColor = QColor(c); }
	void setTextShadowColor(const QColor& c) { m_textShadowColor = QColor(c); }
	void setPatternClipBackground(const QColor& c) { m_patternClipBackground = QColor(c); }
	void setGradient(const bool& b) { m_gradient = b; }
	void setMarkerColor(const QColor& c) { m_markerColor = QColor(c); }

	bool needsUpdate() const { return m_needsUpdate; }
	void setNeedsUpdate(bool b) { m_needsUpdate = b; }

	// Method to get a QVector of Clips to be affected by a context menu action
	QVector<ClipView *> getClickedClips();

	// Methods to remove, copy, cut, paste and mute a QVector of Clip views
	void copy( QVector<ClipView *> clipvs );
	void cut( QVector<ClipView *> clipvs );
	void paste();
	// remove and toggleMute are static because they don't depend
	// being called from a particular Clip view, but can be called anywhere as long
	// as a valid Clip view list is given, while copy/cut require an instance for
	// some metadata to be written to the clipboard.
	static void remove( QVector<ClipView *> clipvs );
	static void toggleMute( QVector<ClipView *> clipvs );

	void toggleSelectedAutoResize();

	QColor getColorForDisplay( QColor );

	void inline setMarkerPos(int x) { m_markerPos = x; }
	void inline setMarkerEnabled(bool e) { m_marker = e; }

public slots:

	//! @brief Close a ClipView
	//!
	//! Closes a ClipView by asking the track view to remove it and then asking the QWidget to close it.
	//!
	//! @return Boolean state of whether the QWidget was able to close.
	virtual bool close();

	//! @brief Removes a ClipView from its track view.
	//!
	//! Like the @ref close method, this asks the track view to remove this ClipView. However, the clip is scheduled for
	//! later deletion rather than closed immediately.
	void remove();

	//! @brief Update a ClipView
	//!
	//! Clips get drawn only when needed, and when a Clip is updated, it needs to be redrawn.
	void update() override;

	void selectColor();
	void randomizeColor();
	void resetColor();

protected:
	enum class ContextMenuAction
	{
		Remove,
		Cut,
		Copy,
		Paste,
		Mute
	};

	TrackView * m_trackView;
	TimePos m_initialClipPos;
	TimePos m_initialClipEnd;

	bool m_marker = false;
	int m_markerPos = 0;

	virtual void constructContextMenu( QMenu * )
	{
	}

	//! @brief Set up the context menu for this ClipView.
	//!
	//! Set up the various context menu events that can apply to a ClipView.
	//!
	//! @param cme The QContextMenuEvent to add the actions to.
	void contextMenuEvent( QContextMenuEvent * cme ) override;

	void contextMenuAction( ContextMenuAction action );

	//! @brief Change the ClipView's display when something being dragged enters it.
	//!
	//! We need to notify Qt to change our display if something being dragged has entered our 'airspace'.
	//!
	//! @param dee The QDragEnterEvent to watch.
	void dragEnterEvent(QDragEnterEvent* dee) override;

	//! @brief Handle something being dropped on this ClipObjectView.
	//!
	//! When something has been dropped on this ClipView, and it's a clip, then use an instance of our dataFile reader to
	//! take the xml of the clip and turn it into something we can write over our current state.
	//!
	//! @param de The QDropEvent to handle.
	void dropEvent( QDropEvent * de ) override;

	//! @brief Handle a mouse press on this ClipView.
	//!
	//! Handles the various ways in which a ClipView can be used with a click of a mouse button.
	//!
	//! - If our container supports rubber band selection then handle selection events.
	//! - or if shift-left button, add this object to the selection
	//! - or if ctrl-left button, start a drag-copy event
	//! - or if just plain left button, resize if we're resizeable
	//! - or if ctrl-middle button, mute the clip
	//! - or if middle button, maybe delete the clip.
	//!
	//! @param me The QMouseEvent to handle.
	void mousePressEvent(QMouseEvent* me) override;

	//! @brief Handle a mouse movement (drag) on this ClipView.
	//!
	//! Handles the various ways in which a ClipView can be used with a mouse drag.
	//!
	//! - If in move mode, move ourselves in the track,
	//! - or if in move-selection mode, move the entire selection,
	//! - or if in resize mode, resize ourselves,
	//! - otherwise ???
	//!
	//! @param me The QMouseEvent to handle.
	//! @todo What does the final else case do here?
	void mouseMoveEvent(QMouseEvent* me) override;

	//! @brief Handle a mouse release on this ClipView.
	//!
	//! If we're in move or resize mode, journal the change as appropriate. Then tidy up.
	//!
	//! @param me The QMouseEvent to handle.
	void mouseReleaseEvent(QMouseEvent* me) override;

	void resizeEvent( QResizeEvent * re ) override
	{
		m_needsUpdate = true;
		selectableObject::resizeEvent( re );
	}

	bool unquantizedModHeld( QMouseEvent * me );
	TimePos quantizeSplitPos(TimePos);

	//! @brief How many pixels a bar takes for this ClipView.
	//! @return the number of pixels per bar.
	float pixelsPerBar();

	//! @brief Create a DataFile suitable for copying multiple clips.
	//!
	//!	Clips in the vector are written to the "clips" node in the DataFile. The ClipView's initial mouse position is
	//! written to the "initialMouseX" node in the DataFile.  When dropped on a track, this is used to create copies of
	//! the Clips.
	//!
	//! @param clips The trackContectObjects to save in a DataFile
	DataFile createClipDataFiles(const QVector<ClipView *> & clips) const;

	virtual void paintTextLabel(QString const & text, QPainter & painter);

	auto hasCustomColor() const -> bool;

protected slots:
	//! @brief Updates a ClipView's length
	//!
	//! If this ClipView has a fixed Clip, then we must keep the width of our parent.  Otherwise, calculate our width
	//! from the clip's length in pixels adding in the border.
	void updateLength();

	//! @brief Updates a ClipView's position.
	//!
	//! Ask our track view to change our position.  Then make sure that the track view is updated in case this position
	//! has changed the track view's length.
	void updatePosition();

private:
	enum class Action
	{
		None,
		Move,
		MoveSelection,
		Resize,
		ResizeLeft,
		Split,
		CopySelection,
		ToggleSelected
	} ;

	static TextFloat * s_textFloat;

	Clip * m_clip;
	Action m_action;
	QPoint m_initialMousePos;
	QPoint m_initialMouseGlobalPos;
	QVector<TimePos> m_initialOffsets;

	TextFloat * m_hint;

// qproperty fields
	QColor m_mutedColor;
	QColor m_mutedBackgroundColor;
	QColor m_selectedColor;
	QColor m_textColor;
	QColor m_textBackgroundColor;
	QColor m_textShadowColor;
	QColor m_patternClipBackground;
	bool m_gradient;
	QColor m_markerColor;

	bool m_needsUpdate;
	inline void setInitialPos( QPoint pos )
	{
		m_initialMousePos = pos;
		m_initialMouseGlobalPos = mapToGlobal( pos );
		m_initialClipPos = m_clip->startPosition();
		m_initialClipEnd = m_initialClipPos + m_clip->length();
	}

	//! @brief Save the offsets between all selected tracks and a clicked track
	void setInitialOffsets();

	//! @brief Detect whether the mouse moved more than n pixels on screen.
	//! @param me The QMouseEvent.
	//! @param distance The threshold distance that the mouse has moved to return true.
	bool mouseMovedDistance(QMouseEvent* me, int distance);

	//! @brief Calculate the new position of a dragged Clip from a mouse event
	//! @param me The QMouseEvent
	TimePos draggedClipPos(QMouseEvent* me);

	int knifeMarkerPos( QMouseEvent * me );

	//! @brief Change color of all selected clips
	//! @param color The new color, if any.
	void setColor(const std::optional<QColor>& color);
	
	//! Returns whether the user can left-resize this clip so that the start of the clip bounds is before the start of the clip content.
	virtual bool isResizableBeforeStart() { return true; };

	
	//! @brief Split this Clip into two clips
	//! @param pos the position of the split, relative to the start of the clip
	//! @return true if the clip could be split
	bool splitClip(const TimePos pos);

	//! @brief Destructively split this Clip into two clips.
	//!
	//! If the clip type does not implement this feature, it will default to normal splitting.
	//!
	//! @param pos the position of the split, relative to the start of the clip
	//! @return true if the clip could be split
	virtual bool destructiveSplitClip(const TimePos pos)
	{
		return splitClip(pos);
	}

	//! @brief Chooses the correct cursor to be displayed on the widget
	//! @param me The QMouseEvent that is triggering the cursor change
	void updateCursor(QMouseEvent* me);
} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_CLIP_VIEW_H
