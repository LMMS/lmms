/*
 * VideoClipView.cpp
 *
 * Copyright (c) 2024 regulus79
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
#include <QApplication>
#include <QMenu>
#include <QPainter>

#include "embed.h"
#include "PathUtil.h"
#include "Song.h"
#include "Track.h" // I don't think this is right
#include "VideoClipView.h"
#include "VideoClip.h"
#include "VideoClipWindow.h"

namespace lmms::gui
{

VideoClipView::VideoClipView(VideoClip * clip, TrackView * tv):
    ClipView(clip, tv),
    m_clip(clip),
    m_paintPixmap()
{
    //updateVideo();
	m_window = new VideoClipWindow(m_clip);
	m_window->toggleVisibility(false);

	setStyle(QApplication::style());
}



void VideoClipView::mouseDoubleClickEvent(QMouseEvent * me)
{
	m_window->toggleVisibility(m_window->parentWidget()->isHidden());
}


void VideoClipView::paintEvent(QPaintEvent* pe)
{
    QPainter painter(this);

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate(false);

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != size())
	{
		m_paintPixmap = QPixmap(size());
	}
    
	QPainter p(&m_paintPixmap);

    bool muted = m_clip->getTrack()->isMuted() || m_clip->isMuted();
	bool selected = isSelected();

	QLinearGradient lingrad(0, 0, 0, height());
	QColor c = painter.background().color();
	if (muted) { c = c.darker(150); }
	if (selected) { c = c.darker(150); }
	lingrad.setColorAt( 1, c.darker( 300 ) );
	lingrad.setColorAt( 0, c );

	// paint a black rectangle under the clip to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}

	auto clipColor = m_clip->color().value_or(m_clip->getTrack()->color().value_or(painter.pen().brush().color()));

	p.setPen(clipColor);

	if (muted)
	{
		QColor penColor = p.pen().brush().color();
		penColor.setHsv(penColor.hsvHue(), penColor.hsvSaturation() / 4, penColor.value());
		p.setPen(penColor.darker(250));
	}
	if (selected)
	{
		p.setPen(p.pen().brush().color().darker(150));
	}

	const int spacing = BORDER_WIDTH + 1;
	const float ppb = fixedClips() ?
			( parentWidget()->width() - 2 * BORDER_WIDTH )
					/ (float) m_clip->length().getBar() :
								pixelsPerBar();

	float nom = Engine::getSong()->getTimeSigModel().getNumerator();
	float den = Engine::getSong()->getTimeSigModel().getDenominator();
	float ticksPerBar = DefaultTicksPerBar * nom / den;

	float offset =  m_clip->startTimeOffset() / ticksPerBar * pixelsPerBar();
	//QRect r = QRect( offset, spacing,
	//		qMax( static_cast<int>( m_clip->sampleLength() * ppb / ticksPerBar ), 1 ), rect().bottom() - 2 * spacing );

    QString name = PathUtil::cleanName(m_clip->videoFile());
	paintTextLabel(name, p);

	// disable antialiasing for borders, since its not needed
	p.setRenderHint( QPainter::Antialiasing, false );

	// inner border
	p.setPen( c.lighter( 135 ) );
	p.drawRect( 1, 1, rect().right() - BORDER_WIDTH,
		rect().bottom() - BORDER_WIDTH );

	// outer border
	p.setPen( c.darker( 200 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );

	// draw the 'muted' pixmap only if the clip was manualy muted
	if( m_clip->isMuted() )
	{
		const int spacing = BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}

	if ( m_marker )
	{
		p.drawLine(m_markerPos, rect().bottom(), m_markerPos, rect().top());
	}

	p.end();

	painter.drawPixmap( 0, 0, m_paintPixmap );
}

bool VideoClipView::splitClip(const TimePos pos)
{
    return false;
}

} // namespace lmms::gui