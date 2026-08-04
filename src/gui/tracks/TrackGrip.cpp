/*
 * TrackGrip.cpp - Grip that can be used to move tracks
 *
 * Copyright (c) 2024- Michael Gregorius
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

#include "TrackGrip.h"

#include "DataFile.h"
#include "embed.h"
#include "KeyboardShortcuts.h"
#include "StringPairDrag.h"
#include "Track.h"
#include "TrackView.h"

#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>


namespace lmms::gui
{

QPixmap* TrackGrip::s_grabbedPixmap = nullptr;
QPixmap* TrackGrip::s_releasedPixmap = nullptr;

constexpr int c_margin = 2;

TrackGrip::TrackGrip(TrackView* trackView, QWidget* parent) :
	QWidget(parent),
	m_trackView(trackView)
{
	if (!s_grabbedPixmap)
	{
		s_grabbedPixmap = new QPixmap(embed::getIconPixmap("track_op_grip_c"));
	}

	if (!s_releasedPixmap)
	{
		s_releasedPixmap = new QPixmap(embed::getIconPixmap("track_op_grip"));
	}

	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

	setCursor(Qt::OpenHandCursor);

	setFixedWidth(std::max(s_grabbedPixmap->width(), s_releasedPixmap->width()) + 2 * c_margin);
}

void TrackGrip::mousePressEvent(QMouseEvent* m)
{
	// Allow the user to Ctrl-drag copy the track if it is not a pattern track
	if (m->button() == Qt::LeftButton
		&& m->modifiers() & KBD_COPY_MODIFIER
		&& m_trackView->getTrack()->type() != Track::Type::Pattern)
	{
		m->accept();
		DataFile dataFile(DataFile::Type::DragNDropData);
		m_trackView->getTrack()->saveState(dataFile, dataFile.content());
		new StringPairDrag(
			QString("track_%1").arg(static_cast<int>(m_trackView->getTrack()->type())),
			dataFile.toString(),
			m_trackView->getTrackSettingsWidget()->grab(),
			this
		);
	}
	else
	{
		m->accept();

		m_isGrabbed = true;
		setCursor(Qt::ClosedHandCursor);

		emit grabbed();
		update();
	}
}

void TrackGrip::mouseReleaseEvent(QMouseEvent* m)
{
	m->accept();

	m_isGrabbed = false;
	setCursor(Qt::OpenHandCursor);

	emit released();

	update();
}

void TrackGrip::paintEvent(QPaintEvent*)
{
	QPainter p(this);

	// Check if the color of the track should be used for the background
	const auto color = m_trackView->getTrack()->color();
	const auto muted = m_trackView->getTrack()->getMutedModel()->value();

	if (color.has_value() && !muted) 
	{
		p.fillRect(rect(), color.value());
	}

	// Paint the pixmap
	auto r = rect().marginsRemoved(QMargins(c_margin, c_margin, c_margin, c_margin));
	p.drawTiledPixmap(r, m_isGrabbed ? *s_grabbedPixmap : *s_releasedPixmap);
}

} // namespace lmms::gui
