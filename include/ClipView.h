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

#ifndef TRACK_CONTENT_OBJECT_VIEW_H
#define TRACK_CONTENT_OBJECT_VIEW_H


#include <QVector>

#include "ModelView.h"
#include "Rubberband.h"
#include "Clip.h"


class QMenu;
class QContextMenuEvent;

namespace lmms
{

class DataFile;
class Clip;

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
	// We have to use a QSize here because using QPoint isn't supported.
	// width -> x, height -> y
	Q_PROPERTY( QSize mouseHotspotHand MEMBER m_mouseHotspotHand )
	Q_PROPERTY( QSize mouseHotspotKnife MEMBER m_mouseHotspotKnife )

public:
	const static int BORDER_WIDTH = 2;

	ClipView( Clip * clip, TrackView * tv );
	virtual ~ClipView();

	bool fixedClips();

	inline Clip * getClip()
	{
		return m_clip;
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
	QColor patternClipBackground() const;
	bool gradient() const;
	void setMutedColor( const QColor & c );
	void setMutedBackgroundColor( const QColor & c );
	void setSelectedColor( const QColor & c );
	void setTextColor( const QColor & c );
	void setTextBackgroundColor( const QColor & c );
	void setTextShadowColor( const QColor & c );
	void setPatternClipBackground(const QColor& c);
	void setGradient( const bool & b );

	// access needsUpdate member variable
	bool needsUpdate();
	void setNeedsUpdate( bool b );

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
	static void mergeClips(QVector<ClipView*> clipvs);

	// Returns true if selection can be merged and false if not
	static bool canMergeSelection(QVector<ClipView*> clipvs);

	QColor getColorForDisplay( QColor );

	void inline setMarkerPos(int x) { m_markerPos = x; }
	void inline setMarkerEnabled(bool e) { m_marker = e; }

public slots:
	virtual bool close();
	void remove();
	void update() override;

	void selectColor();
	void randomizeColor();
	void resetColor();

protected:
	enum ContextMenuAction
	{
		Remove,
		Cut,
		Copy,
		Paste,
		Mute,
		Merge
	};

	TrackView * m_trackView;
	TimePos m_initialClipPos;
	TimePos m_initialClipEnd;

	bool m_marker = false;
	int m_markerPos = 0;

	virtual void constructContextMenu( QMenu * )
	{
	}

	void contextMenuEvent( QContextMenuEvent * cme ) override;
	void contextMenuAction( ContextMenuAction action );
	void dragEnterEvent( QDragEnterEvent * dee ) override;
	void dropEvent( QDropEvent * de ) override;
	void mousePressEvent( QMouseEvent * me ) override;
	void mouseMoveEvent( QMouseEvent * me ) override;
	void mouseReleaseEvent( QMouseEvent * me ) override;
	void resizeEvent( QResizeEvent * re ) override
	{
		m_needsUpdate = true;
		selectableObject::resizeEvent( re );
	}

	bool unquantizedModHeld( QMouseEvent * me );
	TimePos quantizeSplitPos( TimePos, bool shiftMode );

	float pixelsPerBar();


	DataFile createClipDataFiles(const QVector<ClipView *> & clips) const;

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
		Split,
		CopySelection,
		ToggleSelected
	} ;

	static TextFloat * s_textFloat;

	Clip * m_clip;
	Actions m_action;
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
	QSize m_mouseHotspotHand; // QSize must be used because QPoint
	QSize m_mouseHotspotKnife; // isn't supported by property system
	QCursor m_cursorHand;
	QCursor m_cursorKnife;
	bool m_cursorSetYet;

	bool m_needsUpdate;
	inline void setInitialPos( QPoint pos )
	{
		m_initialMousePos = pos;
		m_initialMouseGlobalPos = mapToGlobal( pos );
		m_initialClipPos = m_clip->startPosition();
		m_initialClipEnd = m_initialClipPos + m_clip->length();
	}
	void setInitialOffsets();

	bool mouseMovedDistance( QMouseEvent * me, int distance );
	TimePos draggedClipPos( QMouseEvent * me );
	int knifeMarkerPos( QMouseEvent * me );
	void setColor(const QColor* color);
	//! Return true iff the clip could be split. Currently only implemented for samples
	virtual bool splitClip( const TimePos pos ){ return false; };
	void updateCursor(QMouseEvent * me);
} ;


} // namespace gui

} // namespace lmms

#endif
