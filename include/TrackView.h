/*
 * TrackView.h - declaration of TrackView class
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

#ifndef LMMS_GUI_TRACK_VIEW_H
#define LMMS_GUI_TRACK_VIEW_H

#include <QWidget>

#include "JournallingObject.h"
#include "ModelView.h"
#include "TrackContentWidget.h"
#include "TrackOperationsWidget.h"

class QMenu;

namespace lmms
{

class Track;
class Clip;


namespace gui
{

class FadeButton;
class TrackContainerView;


const int DEFAULT_SETTINGS_WIDGET_WIDTH = 260;
const int TRACK_OP_WIDTH = 78;
// This shaves 150-ish pixels off track buttons,
// ruled from config: ui.compacttrackbuttons
const int DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT = 136;
const int TRACK_OP_WIDTH_COMPACT = TRACK_OP_WIDTH;


class TrackView : public QWidget, public ModelView, public JournallingObject
{
	Q_OBJECT
public:

	//! @brief Create a new track View.
	//!
	//! The track View is handles the actual display of the track, including
	//! displaying its various widgets and the track segments.
	//!
	//! @param track The track to display.
	//! @param tcv The track Container View for us to be displayed in.
	TrackView(Track* track, TrackContainerView* tcv);

	~TrackView() override = default;

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
		return m_action == Action::Move;
	}

	//! @brief Update this track View and all its content objects.
	virtual void update();

	//! @brief Create a menu for assigning/creating channels for this track.
	//!
	//! Currently instrument track and sample track supports it
	virtual QMenu* createMixerMenu(QString title, QString newMixerLabel);


public slots:

	//! @brief Close this track View.
	virtual bool close();


protected:
	//! @brief Register that the model of this track View has changed.
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


	//! @brief Start a drag event on this track View.
	//! @param dee The DragEnterEvent to start.
	void dragEnterEvent(QDragEnterEvent* dee) override;

	//! @brief Accept a drop event on this track View.
	//!
	//! We only accept drop events that are of the same type as this track.
	//! If so, we decode the data from the drop event by just feeding it back into the engine as a state.
	//!
	//! @param de The DropEvent to handle.
	void dropEvent(QDropEvent* de) override;

	//! @brief Handle a mouse press event on this track View.
	//!
	//! - If this track container supports rubber band selection, let the widget handle that and don't bother with any
	//! other handling.
	//! - If the left mouse button is pressed, we handle two things. If SHIFT is pressed, then we resize vertically.
	//! Otherwise we start the process of moving this track to a new position.
	//! - Otherwise we let the widget handle the mouse event as normal.
	//!
	//! @param me The MouseEvent to handle.
	void mousePressEvent(QMouseEvent* me) override;

	//! @brief Handle a mouse move event on this track View.
	//!
	//! If this track container supports rubber band selection, let the widget handle that and don't bother with any
	//! other handling.
	//!
	//! Otherwise if we've started the move process (from mousePressEvent()) then move ourselves into that position,
	//! reordering the track list with moveTrackViewUp() and moveTrackViewDown() to suit. We make a note of this in the
	//! undo journal in case the user wants to undo this move.
	//!
	//! Likewise if we've started a resize process, handle this too, making sure that we never go below the minimum track
	//! height.
	//!
	//! @param me The MouseEvent to handle.
	void mouseMoveEvent(QMouseEvent* me) override;

	//! @brief Handle a mouse release event on this track View.
	//! @param me The MouseEvent to handle.
	void mouseReleaseEvent(QMouseEvent* me) override;

	void wheelEvent(QWheelEvent* we) override;

	//! @brief Repaint this track View.
	//! @param pe The PaintEvent to start.
	void paintEvent(QPaintEvent* pe) override;

	//! @brief Resize this track View.
	//! @param re The Resize Event to handle.
	void resizeEvent(QResizeEvent* re) override;

private:
	void resizeToHeight(int height);

private:
	enum class Action
	{
		None,
		Move,
		Resize
	} ;

	Track * m_track;
	TrackContainerView * m_trackContainerView;

	TrackOperationsWidget m_trackOperationsWidget;
	QWidget m_trackSettingsWidget;
	TrackContentWidget m_trackContentWidget;

	Action m_action = Action::None;

	virtual FadeButton * getActivityIndicator()
	{
		return nullptr;
	}

	void setIndicatorMute(FadeButton* indicator, bool muted);

	friend class TrackLabelButton;


private slots:
	//! @brief Create a Clip View in this track View.
	//! @param clip The Clip to create the view for.
	void createClipView(lmms::Clip* clip);

	void muteChanged();
	void onTrackGripGrabbed();
	void onTrackGripReleased();
} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_TRACK_VIEW_H
