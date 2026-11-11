/*
 * PatternClipView.cpp
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

#include "PatternClipView.h"

#include <QApplication>
#include <QMenu>
#include <QPainter>

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "MidiClip.h"
#include "PatternClip.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "RenameDialog.h"
#include "TrackContainerView.h"
#include "TrackView.h"

namespace lmms::gui
{


PatternClipView::PatternClipView(Clip* _clip, TrackView* _tv) :
	ClipView( _clip, _tv ),
	m_patternClip(dynamic_cast<PatternClip*>(_clip)),
	m_paintPixmap()
{
	connect( _clip->getTrack(), SIGNAL(dataChanged()), 
			this, SLOT(update()));
	connect(Engine::patternStore(), &TrackContainer::trackAdded,
			this, &PatternClipView::update);
	connect(Engine::patternStore(), &TrackContainer::trackRemoved,
			this, &PatternClipView::update);
	connect(Engine::patternStore(), &TrackContainer::trackMoved,
			this, &PatternClipView::update);

	setStyle( QApplication::style() );
}

void PatternClipView::constructContextMenu(QMenu* _cm)
{
	auto a = new QAction(embed::getIconPixmap("pattern_track"), tr("Open in Pattern Editor"), _cm);
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL(triggered(bool)),
			this, SLOT(openInPatternEditor()));
	_cm->insertSeparator( _cm->actions()[1] );
	_cm->addSeparator();
	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT(resetName()));
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT(changeName()));
}




void PatternClipView::mouseDoubleClickEvent(QMouseEvent*)
{
	if (m_trackView->trackContainerView()->knifeMode()) { return; }

	openInPatternEditor();
}




void PatternClipView::paintEvent(QPaintEvent*)
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate( false );

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != size())
	{
		m_paintPixmap = QPixmap(size());
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c = getColorForDisplay( painter.background().color() );
	
	lingrad.setColorAt( 0, c.lighter( 130 ) );
	lingrad.setColorAt( 1, c.lighter( 70 ) );

	// paint a black rectangle under the clip to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	const int pixelsPerPattern = Engine::patternStore()->lengthOfPattern(m_patternClip->patternIndex()) * pixelsPerBar();
	int offset = static_cast<int>(m_patternClip->startTimeOffset() * (pixelsPerBar() / TimePos::ticksPerBar()))
			% pixelsPerPattern;
	if (offset < 2) {
		offset += pixelsPerPattern;
	}

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}

	// Draw notes

	const int patternIndex = static_cast<PatternTrack*>(m_patternClip->getTrack())->patternIndex();
	// Count the number of non-empty instrument tracks.
	// Only used midi tracks will be drawn, but empty tracks will still give a bit of empty space for padding.
	int numberInstrumentTracksUsed = 0;
	for (const auto& track : Engine::patternStore()->tracks())
	{
		const MidiClip* const mClip = dynamic_cast<MidiClip*>(track->getClip(patternIndex));
		if (mClip && mClip->notes().size() > 0)
		{
			numberInstrumentTracksUsed++;
		}
	}
	const auto totalTracks = Engine::patternStore()->tracks().size();
	const auto numberEmptyTracks = totalTracks - numberInstrumentTracksUsed;

	const float totalHeight = height() * (1.0f - 2 * m_verticalPadding);
	const float totalHeightForEmptyTracks = totalHeight * numberEmptyTracks / totalTracks * m_emptyTrackHeightRatio;
	const float totalHeightForTracks = totalHeight - totalHeightForEmptyTracks;

	const float trackHeight = numberInstrumentTracksUsed > 0
		? totalHeightForTracks / numberInstrumentTracksUsed
		: 0.f;
	const float emptyTrackHeight = numberEmptyTracks > 0
		? totalHeightForEmptyTracks / numberEmptyTracks
		: 0.f;

	const int verticalNoteSpacing = trackHeight * m_noteVerticalSpacing;
	const int horizontalNoteSpacing = pixelsPerBar() / TimePos::stepsPerBar() * m_noteHorizontalSpacing;

	// This pixmap is a batching optimization which draws notes into one vertical strip for tiling patterns.
	QPixmap mPatternPixmap(pixelsPerPattern, m_paintPixmap.height());
	mPatternPixmap.fill(Qt::transparent);

	QPainter pixPainter(&mPatternPixmap);
	pixPainter.setPen(Qt::NoPen);
	const auto noteColor = QColor(m_noteColor.red(), m_noteColor.green(), m_noteColor.blue());
	pixPainter.setBrush(QBrush(noteColor));

	TimePos longestLength(-1);
	float lastY = height() * m_verticalPadding;
	for (const auto& track : Engine::patternStore()->tracks())
	{
		const MidiClip* const mClip = dynamic_cast<MidiClip*>(track->getClip(patternIndex));
		if (!mClip || mClip->notes().size() == 0)
		{
			lastY += emptyTrackHeight;
			continue;
		}

		longestLength = std::max(longestLength, mClip->length());
		for (const Note* note : mClip->notes())
		{
			QRect noteRect = QRect(
				note->pos() * pixelsPerBar() / TimePos::ticksPerBar(),
				lastY + verticalNoteSpacing / 2,
				std::max(1.0f, pixelsPerBar() / TimePos::stepsPerBar() - horizontalNoteSpacing),
				std::max(1.0f, trackHeight - verticalNoteSpacing)
			);
			pixPainter.setOpacity(std::clamp(note->getVolume() / 100.0, 0.2, 1.0));
			pixPainter.drawRect(noteRect);
		}

		lastY += trackHeight;
	}
	// Compare how long the longest clip view is compared to the underlying pattern. First +1 for ceiling, second +1 for possible previous bar.
	const int maxPossibleRepetions = getClip()->length() / longestLength + 1 + 1;
	p.drawTiledPixmap(offset + horizontalNoteSpacing / 2 -pixelsPerPattern, 0, width() + (maxPossibleRepetions-1)*pixelsPerPattern, height(), mPatternPixmap);

	// bar lines
	const int lineSize = 3;

	p.setPen( c.darker( 200 ) );

	if (pixelsPerPattern > 0)
	{
		for (int x = offset; x < width() - 2; x += pixelsPerPattern)
		{
			p.drawLine( x, BORDER_WIDTH, x, BORDER_WIDTH + lineSize );
			p.drawLine( x, rect().bottom() - ( BORDER_WIDTH + lineSize ),
			 	x, rect().bottom() - BORDER_WIDTH );
		}
	}

	// clip name
	paintTextLabel(m_patternClip->name(), p);

	// inner border
	p.setPen( c.lighter( 130 ) );
	p.drawRect( 1, 1, rect().right() - BORDER_WIDTH,
		rect().bottom() - BORDER_WIDTH );	

	// outer border
	p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
	
	// draw the 'muted' pixmap only if the clip was manually muted
	if (m_patternClip->isMuted())
	{
		const int spacing = BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}
	
	if (m_marker)
	{
		p.setPen(markerColor());
		p.drawLine(m_markerPos, rect().bottom(), m_markerPos, rect().top());
	}
	
	p.end();
	
	painter.drawPixmap( 0, 0, m_paintPixmap );
}




void PatternClipView::openInPatternEditor()
{
	Engine::patternStore()->setCurrentPattern(m_patternClip->patternIndex());

	getGUI()->mainWindow()->togglePatternEditorWin(true);
}




void PatternClipView::resetName() { m_patternClip->setName(""); }




void PatternClipView::changeName()
{
	QString s = m_patternClip->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_patternClip->setName(s);
}



void PatternClipView::update()
{
	setToolTip(m_patternClip->name());

	ClipView::update();
}

} // namespace lmms::gui
